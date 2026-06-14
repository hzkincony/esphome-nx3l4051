#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace nx3l4051 {

class NX3L4051Component : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_s1(GPIOPin *pin) { this->select_pins_[0].set_pin(pin); }
  void set_s1(bool fixed_level) { this->select_pins_[0].set_fixed_level(fixed_level); }
  void set_s2(GPIOPin *pin) { this->select_pins_[1].set_pin(pin); }
  void set_s2(bool fixed_level) { this->select_pins_[1].set_fixed_level(fixed_level); }
  void set_s3(GPIOPin *pin) { this->select_pins_[2].set_pin(pin); }
  void set_s3(bool fixed_level) { this->select_pins_[2].set_fixed_level(fixed_level); }
  void set_enable_pin(GPIOPin *pin) { this->enable_pin_ = pin; }
  void set_delay(uint32_t delay_ms) { this->delay_ms_ = delay_ms; }

  void select_channel(uint8_t channel);
  void disable();

 protected:
  class SelectPin {
   public:
    void set_pin(GPIOPin *pin) {
      this->pin_ = pin;
      this->has_fixed_level_ = false;
    }
    void set_fixed_level(bool fixed_level) {
      this->pin_ = nullptr;
      this->fixed_level_ = fixed_level;
      this->has_fixed_level_ = true;
    }
    void setup();
    void write(bool level);
    bool is_fixed() const { return this->has_fixed_level_; }
    bool fixed_level() const { return this->fixed_level_; }
    bool has_pin() const { return this->pin_ != nullptr; }
    GPIOPin *pin() const { return this->pin_; }

   protected:
    GPIOPin *pin_{nullptr};
    bool has_fixed_level_{false};
    bool fixed_level_{false};
  };

  SelectPin select_pins_[3];
  GPIOPin *enable_pin_{nullptr};
  uint32_t delay_ms_{20};
  uint8_t current_channel_{0};
};

template<typename... Ts> class SelectAction : public Action<Ts...> {
 public:
  explicit SelectAction(NX3L4051Component *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint8_t, channel)

  void play(const Ts &...x) override {
    const uint8_t channel = this->channel_.value(x...);
    this->parent_->select_channel(channel);
  }

 protected:
  NX3L4051Component *parent_;
};

template<typename... Ts> class DisableAction : public Action<Ts...> {
 public:
  explicit DisableAction(NX3L4051Component *parent) : parent_(parent) {}

  void play(const Ts &...x) override { this->parent_->disable(); }

 protected:
  NX3L4051Component *parent_;
};

}  // namespace nx3l4051
}  // namespace esphome
