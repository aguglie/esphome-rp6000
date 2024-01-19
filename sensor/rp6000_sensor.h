#pragma once

#include "esphome/core/component.h"
#include "../rp6000.h"

#define TAG "RP6000_Sensor"

namespace esphome
{
    namespace rp6000
    {

        class RP6000Sensor : public PollingComponent
        {
        public:
            RP6000Sensor() : PollingComponent(5000)
            {
            }

            void update() override
            {
                this->parent->updateStatus();
                
                if (this->grid_voltage_sensor_ != nullptr){
                    this->grid_voltage_sensor_->publish_state(this->parent->grid_voltage);
                }
                if (this->grid_frequency_sensor_ != nullptr){
                    this->grid_frequency_sensor_->publish_state(this->parent->grid_frequency);
                }
                if (this->out_voltage_sensor_ != nullptr){
                    this->out_voltage_sensor_->publish_state(this->parent->out_voltage);
                }
                if (this->out_frequency_sensor_ != nullptr){
                    this->out_frequency_sensor_->publish_state(this->parent->out_frequency);
                }
                if (this->battery_voltage_sensor_ != nullptr){
                    this->battery_voltage_sensor_->publish_state(this->parent->battery_voltage);
                }
                if (this->battery_charge_current_sensor_ != nullptr){
                    this->battery_charge_current_sensor_->publish_state(this->parent->battery_charge_current);
                }
                if (this->power_volt_ampere_sensor_ != nullptr){
                    this->power_volt_ampere_sensor_->publish_state(this->parent->power_volt_ampere);
                }
                if (this->power_watt_sensor_ != nullptr){
                    this->power_watt_sensor_->publish_state(this->parent->power_watt);
                }
                if (this->load_percentage_sensor_ != nullptr){
                    this->load_percentage_sensor_->publish_state(this->parent->load_percentage);
                }
                if (this->temperature_sensor_ != nullptr){
                    this->temperature_sensor_->publish_state(this->parent->temperature);
                }
            
            }

            void set_parent(RP6000 *parent)
            {
                this->parent = parent;
            }

            void set_grid_voltage(sensor::Sensor *grid_voltage_sensor)
            {
                this->grid_voltage_sensor_ = grid_voltage_sensor;
            }

            void set_grid_frequency(sensor::Sensor *grid_frequency_sensor)
            {
                this->grid_frequency_sensor_ = grid_frequency_sensor;
            }

            void set_out_voltage(sensor::Sensor *out_voltage_sensor)
            {
                this->out_voltage_sensor_ = out_voltage_sensor;
            }

            void set_out_frequency(sensor::Sensor *out_frequency_sensor)
            {
                this->out_frequency_sensor_ = out_frequency_sensor;
            }

            void set_battery_voltage(sensor::Sensor *battery_voltage_sensor)
            {
                this->battery_voltage_sensor_ = battery_voltage_sensor;
            }

            void set_battery_charge_current(sensor::Sensor *battery_charge_current_sensor)
            {
                this->battery_charge_current_sensor_ = battery_charge_current_sensor;
            }

            void set_power_volt_ampere(sensor::Sensor *power_volt_ampere_sensor)
            {
                this->power_volt_ampere_sensor_ = power_volt_ampere_sensor;
            }

            void set_power_watt(sensor::Sensor *power_watt_sensor)
            {
                this->power_watt_sensor_ = power_watt_sensor;
            }

            void set_load_percentage(sensor::Sensor *load_percentage_sensor)
            {
                this->load_percentage_sensor_ = load_percentage_sensor;
            }

            void set_temperature(sensor::Sensor *temperature_sensor)
            {
                this->temperature_sensor_ = temperature_sensor;
            }

        private:
            sensor::Sensor *grid_voltage_sensor_{nullptr};
            sensor::Sensor *grid_frequency_sensor_{nullptr};
            sensor::Sensor *out_voltage_sensor_{nullptr};
            sensor::Sensor *out_frequency_sensor_{nullptr};
            sensor::Sensor *battery_voltage_sensor_{nullptr};
            sensor::Sensor *battery_charge_current_sensor_{nullptr};
            sensor::Sensor *power_volt_ampere_sensor_{nullptr};
            sensor::Sensor *power_watt_sensor_{nullptr};
            sensor::Sensor *load_percentage_sensor_{nullptr};
            sensor::Sensor *temperature_sensor_{nullptr};

            RP6000 *parent;
        };

    } // namespace rp6000
} // namespace esphome
