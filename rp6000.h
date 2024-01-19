#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#define TAG "RP6000"

namespace esphome {
namespace rp6000 {

class RP6000 : public Component, public uart::UARTDevice {
public:
    void setup() override
    {
        pinMode(5, OUTPUT);
        digitalWrite(5, LOW);
    }

    void updateStatus()
    {
        ESP_LOGD(TAG, "Requesting status from RP6000");
        digitalWrite(5, HIGH);
        write_str("Q6\r\n");
        this->flush();
        digitalWrite(5, LOW);

        std::vector<uint8_t> inverter_reply;
        if (!read_line(inverter_reply))
        {
            return;
        }

        ESP_LOGD(TAG, "Got status reply from inverter: %s", format_hex_pretty(inverter_reply).c_str());

        inverter_reply.erase(inverter_reply.begin());

        std::vector<std::vector<uint8_t>> parts;
        if (!split_line(inverter_reply, parts))
        {
            ESP_LOGW(TAG, "Failed to split line into parts");
            return;
        }

        if (parts.size() < 14)
        {
            ESP_LOGE(TAG, "Invalid reply from inverter");
            return;
        }

        if (!cast_to_uint16(parts[0], grid_voltage))
        {
            ESP_LOGW(TAG, "Failed to cast voltage to uint16");
            return;
        }
        ESP_LOGD(TAG, "Grid voltage: %d", grid_voltage);
        this->grid_voltage = grid_voltage;

        if (!cast_to_float(parts[1], grid_frequency))
        {
            ESP_LOGW(TAG, "Failed to cast frequency to uint16");
            return;
        }
        ESP_LOGD(TAG, "Grid frequency: %f", grid_frequency);

        if (!cast_to_uint16(parts[2], out_voltage))
        {
            ESP_LOGW(TAG, "Failed to cast out voltage to uint16");
            return;
        }
        ESP_LOGD(TAG, "Out voltage: %d", out_voltage);

        if (!cast_to_float(parts[3], out_frequency))
        {
            ESP_LOGW(TAG, "Failed to cast out frequency to float");
            return;
        }
        ESP_LOGD(TAG, "Out frequency: %f", out_frequency);

        if (!cast_to_float(parts[4], battery_voltage))
        {
            ESP_LOGW(TAG, "Failed to cast battery voltage to float");
            return;
        }
        battery_voltage = battery_voltage * 4;
        ESP_LOGD(TAG, "Battery voltage: %f", battery_voltage);

        if (!cast_to_float(parts[6], battery_charge_current))
        {
            ESP_LOGW(TAG, "Failed to cast battery charge current to float");
            return;
        }
        ESP_LOGD(TAG, "Battery charge current: %f", battery_charge_current);

        if (!cast_to_uint16(parts[10], power_volt_ampere))
        {
            ESP_LOGW(TAG, "Failed to cast power volt ampere to uint16");
            return;
        }
        power_volt_ampere = power_volt_ampere * 10;
        ESP_LOGD(TAG, "Power volt ampere: %d", power_volt_ampere);

        if (!cast_to_uint16(parts[11], power_watt))
        {
            ESP_LOGW(TAG, "Failed to cast power watt to uint16");
            return;
        }
        power_watt = power_watt * 10;
        ESP_LOGD(TAG, "Power watt: %d", power_watt);

        if (!cast_to_uint16(parts[12], load_percentage))
        {
            ESP_LOGW(TAG, "Failed to cast load percentage to uint16");
            return;
        }
        ESP_LOGD(TAG, "Load percentage: %d", load_percentage);

        if (!cast_to_float(parts[13], temperature))
        {
            ESP_LOGW(TAG, "Failed to cast temperature to float");
            return;
        }
        ESP_LOGD(TAG, "Temperature: %f", temperature);
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

private:
    bool read_line(std::vector<uint8_t> &reply, uint16_t timeout = 100)
    {
        auto start_at = millis();
        while (millis() - start_at < timeout && !this->available())
        {
        }

        if (!this->available())
        {
            ESP_LOGW(TAG, "Timeout while waiting for data");
            return false;
        }

        uint8_t data;

        while (this->available() > 0)
        {
            this->read_byte(&data);

            if (data == '\n' || data == '\r')
            {

                return true;
            }

            reply.push_back(data);
        }

        ESP_LOGW(TAG, "Invalid data read from inverter: %s", format_hex_pretty(reply).c_str());

        reply.clear();
        return false;
    }

    bool split_line(std::vector<uint8_t> &reply, std::vector<std::vector<uint8_t>> &parts, char delimiter = ' ')
    {
        parts.clear();

        std::vector<uint8_t> part;
        for (auto &data : reply)
        {
            if (data == delimiter)
            {
                parts.push_back(part);
                part.clear();
                continue;
            }

            part.push_back(data);
        }

        if (!part.empty())
        {
            parts.push_back(part);
        }

        return true;
    }

    bool cast_to_uint16(std::vector<uint8_t> &data, uint16_t &value)
    {
        value = 0;

        for (auto &c : data)
        {
            if (c < '0' || c > '9')
            {
                return false;
            }

            value *= 10;
            value += c - '0';
        }

        return true;
    }

    bool cast_to_float(std::vector<uint8_t> &data, float &value)
    {
        value = 0;

        bool decimal = false;
        float decimal_place = 0.1;

        for (auto &c : data)
        {
            if (c == '.')
            {
                if (decimal)
                {
                    return false;
                }

                decimal = true;
                continue;
            }

            if (c < '0' || c > '9')
            {
                return false;
            }

            if (decimal)
            {
                value += (c - '0') * decimal_place;
                decimal_place *= 0.1;
            }
            else
            {
                value *= 10;
                value += c - '0';
            }
        }

        return true;
    }
};

} // namespace rp6000
} // namespace esphome