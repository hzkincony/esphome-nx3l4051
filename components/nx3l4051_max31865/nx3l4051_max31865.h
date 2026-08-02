#pragma once

#include "driver/spi_master.h"

#include "esphome/components/max31865/max31865.h"
#include "esphome/components/nx3l4051/nx3l4051.h"

namespace esphome {
namespace nx3l4051_max31865 {

class NX3L4051MAX31865Sensor : public max31865::MAX31865Sensor {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  void set_mux(nx3l4051::NX3L4051Component *mux) { this->mux_ = mux; }
  void set_clk_pin(GPIOPin *pin) { this->clk_pin_ = pin; }
  void set_mosi_pin(GPIOPin *pin) { this->mosi_pin_ = pin; }
  void set_miso_pin(GPIOPin *pin) { this->miso_pin_ = pin; }
  void set_cs_pin_direct(GPIOPin *pin) { this->cs_pin_ = pin; }

 protected:
  void read_data_keep_bias_();
  void log_faults_(uint8_t faults);
  void begin_transaction_();
  void end_transaction_();
  void write_register_transaction_(uint8_t reg, uint8_t value);
  uint8_t read_register_transaction_(uint8_t reg);
  uint16_t read_register_16_transaction_(uint8_t reg);

  nx3l4051::NX3L4051Component *mux_{nullptr};
  GPIOPin *clk_pin_{nullptr};
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *miso_pin_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  spi_device_handle_t spi_device_{nullptr};
};

}  // namespace nx3l4051_max31865
}  // namespace esphome
