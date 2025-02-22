#include "emc2305.h"
#include "esphome/core/log.h"

namespace esphome {
namespace emc2305 {

static const char *const TAG = "emc2305";

void EMC2305Output::write_state(float state) {
  this->parent_->set_pwm_duty(this->fan_number_, state);
}

void EMC2305Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up EMC2305...");
  
  // Verify chip ID
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

  // Configure device
  uint8_t config = CONFIG_DIS_TO;  // Disable timeout for I2C compatibility
  if (!this->write_byte(REG_CONFIG, config)) {
    this->mark_failed();
    return;
  }
  
  // Set all PWM outputs to push-pull mode
  if (!this->write_byte(REG_PWM_OUTPUT, 0x1F)) {  // All 5 bits set
    this->mark_failed();
    return;
  }

  // Set 25kHz PWM frequency base for all fans
  if (!this->write_byte(REG_PWM_BASE, PWM_FREQ_25KHZ)) {
    this->mark_failed();
    return;
  }

  // Initialize all fans to 0%
  for (uint8_t i = 1; i <= 5; i++) {
    if (this->outputs_[i-1] != nullptr) {
      if (!this->write_byte(get_fan_reg_(i), 0)) {
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
    if (this->outputs_[i] != nullptr) {
      ESP_LOGCONFIG(TAG, "  Fan %d configured", i + 1);
    }
  }
}

EMC2305Output *EMC2305Component::create_fan_output(uint8_t fan_number) {
  if (fan_number < 1 || fan_number > 5) {
    ESP_LOGE(TAG, "Invalid fan number: %d", fan_number);
    return nullptr;
  }
  
  if (this->outputs_[fan_number - 1] != nullptr) {
    return this->outputs_[fan_number - 1];
  }
  
  auto *output = new EMC2305Output(this, fan_number);
  this->outputs_[fan_number - 1] = output;
  return output;
}

void EMC2305Component::set_pwm_duty(uint8_t fan_number, float duty) {
  if (fan_number < 1 || fan_number > 5) {
    ESP_LOGE(TAG, "Invalid fan number: %d", fan_number);
    return;
  }

  uint8_t pwm = duty * 255;
  if (!this->write_byte(get_fan_reg_(fan_number), pwm)) {
    ESP_LOGE(TAG, "Failed to set fan %d PWM duty cycle", fan_number);
    this->status_set_warning();
  }
}

}  // namespace emc2305
}  // namespace esphome