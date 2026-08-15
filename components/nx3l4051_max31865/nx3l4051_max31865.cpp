#include "nx3l4051_max31865.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <cmath>

namespace esphome {
namespace nx3l4051_max31865 {

static const char *const TAG = "nx3l4051_max31865";

static const uint8_t SPI_WRITE_MASK = 0x80;
static const uint8_t CONFIGURATION_REG = 0x00;
static const uint8_t RTD_RESISTANCE_MSB_REG = 0x01;
static const uint8_t FAULT_STATUS_REG = 0x07;
static const uint8_t CONFIG_VBIAS = 1 << 7;
static const uint8_t CONFIG_1SHOT = 1 << 5;
static const uint8_t CONFIG_3WIRE = 1 << 4;
static const uint8_t CONFIG_FAULT_CLEAR = 1 << 1;

void NX3L4051MAX31865Sensor::setup() {
  this->cs_pin_->setup();
  this->cs_pin_->digital_write(true);

  spi_bus_config_t bus_config{};
  bus_config.mosi_io_num = spi::Utility::get_pin_no(this->mosi_pin_);
  bus_config.miso_io_num = spi::Utility::get_pin_no(this->miso_pin_);
  bus_config.sclk_io_num = spi::Utility::get_pin_no(this->clk_pin_);
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  esp_err_t err = spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SPI3_HOST: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  spi_device_interface_config_t device_config{};
  device_config.clock_speed_hz = 1000000;
  device_config.mode = 1;
  device_config.spics_io_num = -1;
  device_config.queue_size = 1;
  err = spi_bus_add_device(SPI3_HOST, &device_config, &this->spi_device_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add MAX31865 on SPI3_HOST: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  // Match the MAX31865 initialization sequence by reading the configuration before its first write.
  (void) this->read_register_transaction_(CONFIGURATION_REG);
  this->base_config_ = static_cast<uint8_t>(this->filter_) & 1;
  if (this->rtd_wires_ == 3) {
    this->base_config_ |= CONFIG_3WIRE;
  }
  this->write_register_transaction_(CONFIGURATION_REG,
                                    this->base_config_ | CONFIG_VBIAS | CONFIG_FAULT_CLEAR);
}

void NX3L4051MAX31865Sensor::dump_config() {
  LOG_SENSOR("", "NX3L4051 MAX31865", this);
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
  LOG_PIN("  MOSI Pin: ", this->mosi_pin_);
  LOG_PIN("  MISO Pin: ", this->miso_pin_);
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG,
                "  Reference Resistance: %.2fΩ\n"
                "  RTD: %u-wire %.2fΩ\n"
                "  Mains Filter: %s\n"
                "  SPI: hardware mode 1 at 1 MHz\n"
                "  VBIAS: always on\n"
                "  Automatic fault detection: disabled",
                this->reference_resistance_, this->rtd_wires_, this->rtd_nominal_resistance_,
                this->filter_ == FILTER_60HZ ? "60 Hz" : "50 Hz");
}

void NX3L4051MAX31865Sensor::update() {
  const uint8_t faults = this->read_register_transaction_(FAULT_STATUS_REG);
  if (faults != 0) {
    this->write_register_transaction_(CONFIGURATION_REG,
                                      this->base_config_ | CONFIG_VBIAS | CONFIG_FAULT_CLEAR);
  }

  this->write_register_transaction_(CONFIGURATION_REG,
                                    this->base_config_ | CONFIG_VBIAS | CONFIG_1SHOT);
  this->set_timeout("value", this->filter_ == FILTER_60HZ ? 55 : 66,
                    [this]() { this->read_data_keep_bias_(); });
}

void NX3L4051MAX31865Sensor::read_data_keep_bias_() {
  const uint16_t rtd_register =
      this->read_register_16_transaction_(RTD_RESISTANCE_MSB_REG);
  const uint8_t faults = this->read_register_transaction_(FAULT_STATUS_REG);
  const uint8_t channel = this->mux_ == nullptr ? 0 : this->mux_->get_current_channel();
  const float resistance = static_cast<float>(rtd_register >> 1) * this->reference_resistance_ / 32768.0f;
  ESP_LOGD(TAG, "Channel %u: raw=0x%04X, resistance=%.2fΩ, faults=0x%02X", channel, rtd_register, resistance,
           faults);

  if (rtd_register == 0x0000 || rtd_register == 0xFFFF) {
    ESP_LOGE(TAG, "Channel %u RTD register read 0x%04X", channel, rtd_register);
    this->publish_state(NAN);
    this->status_set_error();
    return;
  }

  this->has_fault_ = (rtd_register & 0x0001) || (faults & 0b00111100);
  this->has_warn_ = faults & 0b11000000;

  if (this->has_fault_) {
    this->log_faults_(faults);
    this->publish_state(NAN);
    this->status_set_error();
    return;
  }

  if (this->has_warn_) {
    this->log_faults_(faults);
    this->status_set_warning();
  } else {
    this->status_clear_warning();
  }
  this->status_clear_error();

  const float rtd_ratio = static_cast<float>(rtd_register >> 1) / static_cast<float>((1 << 15) - 1);
  const float temperature = this->calc_temperature_(rtd_ratio);
  ESP_LOGD(TAG, "RTD %.2fΩ, temperature %.2f°C", this->reference_resistance_ * rtd_ratio, temperature);
  this->publish_state(temperature);
}

void NX3L4051MAX31865Sensor::begin_transaction_() {
  this->cs_pin_->digital_write(false);
  delay_microseconds_safe(2);
}

void NX3L4051MAX31865Sensor::end_transaction_() {
  delay_microseconds_safe(2);
  this->cs_pin_->digital_write(true);
}

void NX3L4051MAX31865Sensor::write_register_transaction_(uint8_t reg, uint8_t value) {
  spi_transaction_t transaction{};
  transaction.flags = SPI_TRANS_USE_TXDATA;
  transaction.length = 16;
  transaction.tx_data[0] = reg | SPI_WRITE_MASK;
  transaction.tx_data[1] = value;
  this->begin_transaction_();
  const esp_err_t err = spi_device_transmit(this->spi_device_, &transaction);
  this->end_transaction_();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "MAX31865 register write failed: %s", esp_err_to_name(err));
  }
}

