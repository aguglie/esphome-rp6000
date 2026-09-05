#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace esphome {
namespace rp6000 {

static const char *const TAG = "RP6000";

class RP6000 : public Component, public uart::UARTDevice {
 public:
  void setup() override {
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    enter_recovery_();
  }

  // Polling only schedules a request. loop() owns all UART reads and writes.
  void updateStatus() { status_requested_ = true; }
  void add_on_status_callback(std::function<void()> callback) { status_callbacks_.push_back(std::move(callback)); }
  bool has_fresh_status(uint32_t max_age = 30000) const {
    return has_status_ && millis() - last_status_at_ <= max_age;
  }

  bool queue_command(const std::string &key, const std::string &command, std::function<void(bool)> done) {
    Request request;
    request.key = key;
    request.command = command;
    request.done = std::move(done);
    // Keep only the latest unsent setpoint for each output.
    for (auto &queued : commands_) {
      if (queued.key == key) {
        auto previous_done = std::move(queued.done);
        queued = std::move(request);
        if (previous_done) previous_done(false);
        return true;
      }
    }
    if (commands_.size() >= 8) {
      ESP_LOGW(TAG, "Command queue full");
      return false;
    }
    commands_.push_back(std::move(request));
    return true;
  }

  void loop() override {
    uint8_t byte;
    // Bound work and memory even on a noisy/unterminated stream.
    for (size_t count = 0; count < 256 && available(); count++) {
      if (!read_byte(&byte)) break;
      last_rx_at_ = millis();
      if (byte == '\r' || byte == '\n') {
        if (discard_frame_) {
          discard_frame_ = false;
          rx_.clear();
        } else if (!rx_.empty()) {
          auto frame = std::move(rx_);
          rx_.clear();
          handle_frame_(frame);
        }
      } else if (!discard_frame_) {
        if (rx_.size() >= 160) {
          ESP_LOGW(TAG, "Oversized reply discarded");
          rx_.clear();
          discard_frame_ = true;
        } else {
          rx_.push_back(byte);
        }
      }
    }
    const uint32_t now = millis();
    if (state_ == State::WAITING && now - sent_at_ >= RESPONSE_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Timeout waiting for %s (%u ms)", waiting_status_ ? "Q6 status" : "command ACK",
               static_cast<unsigned>(now - sent_at_));
      retry_or_fail_();
      // Logging/callbacks can advance millis(). Re-evaluate recovery with a
      // fresh timestamp on the next loop, never subtract its start from 'now'.
      return;
    }
    if ((!rx_.empty() || discard_frame_) && now - last_rx_at_ >= PARTIAL_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Incomplete reply discarded");
      rx_.clear();
      discard_frame_ = false;
      if (state_ == State::IDLE) {
        enter_recovery_();
        return;
      }
    }
    if (available() || !rx_.empty() || discard_frame_) return;
    if (state_ == State::RECOVERING) {
      if (now - recovery_at_ < RECOVERY_MS || now - last_rx_at_ < QUIET_MS) return;
      state_ = State::IDLE;
      ESP_LOGD(TAG, "UART recovery complete");
    }
    if (state_ != State::IDLE || now - last_rx_at_ < QUIET_MS) return;
    if (!commands_.empty()) {
      active_ = std::move(commands_.front());
      commands_.pop_front();
      waiting_status_ = false;
      active_.attempts++;
      send_(active_.command);
    } else if (status_requested_) {
      status_requested_ = false;
      waiting_status_ = true;
      send_("Q6\r\n");
    }
  }

  uint16_t grid_voltage = 0;
  float grid_frequency = 0;
  uint16_t out_voltage = 0;
  float out_frequency = 0;
  float battery_voltage = 0;
  float battery_charge_current = 0;
  uint16_t power_volt_ampere = 0;
  uint16_t power_watt = 0;
  uint16_t load_percentage = 0;
  float temperature = 0;

 protected:
  // At 2400 baud a Q6 frame takes ~380 ms on the wire. Arduino's RX FIFO
  // can expose the first bytes only after the frame ends. Never use a 100 ms
  // available() timeout to decide that the inverter has stopped transmitting.
  static constexpr uint32_t RESPONSE_TIMEOUT_MS = 1500;
  static constexpr uint32_t RECOVERY_MS = 2000;
  static constexpr uint32_t QUIET_MS = 200;
  static constexpr uint32_t PARTIAL_TIMEOUT_MS = 500;
  enum class State { IDLE, WAITING, RECOVERING };
  struct Request {
    std::string key;
    std::string command;
    std::function<void(bool)> done;
    unsigned attempts = 0;
  };

  void send_(const std::string &command) {
    state_ = State::WAITING;
    digitalWrite(5, HIGH);
    write_str(command.c_str());
    flush();  // Wait for TX completion before releasing the RS485 driver.
    digitalWrite(5, LOW);
    sent_at_ = millis();
    ESP_LOGD(TAG, "Sent %s; waiting for %s", command.substr(0, command.find('\r')).c_str(),
             waiting_status_ ? "status" : "ACK");
  }

  void enter_recovery_() {
    state_ = State::RECOVERING;
    recovery_at_ = millis();
    last_rx_at_ = millis();
  }

