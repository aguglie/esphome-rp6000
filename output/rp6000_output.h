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
      void set_value(float value) { this->write_state(value); };

    protected:
      void write_state(float state) override
      {
        char tmp[10];
        sprintf(tmp, this->set_command_.c_str(), state);

        if (std::find(this->possible_values_.begin(), this->possible_values_.end(), state) != this->possible_values_.end())
        {
          ESP_LOGD(TAG, "Will write: %s out of value %f / %02.0f", tmp, state, state);
          this->parent_->write_line(tmp);
        }
        else
        {
          ESP_LOGD(TAG, "Will not write: %s as it is not in list of allowed values", tmp);
        }
      }
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
