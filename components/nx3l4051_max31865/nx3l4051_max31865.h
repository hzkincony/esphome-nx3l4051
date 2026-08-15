#pragma once

#include "driver/spi_master.h"

#include "esphome/components/nx3l4051/nx3l4051.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace nx3l4051_max31865 {

enum NX3L4051MAX31865ConfigFilter {
  FILTER_60HZ = 0,
  FILTER_50HZ = 1,
};

class NX3L4051MAX31865Sensor : public sensor::Sensor, public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  void set_mux(nx3l4051::NX3L4051Component *mux) { this->mux_ = mux; }
  void set_clk_pin(GPIOPin *pin) { this->clk_pin_ = pin; }
  void set_mosi_pin(GPIOPin *pin) { this->mosi_pin_ = pin; }
  void set_miso_pin(GPIOPin *pin) { this->miso_pin_ = pin; }
  void set_cs_pin_direct(GPIOPin *pin) { this->cs_pin_ = pin; }
  void set_reference_resistance(float reference_resistance) { this->reference_resistance_ = reference_resistance; }
  void set_nominal_resistance(float nominal_resistance) { this->rtd_nominal_resistance_ = nominal_resistance; }
  void set_filter(NX3L4051MAX31865ConfigFilter filter) { this->filter_ = filter; }
  void set_num_rtd_wires(uint8_t rtd_wires) { this->rtd_wires_ = rtd_wires; }

 protected:
  void read_data_keep_bias_();
  void log_faults_(uint8_t faults);
  void begin_transaction_();
  void end_transaction_();
  void write_register_transaction_(uint8_t reg, uint8_t value);
  uint8_t read_register_transaction_(uint8_t reg);
  uint16_t read_register_16_transaction_(uint8_t reg);
  float calc_temperature_(float rtd_ratio);

  nx3l4051::NX3L4051Component *mux_{nullptr};
  GPIOPin *clk_pin_{nullptr};
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *miso_pin_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  spi_device_handle_t spi_device_{nullptr};
  float reference_resistance_{0.0f};
  float rtd_nominal_resistance_{0.0f};
  NX3L4051MAX31865ConfigFilter filter_{FILTER_60HZ};
  uint8_t rtd_wires_{4};
  uint8_t base_config_{0};
  bool has_fault_{false};
  bool has_warn_{false};
};

}  // namespace nx3l4051_max31865
}  // namespace esphome
