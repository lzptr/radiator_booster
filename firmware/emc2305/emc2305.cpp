#include "emc2305.h"
#include "esphome/core/log.h"

namespace esphome {
namespace emc2305 {

static const char *const TAG = "emc2305";

// EMC2305Fan implementation
EMC2305Fan::EMC2305Fan(EMC2305Component *parent, uint8_t fan_number) 
    : parent_(parent), fan_number_(fan_number) {}

void EMC2305Fan::write_state(float state) {
  this->parent_->set_fan_duty(this->fan_number_, state);
}

void EMC2305Fan::update_rpm(float rpm) {
  if (this->rpm_sensor_ != nullptr) {
    this->rpm_sensor_->publish_state(rpm);
  }
}

// EMC2305Component implementation
void EMC2305Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up EMC2305...");
  
  // Check if we can communicate with the right chip
  uint8_t chip_id;
  if (!this->read_byte(REG_PRODUCT_ID, &chip_id)) {
    ESP_LOGE(TAG, "Failed to read chip ID!");
    this->mark_failed();
    return;
  }
  
  if (chip_id != EMC2305_CHIP_ID) {
    ESP_LOGE(TAG, "Wrong chip ID! Expected: 0x%02X, got: 0x%02X", EMC2305_CHIP_ID, chip_id);
    this->mark_failed();
    return;
  }

  // Configure base device settings
  uint8_t config = CONFIG_DIS_TO;  // Disable timeout for I2C compatibility
  if (!this->write_byte(REG_CONFIG, config)) {
    this->mark_failed();
    return;
  }
  
  // Set all PWM outputs to push-pull mode (all bits 1)
  if (!this->write_byte(REG_PWM_OUTPUT, 0x1F)) {
    this->mark_failed();
    return;
  }

  // Set 25kHz PWM frequency base for all fans
  if (!this->write_byte(REG_PWM_BASE, PWM_FREQ_25KHZ)) {
    this->mark_failed();
    return;
  }

  // Initialize each configured fan
  for (uint8_t i = 0; i < 5; i++) {
    if (this->fans_[i] != nullptr) {
      uint8_t fan_num = i + 1;
      uint8_t base_reg = get_fan_reg_base_(fan_num);
      
      // Set PWM divide to default (register offset 0x01)
      if (!write_fan_config_(fan_num, 0x01, PWM_DIVIDE_DEFAULT)) {
        this->mark_failed();
        return;
      }

      // Configure fan settings (register offset 0x02)
      // - Enable TACH input (EDGx1:0 = 01)
      // - Set moderate update time (UDTx2:0 = 010)
      uint8_t fan_config = (0b01 << 3) | (0b010);
      if (!write_fan_config_(fan_num, 0x02, fan_config)) {
        this->mark_failed();
        return;
      }

      // Start with 0% duty cycle
      if (!write_fan_config_(fan_num, 0x00, 0)) {
        this->mark_failed();
        return;
      }
    }
  }
}

void EMC2305Component::dump_config() {
  ESP_LOGCONFIG(TAG, "EMC2305:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication failed!");
    return;
  }
  
  for (uint8_t i = 0; i < 5; i++) {
    if (this->fans_[i] != nullptr) {
      ESP_LOGCONFIG(TAG, "  Fan %d:", i + 1);
      ESP_LOGCONFIG(TAG, "    PWM Setting: %d%%", 
        (read_fan_config_(i + 1, 0x00) * 100) / 255);
    }
  }
}

EMC2305Fan *EMC2305Component::create_fan(uint8_t fan_number) {
  if (fan_number < 1 || fan_number > 5) {
    ESP_LOGE(TAG, "Invalid fan number: %d", fan_number);
    return nullptr;
  }
  
  auto *fan = new EMC2305Fan(this, fan_number);
  this->fans_[fan_number - 1] = fan;
  return fan;
}

void EMC2305Component::update() {
  for (uint8_t i = 0; i < 5; i++) {
    if (this->fans_[i] != nullptr) {
      float rpm = this->get_fan_rpm(i + 1);
      this->fans_[i]->update_rpm(rpm);
    }
  }
}

bool EMC2305Component::write_fan_config_(uint8_t fan, uint8_t reg_offset, uint8_t value) {
  uint8_t reg = get_fan_reg_base_(fan) + reg_offset;
  return this->write_byte(reg, value);
}

uint8_t EMC2305Component::read_fan_config_(uint8_t fan, uint8_t reg_offset) {
  uint8_t reg = get_fan_reg_base_(fan) + reg_offset;
  uint8_t value;
  if (!this->read_byte(reg, &value)) {
    ESP_LOGE(TAG, "Failed to read fan config register 0x%02X", reg);
    return 0;
  }
  return value;
}

void EMC2305Component::set_fan_duty(uint8_t fan, float duty) {
  if (fan < 1 || fan > 5) return;
  
  uint8_t pwm = duty * 255;
  if (!write_fan_config_(fan, 0x00, pwm)) {
    ESP_LOGE(TAG, "Failed to set fan %d duty cycle", fan);
    this->status_set_warning();
  }
}

float EMC2305Component::get_fan_rpm(uint8_t fan) {
  if (fan < 1 || fan > 5) return 0;

  uint8_t base_reg = get_fan_reg_base_(fan);
  uint8_t high, low;
  
  // Read TACH value (offset 0x0E for high byte, 0x0F for low byte)
  if (!this->read_byte(base_reg + 0x0E, &high) || 
      !this->read_byte(base_reg + 0x0F, &low)) {
    ESP_LOGE(TAG, "Failed to read fan %d RPM", fan);
    this->status_set_warning();
    return 0;
  }

  uint16_t tach = (high << 8) | low;
  if (tach == 0xFFFF) return 0;
  
  // Convert TACH to RPM
  // For Noctua fans: 2 pulses per revolution
  // Formula: RPM = (60 * frequency) / pulses_per_rev
  // frequency = clock / tach = 32768 / tach
  return (60.0f * 32768.0f) / (2.0f * tach);
}

}  // namespace emc2305
}  // namespace esphome