#pragma once

#include "../rp6000.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/component.h"

#include <vector>

namespace esphome
{
  namespace rp6000
  {
    class RP6000Output : public Component, public output::FloatOutput
    {
    public:
      void set_parent(RP6000 *parent) { this->parent_ = parent; }
      void set_set_command(const std::string &command) { this->set_command_ = command; };
      void set_possible_values(std::vector<float> possible_values) { this->possible_values_ = std::move(possible_values); }
      void set_value(float value) { this->write_state(value); }
      void set_ack_callback(std::function<void(float)> callback) { ack_callback_ = std::move(callback); }
      bool is_pending() const { return pending_; }
      float get_confirmed_value() const { return confirmed_value_; }

    protected:
      void write_state(float state) override {
        if (!std::isfinite(state) || std::find(possible_values_.begin(), possible_values_.end(), state) == possible_values_.end()) {
          ESP_LOGE(TAG, "Invalid output value: %f", state);
          return;
        }
        char command[32];
        const int length = snprintf(command, sizeof(command), set_command_.c_str(), state);
        if (length < 0 || static_cast<size_t>(length) >= sizeof(command)) return;
        const uint32_t generation = ++generation_;
        pending_ = true;
        if (!parent_->queue_command(set_command_, command, [this, state, generation](bool accepted) {
          if (generation == generation_) pending_ = false;
          if (accepted) {
            confirmed_value_ = state;
            if (ack_callback_) ack_callback_(state);
          } else {
            ESP_LOGW(TAG, "Output value %.0f was not confirmed", state);
          }
        })) pending_ = false;
      }
      std::function<void(float)> ack_callback_;
      float confirmed_value_ = NAN;
      bool pending_ = false;
      uint32_t generation_ = 0;
      std::string set_command_;
      RP6000 *parent_;
      std::vector<float> possible_values_;
    };

    template <typename... Ts>
    class SetOutputAction : public Action<Ts...>
    {
    public:
      SetOutputAction(RP6000Output *output) : output_(output) {}

      TEMPLATABLE_VALUE(float, level)

      void play(Ts... x) override { this->output_->set_value(this->level_.value(x...)); }

    protected:
      RP6000Output *output_;
    };

  } // namespace rp6000
} // namespace esphome