  void retry_or_fail_() {
    enter_recovery_();
    if (waiting_status_) return;
    // A newer setpoint takes precedence over retrying the previous one.
    const bool replaced = std::any_of(commands_.begin(), commands_.end(),
                                     [this](const Request &r) { return r.key == active_.key; });
    auto failed = std::move(active_);
    active_ = Request{};
    if (!replaced && failed.attempts < 3) {
      ESP_LOGW(TAG, "Retry scheduled after UART recovery (attempt %u/3)", failed.attempts + 1);
      commands_.push_front(std::move(failed));
    } else if (failed.done) {
      failed.done(false);
    }
  }

  static bool token_(const std::vector<uint8_t> &frame, const char *token) {
    size_t offset = !frame.empty() && frame[0] == '(' ? 1 : 0;
    return frame.size() == offset + 3 && std::equal(frame.begin() + offset, frame.end(), token);
  }

  void handle_frame_(const std::vector<uint8_t> &frame) {
    if (state_ != State::WAITING) {
      ESP_LOGD(TAG, "Discarding unsolicited/late reply (%u bytes)", static_cast<unsigned>(frame.size()));
      enter_recovery_();
      return;
    }
    if (token_(frame, "ACK") || token_(frame, "NAK")) {
      if (waiting_status_) {
        ESP_LOGW(TAG, "Late command reply while waiting for Q6; continuing to wait for status");
        return;
      }
      const bool accepted = token_(frame, "ACK");
      ESP_LOGI(TAG, "Command %s: %s after %u ms", active_.command.substr(0, active_.command.find('\r')).c_str(),
               accepted ? "ACK" : "NAK", static_cast<unsigned>(millis() - sent_at_));
      auto done = std::move(active_.done);
      active_ = Request{};
      state_ = State::IDLE;
      if (!accepted) enter_recovery_();
      if (done) done(accepted);
      return;
    }
    if (!waiting_status_) {
      ESP_LOGW(TAG, "Non-ACK reply (%u bytes) while waiting for command; continuing to wait",
               static_cast<unsigned>(frame.size()));
      return;
    }
    if (!parse_status_(frame)) {
      ESP_LOGW(TAG, "Invalid Q6 reply (%u bytes); continuing to wait", static_cast<unsigned>(frame.size()));
      return;
    }
    state_ = State::IDLE;
    last_status_at_ = millis();
    has_status_ = true;
    ESP_LOGD(TAG, "Q6 status received (%u bytes) after %u ms", static_cast<unsigned>(frame.size()),
             static_cast<unsigned>(millis() - sent_at_));
    for (auto &callback : status_callbacks_) callback();
  }

  static bool number_(const std::string &part, float &value) {
    if (part.empty()) return false;
    bool decimal = false, digit = false;
    for (char c : part) {
      if (c == '.' && !decimal) { decimal = true; continue; }
      if (c < '0' || c > '9') return false;
      digit = true;
    }
    if (!digit) return false;
    value = strtof(part.c_str(), nullptr);
    return std::isfinite(value);
  }

  bool parse_status_(const std::vector<uint8_t> &frame) {
    if (frame.empty() || frame.front() != '(') return false;
    std::vector<std::string> parts(1);
    for (size_t i = 1; i < frame.size(); i++) {
      if (frame[i] == ' ') parts.emplace_back();
      else parts.back().push_back(static_cast<char>(frame[i]));
    }
    if (parts.size() < 14) return false;
    // Validate the entire numeric prefix before changing any published value.
    float values[14];
    for (size_t i = 0; i < 14; i++) if (!number_(parts[i], values[i])) return false;
    if (values[0] > 300 || values[1] > 70 || values[2] > 300 || values[3] > 70 ||
        values[4] < 8 || values[4] > 16 || values[6] > 200 || values[10] > 6553 ||
        values[11] > 6553 || values[12] > 200 || values[13] > 150) return false;
    // Observed Q6 variants have optional fields before the firmware version,
    // followed by four one-byte status/check bytes. Reject truncated/collided
    // tails without assuming every model has exactly the same frame length.
    if (parts.size() < 19) return false;
    const size_t version_index = parts.size() - 5;
    const auto &version = parts[version_index];
    if (version_index < 14 || version.size() != 7 || version[0] != 'U' || version[2] != 'V') return false;
    for (size_t i : {size_t(1), size_t(3), size_t(4), size_t(5), size_t(6)})
      if (version[i] < '0' || version[i] > '9') return false;
    for (size_t i = version_index + 1; i < parts.size(); i++) if (parts[i].size() != 1) return false;
    grid_voltage = values[0]; grid_frequency = values[1];
    out_voltage = values[2]; out_frequency = values[3];
    battery_voltage = values[4] * 4; battery_charge_current = values[6];
    power_volt_ampere = values[10] * 10; power_watt = values[11] * 10;
    load_percentage = values[12]; temperature = values[13];
    return true;
  }

  State state_ = State::RECOVERING;
  bool waiting_status_ = false, status_requested_ = false, has_status_ = false, discard_frame_ = false;
  uint32_t sent_at_ = 0, last_rx_at_ = 0, recovery_at_ = 0, last_status_at_ = 0;
  std::vector<uint8_t> rx_;
  std::deque<Request> commands_;
  Request active_;
  std::vector<std::function<void()>> status_callbacks_;
};

}  // namespace rp6000
}  // namespace esphome
