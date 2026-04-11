#include "switchbot_curtain.h"
#include "esphome/core/log.h"

namespace esphome {
namespace switchbot_curtain {

static const char *const TAG = "switchbot_curtain";

static const uint8_t CMD_OPEN[] = {0x57, 0x0F, 0x45, 0x01, 0x01, 0x01, 0x00};
static const uint8_t CMD_CLOSE[] = {0x57, 0x0F, 0x45, 0x01, 0x01, 0x01, 0x64};
static const uint8_t CMD_STOP[] = {0x57, 0x0F, 0x45, 0x01, 0x00, 0x01};

void SwitchbotCurtain::dump_config() {
  ESP_LOGCONFIG(TAG, "SwitchBot Curtain");
  ESP_LOGCONFIG(TAG, "  Address: 0x%012llX", this->address_);
  ESP_LOGCONFIG(TAG, "  Reverse mode: %s", this->reverse_mode_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Command state timeout: %u ms", this->command_state_timeout_ms_);
}

cover::CoverTraits SwitchbotCurtain::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(false);
  traits.set_supports_position(true);
  traits.set_supports_stop(true);
  return traits;
}

bool SwitchbotCurtain::matches_(const esp32_ble_tracker::ESPBTDevice &device) const {
  return device.address_uint64() == this->address_;
}

bool SwitchbotCurtain::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (!this->matches_(device)) {
    return false;
  }

  ESP_LOGD(TAG, "Matched SwitchBot Curtain advertisement: %s", device.address_str().c_str());
  this->update_from_advertisement_(device);
  return true;
}

void SwitchbotCurtain::update_from_advertisement_(const esp32_ble_tracker::ESPBTDevice &device) {
  ESP_LOGD(TAG, "Advertisement received for curtain device");

  // Placeholder for future parsing of advertisement data.
  // This is where battery %, position, and motion state should be extracted.

  if (this->battery_sensor_ != nullptr) {
    // this->battery_sensor_->publish_state(parsed_battery);
  }

  if (this->has_position_) {
    this->publish_state();
  }
}

void SwitchbotCurtain::publish_position_(float position) {
  this->position = position;
  this->current_position_ = position;
  this->has_position_ = true;
  this->publish_state();
}

void SwitchbotCurtain::publish_operation_(cover::CoverOperation operation) {
  this->current_operation = operation;
  this->publish_state();
}

void SwitchbotCurtain::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    this->handle_stop_();
    return;
  }

  if (call.get_position().has_value()) {
    this->handle_position_(*call.get_position());
    return;
  }

  if (call.get_command_open()) {
    this->handle_open_();
    return;
  }

  if (call.get_command_close()) {
    this->handle_close_();
    return;
  }
}

void SwitchbotCurtain::handle_open_() {
  ESP_LOGI(TAG, "Sending open command");
  this->publish_operation_(cover::COVER_OPERATION_OPENING);
  this->send_command_(std::vector<uint8_t>(std::begin(CMD_OPEN), std::end(CMD_OPEN)));
}

void SwitchbotCurtain::handle_close_() {
  ESP_LOGI(TAG, "Sending close command");
  this->publish_operation_(cover::COVER_OPERATION_CLOSING);
  this->send_command_(std::vector<uint8_t>(std::begin(CMD_CLOSE), std::end(CMD_CLOSE)));
}

void SwitchbotCurtain::handle_stop_() {
  ESP_LOGI(TAG, "Sending stop command");
  this->publish_operation_(cover::COVER_OPERATION_IDLE);
  this->send_command_(std::vector<uint8_t>(std::begin(CMD_STOP), std::end(CMD_STOP)));
}

void SwitchbotCurtain::handle_position_(float position) {
  ESP_LOGI(TAG, "Setting curtain position to %.2f", position);

  uint8_t raw_position = static_cast<uint8_t>(position * 100.0f);
  if (this->reverse_mode_) {
    raw_position = 100 - raw_position;
  }

  std::vector<uint8_t> payload = {0x57, 0x0F, 0x45, 0x01, 0x01, 0x01, raw_position};

  if (position > this->current_position_) {
    this->publish_operation_(cover::COVER_OPERATION_OPENING);
  } else if (position < this->current_position_) {
    this->publish_operation_(cover::COVER_OPERATION_CLOSING);
  }

  this->send_command_(payload);
}

bool SwitchbotCurtain::send_command_(const std::vector<uint8_t> &payload) {
  ESP_LOGW(TAG, "BLE command send not implemented yet; payload size=%u",
           static_cast<unsigned int>(payload.size()));
  return false;
}

}  // namespace switchbot_curtain
}  // namespace esphome