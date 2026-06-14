#include "nx3l4051.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nx3l4051 {

static const char *const TAG = "nx3l4051";

void NX3L4051Component::SelectPin::setup() {
  if (this->pin_ != nullptr) {
    this->pin_->setup();
  }
}

void NX3L4051Component::SelectPin::write(bool level) {
  if (this->pin_ != nullptr) {
    this->pin_->digital_write(level);
  }
}

void NX3L4051Component::setup() {
  for (auto &select_pin : this->select_pins_) {
    select_pin.setup();
  }

  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(false);
  }

  this->select_channel(0);
}

void NX3L4051Component::dump_config() {
  ESP_LOGCONFIG(TAG, "NX3L4051:");
  for (uint8_t i = 0; i < 3; i++) {
    if (this->select_pins_[i].has_pin()) {
      LOG_PIN("  S", this->select_pins_[i].pin());
    } else if (this->select_pins_[i].is_fixed()) {
      ESP_LOGCONFIG(TAG, "  S%u: fixed %s", i + 1, this->select_pins_[i].fixed_level() ? "HIGH" : "LOW");
    }
  }
  LOG_PIN("  Enable Pin: ", this->enable_pin_);
  ESP_LOGCONFIG(TAG, "  Delay: %" PRIu32 " ms", this->delay_ms_);
}

void NX3L4051Component::select_channel(uint8_t channel) {
  channel &= 0x07;
  uint8_t actual_channel = channel;

  for (uint8_t bit = 0; bit < 3; bit++) {
    const bool requested = (channel >> bit) & 0x01;
    auto &select_pin = this->select_pins_[bit];

    if (select_pin.is_fixed()) {
      if (select_pin.fixed_level() != requested) {
        ESP_LOGW(TAG, "Requested channel %u conflicts with fixed S%u=%s", channel, bit + 1,
                 select_pin.fixed_level() ? "HIGH" : "LOW");
      }
      if (select_pin.fixed_level()) {
        actual_channel |= 1 << bit;
      } else {
        actual_channel &= ~(1 << bit);
      }
      continue;
    }

    select_pin.write(requested);
  }

  this->current_channel_ = actual_channel;
  if (this->delay_ms_ > 0) {
    delay(this->delay_ms_);
  }
}

void NX3L4051Component::disable() {
  if (this->enable_pin_ == nullptr) {
    ESP_LOGV(TAG, "disable requested but enable_pin is not configured");
    return;
  }
  this->enable_pin_->digital_write(true);
}

}  // namespace nx3l4051
}  // namespace esphome