uint8_t NX3L4051MAX31865Sensor::read_register_transaction_(uint8_t reg) {
  spi_transaction_t transaction{};
  transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  transaction.length = 16;
  transaction.tx_data[0] = reg;
  this->begin_transaction_();
  const esp_err_t err = spi_device_transmit(this->spi_device_, &transaction);
  this->end_transaction_();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "MAX31865 register read failed: %s", esp_err_to_name(err));
    return 0xFF;
  }
  return transaction.rx_data[1];
}

uint16_t NX3L4051MAX31865Sensor::read_register_16_transaction_(uint8_t reg) {
  spi_transaction_t transaction{};
  transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  transaction.length = 24;
  transaction.tx_data[0] = reg;
  this->begin_transaction_();
  const esp_err_t err = spi_device_transmit(this->spi_device_, &transaction);
  this->end_transaction_();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "MAX31865 register read failed: %s", esp_err_to_name(err));
    return 0xFFFF;
  }
  return (static_cast<uint16_t>(transaction.rx_data[1]) << 8) | transaction.rx_data[2];
}

float NX3L4051MAX31865Sensor::calc_temperature_(float rtd_ratio) {
  const float a = 3.9083e-3f;
  const float b = -5.775e-7f;
  const float z1 = -a;
  const float z2 = a * a - 4 * b;
  const float z3 = 4 * b / this->rtd_nominal_resistance_;
  const float z4 = 2 * b;

  float rtd_resistance = rtd_ratio * this->reference_resistance_;
  const float pos_temp = (z1 + std::sqrt(z2 + (z3 * rtd_resistance))) / z4;
  if (pos_temp >= 0) {
    return pos_temp;
  }

  if (this->rtd_nominal_resistance_ != 100) {
    rtd_resistance /= this->rtd_nominal_resistance_;
    rtd_resistance *= 100;
  }
  float rpoly = rtd_resistance;
  float neg_temp = -242.02f;
  neg_temp += 2.2228f * rpoly;
  rpoly *= rtd_resistance;
  neg_temp += 2.5859e-3f * rpoly;
  rpoly *= rtd_resistance;
  neg_temp -= 4.8260e-6f * rpoly;
  rpoly *= rtd_resistance;
  neg_temp -= 2.8183e-8f * rpoly;
  rpoly *= rtd_resistance;
  neg_temp += 1.5243e-10f * rpoly;
  return neg_temp;
}

void NX3L4051MAX31865Sensor::log_faults_(uint8_t faults) {
  if (faults & (1 << 2)) {
    ESP_LOGE(TAG, "Overvoltage/undervoltage fault");
  }
  if (faults & (1 << 3)) {
    ESP_LOGE(TAG, "RTDIN- < 0.85 x V_BIAS (FORCE- open)");
  }
  if (faults & (1 << 4)) {
    ESP_LOGE(TAG, "REFIN- < 0.85 x V_BIAS (FORCE- open)");
  }
  if (faults & (1 << 5)) {
    ESP_LOGE(TAG, "REFIN- > 0.85 x V_BIAS");
  }
  if (faults & (1 << 6)) {
    ESP_LOGW(TAG, "RTD low threshold");
  }
  if (faults & (1 << 7)) {
    ESP_LOGW(TAG, "RTD high threshold");
  }
}

}  // namespace nx3l4051_max31865
}  // namespace esphome
