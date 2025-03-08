#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace emc2305 {

static const uint8_t EMC2305_I2C_ADDR = 0x2C;

class EMC2305Output : public output::FloatOutput {
 public:
  EMC2305Output(class EMC2305Component *parent, uint8_t fan_number) 
      : parent_(parent), fan_number_(fan_number) {}
  
 protected:
  void write_state(float state) override;
  class EMC2305Component *parent_;
  uint8_t fan_number_;
};

class EMC2305Component : public Component, public i2c::I2CDevice {
 public:
  EMC2305Component() = default;

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_pwm_duty(uint8_t fan_number, float duty);
  EMC2305Output *create_fan_output(uint8_t fan_number);

 protected:
  EMC2305Output *outputs_[5]{nullptr};

  // Register addresses
  static const uint8_t REG_CONFIG = 0x20;
  static const uint8_t REG_PWM_OUTPUT = 0x2B;
  static const uint8_t REG_PWM_BASE = 0x2D;
  static const uint8_t REG_PRODUCT_ID = 0xFD;
  static const uint8_t FAN_BASE_ADDR = 0x30;
  static const uint8_t FAN_SPACING = 0x10;

  // Chip configuration
  static const uint8_t EMC2305_CHIP_ID = 0x34;
  static const uint8_t PWM_FREQ_25KHZ = 0x00;
  static const uint8_t CONFIG_DIS_TO = (1 << 6);

  uint8_t get_fan_reg_(uint8_t fan_number) {
    return FAN_BASE_ADDR + ((fan_number - 1) * FAN_SPACING);
  }
};

}  // namespace emc2305
}  // namespace esphome