#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace emc2305 {

// Default I2C address for 10k pull-up (0101_100x)
static const uint8_t EMC2305_I2C_ADDR = 0x2C;

class EMC2305Fan : public output::FloatOutput {
 public:
  EMC2305Fan(EMC2305Component *parent, uint8_t fan_number);
  void set_rpm_sensor(sensor::Sensor *sensor) { rpm_sensor_ = sensor; }
  void update_rpm(float rpm);
  
 protected:
  void write_state(float state) override;
  EMC2305Component *parent_;
  uint8_t fan_number_;
  sensor::Sensor *rpm_sensor_{nullptr};
  friend class EMC2305Component;
};

class EMC2305Component : public PollingComponent, public i2c::I2CDevice {
 public:
  EMC2305Component() = default;

  void setup() override;
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  EMC2305Fan *create_fan(uint8_t fan_number);
  void set_fan_duty(uint8_t fan, float duty);
  float get_fan_rpm(uint8_t fan);

 protected:
  EMC2305Fan *fans_[5]{nullptr};
  static const char *const TAG;

  // Register addresses
  enum RegisterMap {
    REG_CONFIG = 0x20,
    REG_FAN_STATUS = 0x24,
    REG_FAN_STALL = 0x25,
    REG_FAN_SPIN = 0x26,
    REG_DRIVE_FAIL = 0x27,
    REG_FAN_INTERRUPT_ENABLE = 0x29,
    REG_PWM_POLARITY = 0x2A,
    REG_PWM_OUTPUT = 0x2B,
    REG_PWM_BASE = 0x2D,
    REG_PRODUCT_ID = 0xFD,
  };

  // Configuration bits
  enum ConfigBits {
    CONFIG_MASK = (1 << 7),
    CONFIG_DIS_TO = (1 << 6),  // Disable timeout
    CONFIG_WD_EN = (1 << 5),   // Watchdog enable
    CONFIG_DRECK = (1 << 1),   // Drive external clock
    CONFIG_USECK = (1 << 0),   // Use external clock
  };

  // Per-fan register offsets
  static const uint8_t FAN_SETTINGS_OFFSET = 0x30;
  static const uint8_t FAN_REGISTER_SPACING = 0x10;

  // Product and configuration validation
  static const uint8_t EMC2305_CHIP_ID = 0x34;    // Product ID for EMC2305
  static const uint8_t PWM_FREQ_25KHZ = 0x00;     // 26kHz base frequency setting
  static constexpr uint8_t PWM_DIVIDE_DEFAULT = 1; // Default PWM frequency divider

  uint8_t get_fan_reg_base_(uint8_t fan) {
    return FAN_SETTINGS_OFFSET + ((fan - 1) * FAN_REGISTER_SPACING);
  }

  bool write_fan_config_(uint8_t fan, uint8_t reg_offset, uint8_t value);
  uint8_t read_fan_config_(uint8_t fan, uint8_t reg_offset);
};

}  // namespace emc2305
}  // namespace esphome