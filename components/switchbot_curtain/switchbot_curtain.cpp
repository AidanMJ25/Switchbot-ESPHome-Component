#include "switchbot_curtain.h"

#include <cmath>
#include <utility>

#include "esphome/components/esp32_ble_client/ble_characteristic.h"
#include "esphome/core/log.h"

namespace esphome {
namespace switchbot_curtain {

static const char *const TAG = "switchbot_curtain";
static const char *const COMMAND_TIMEOUT_NAME = "command-finish";
static const char *const DISCONNECT_TIMEOUT_NAME = "disconnect-after-write";

static const uint8_t CMD_OPEN[] = {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x00};
static const uint8_t CMD_CLOSE[] = {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x64};
static const uint8_t CMD_STOP[] = {0x57, 0x0F, 0x45, 0x01, 0x00, 0xFF};

static const auto COMMAND_SERVICE_UUID =
    esp32_ble::ESPBTUUID::from_raw("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
static const auto COMMAND_CHARACTERISTIC_UUID =
    esp32_ble::ESPBTUUID::from_raw("cba20002-224d-11e6-9fb8-0002a5d5c51b");

void SwitchbotCurtain::setup() {
  this->set_auto_connect(false);
  BLEClientBase::setup();

  if (auto restore = this->restore_state_()) {
    restore->apply(this);
    this->current_position_ = this->position;
    this->has_position_ = true;
  } else {
    this->current_operation = cover::COVER_OPERATION_IDLE;
  }
}

void SwitchbotCurtain::dump_config() {
  ESP_LOGCONFIG(TAG, "SwitchBot Curtain");
  LOG_COVER("  ", "SwitchBot Curtain", this);
  BLEClientBase::dump_config();
  ESP_LOGCONFIG(TAG, "  Reverse mode: %s", TRUEFALSE(this->reverse_mode_));
  ESP_LOGCONFIG(TAG, "  Command state timeout: %u ms", this->command_state_timeout_ms_);
}

cover::CoverTraits SwitchbotCurtain::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(true);
  traits.set_supports_position(true);
  traits.set_supports_stop(true);
  return traits;
}

bool SwitchbotCurtain::matches_(const esp32_ble_tracker::ESPBTDevice &device) const {
  return device.address_uint64() == this->get_address();
}

bool SwitchbotCurtain::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (!this->matches_(device)) {
    return false;
  }

  this->has_seen_device_ = true;
  this->set_remote_addr_type(device.get_address_type());

  ESP_LOGD(TAG, "Matched SwitchBot Curtain advertisement: %s", device.address_str().c_str());
  this->update_from_advertisement_(device);

  if (this->command_pending_ && this->state() == esp32_ble_tracker::ClientState::IDLE) {
    this->schedule_connection_();
  }

  return true;
}

bool SwitchbotCurtain::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                           esp_ble_gattc_cb_param_t *param) {
  if (!BLEClientBase::gattc_event_handler(event, gattc_if, param)) {
    return false;
  }

  switch (event) {
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      this->command_handle_ = 0;
      auto *chr = this->get_characteristic(COMMAND_SERVICE_UUID, COMMAND_CHARACTERISTIC_UUID);
      if (chr == nullptr) {
        ESP_LOGW(TAG, "SwitchBot command characteristic not found");
        break;
      }

      this->command_handle_ = chr->handle;
      ESP_LOGD(TAG, "Resolved SwitchBot command handle: 0x%04X", this->command_handle_);

      if (this->command_pending_) {
        this->send_pending_command_();
      }
      break;
    }
    case ESP_GATTC_CLOSE_EVT:
      this->command_handle_ = 0;
      break;
    default:
      break;
  }

  return true;
}

void SwitchbotCurtain::publish_position_(float position) {
  this->position = clamp(position, 0.0f, 1.0f);
  this->current_position_ = this->position;
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
    const float position = *call.get_position();
    if (position <= cover::COVER_CLOSED) {
      this->handle_close_();
    } else if (position >= cover::COVER_OPEN) {
      this->handle_open_();
    } else {
      this->handle_position_(position);
    }
  }
}

void SwitchbotCurtain::handle_open_() {
  ESP_LOGI(TAG, "Queueing open command");
  this->queue_command_(std::vector<uint8_t>(std::begin(CMD_OPEN), std::end(CMD_OPEN)),
                       cover::COVER_OPERATION_OPENING, cover::COVER_OPEN);
}

void SwitchbotCurtain::handle_close_() {
  ESP_LOGI(TAG, "Queueing close command");
  this->queue_command_(std::vector<uint8_t>(std::begin(CMD_CLOSE), std::end(CMD_CLOSE)),
                       cover::COVER_OPERATION_CLOSING, cover::COVER_CLOSED);
}

void SwitchbotCurtain::handle_stop_() {
  ESP_LOGI(TAG, "Queueing stop command");
  this->queue_command_(std::vector<uint8_t>(std::begin(CMD_STOP), std::end(CMD_STOP)),
                       cover::COVER_OPERATION_IDLE);
}

void SwitchbotCurtain::handle_position_(float position) {
  const uint8_t raw_position = this->cover_position_to_device_position_(position);
  std::vector<uint8_t> payload = {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, raw_position};

  cover::CoverOperation operation = cover::COVER_OPERATION_IDLE;
  if (position > this->current_position_) {
    operation = cover::COVER_OPERATION_OPENING;
  } else if (position < this->current_position_) {
    operation = cover::COVER_OPERATION_CLOSING;
  }

  ESP_LOGI(TAG, "Queueing position command %.0f%% (device=%u)", position * 100.0f, raw_position);
  this->queue_command_(std::move(payload), operation, position);
}

uint8_t SwitchbotCurtain::cover_position_to_device_position_(float position) const {
  const float clamped = clamp(position, 0.0f, 1.0f);
  const float scaled = this->reverse_mode_ ? (1.0f - clamped) * 100.0f : clamped * 100.0f;
  return static_cast<uint8_t>(clamp<int>(static_cast<int>(std::lround(scaled)), 0, 100));
}

void SwitchbotCurtain::queue_command_(std::vector<uint8_t> payload, cover::CoverOperation operation,
                                      optional<float> target_position) {
  this->pending_command_ = std::move(payload);
  this->pending_operation_ = operation;
  this->pending_target_position_ = target_position;
  this->command_pending_ = true;

  if (operation != cover::COVER_OPERATION_IDLE) {
    this->publish_operation_(operation);
  } else {
    this->publish_state();
  }

  if (this->state() == esp32_ble_tracker::ClientState::ESTABLISHED) {
    this->send_pending_command_();
    return;
  }

  this->schedule_connection_();
}

void SwitchbotCurtain::schedule_connection_() {
  if (!this->has_seen_device_) {
    ESP_LOGW(TAG, "Cannot connect yet; no advertisement has been seen for this curtain");
    return;
  }

  if (this->state() == esp32_ble_tracker::ClientState::IDLE) {
    ESP_LOGD(TAG, "Scheduling BLE connection to send queued curtain command");
    this->set_state(esp32_ble_tracker::ClientState::DISCOVERED);
    return;
  }

  if (this->state() == esp32_ble_tracker::ClientState::DISCONNECTING) {
    ESP_LOGD(TAG, "Waiting for current BLE disconnect to complete before retrying command");
  }
}

bool SwitchbotCurtain::send_pending_command_() {
  if (!this->command_pending_) {
    return false;
  }

  if (this->state() != esp32_ble_tracker::ClientState::ESTABLISHED) {
    this->schedule_connection_();
    return false;
  }

  if (this->command_handle_ == 0) {
    auto *chr = this->get_characteristic(COMMAND_SERVICE_UUID, COMMAND_CHARACTERISTIC_UUID);
    if (chr == nullptr) {
      ESP_LOGW(TAG, "Cannot send command; SwitchBot command characteristic is unavailable");
      return false;
    }
    this->command_handle_ = chr->handle;
  }

  auto status = esp_ble_gattc_write_char(this->get_gattc_if(), this->get_conn_id(), this->command_handle_,
                                         this->pending_command_.size(), this->pending_command_.data(),
                                         ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (status != ESP_OK) {
    ESP_LOGW(TAG, "Failed to queue SwitchBot BLE command, status=%d", status);
    return false;
  }

  ESP_LOGI(TAG, "Queued SwitchBot BLE command (%u bytes)", static_cast<unsigned int>(this->pending_command_.size()));

  const auto target_position = this->pending_target_position_;
  this->command_pending_ = false;
  this->pending_command_.clear();

  this->set_timeout(COMMAND_TIMEOUT_NAME, this->command_state_timeout_ms_, [this, target_position]() {
    if (target_position.has_value()) {
      this->position = clamp(*target_position, 0.0f, 1.0f);
      this->current_position_ = this->position;
      this->has_position_ = true;
    }
    this->current_operation = cover::COVER_OPERATION_IDLE;
    this->publish_state();
  });

  this->set_timeout(DISCONNECT_TIMEOUT_NAME, 750, [this]() {
    if (this->state() == esp32_ble_tracker::ClientState::CONNECTED ||
        this->state() == esp32_ble_tracker::ClientState::ESTABLISHED) {
      this->disconnect();
    }
  });

  return true;
}

void SwitchbotCurtain::update_from_advertisement_(const esp32_ble_tracker::ESPBTDevice &device) {
  if (this->rssi_sensor_ != nullptr) {
    this->rssi_sensor_->publish_state(device.get_rssi());
  }

  for (const auto &service_data : device.get_service_datas()) {
    if (this->parse_switchbot_service_data_(service_data)) {
      return;
    }
  }

  for (const auto &manufacturer_data : device.get_manufacturer_datas()) {
    if (this->parse_switchbot_manufacturer_data_(manufacturer_data)) {
      return;
    }
  }

  ESP_LOGD(TAG, "Advertisement received for curtain device");
}

bool SwitchbotCurtain::parse_switchbot_service_data_(const esp32_ble_tracker::ServiceData &service_data) {
  if (service_data.data.size() < 5) {
    return false;
  }

  const uint8_t model = service_data.data[0] & 0x7F;
  if (model != 'c' && model != 'C' && model != '{' && model != '[') {
    return false;
  }

  const bool in_motion = (service_data.data[3] & 0x80) != 0;
  const int raw_position = clamp<int>(service_data.data[3] & 0x7F, 0, 100);
  const float battery = service_data.data[2] & 0x7F;
  const int light_level = (service_data.data[4] >> 4) & 0x0F;
  const bool calibration = (service_data.data[1] & 0x40) != 0;

  return this->apply_curtain_data_(in_motion, raw_position, battery, light_level, calibration);
}

bool SwitchbotCurtain::parse_switchbot_manufacturer_data_(const esp32_ble_tracker::ServiceData &manufacturer_data) {
  if (manufacturer_data.data.size() < 11) {
    return false;
  }

  const uint8_t *device_data = manufacturer_data.data.data() + 8;
  const bool in_motion = (device_data[0] & 0x80) != 0;
  const int raw_position = clamp<int>(device_data[0] & 0x7F, 0, 100);
  const int light_level = (device_data[1] >> 4) & 0x0F;
  optional<float> battery{};
  if (manufacturer_data.data.size() > 12) {
    battery = static_cast<float>(manufacturer_data.data[12] & 0x7F);
  }

  return this->apply_curtain_data_(in_motion, raw_position, battery, light_level, {});
}

bool SwitchbotCurtain::apply_curtain_data_(bool in_motion, int raw_position, optional<float> battery,
                                           optional<int> light_level, optional<bool> calibration) {
  const float previous_position = this->current_position_;
  int curtain_position = this->reverse_mode_ ? 100 - raw_position : raw_position;
  const int normalized_position = curtain_position;
  if (curtain_position <= 2) {
    curtain_position = 0;
  } else if (curtain_position >= 98) {
    curtain_position = 100;
  }
  const float cover_position = curtain_position / 100.0f;

  if (battery.has_value() && this->battery_sensor_ != nullptr) {
    this->battery_sensor_->publish_state(*battery);
  }
  if (light_level.has_value() && this->light_level_sensor_ != nullptr) {
    this->light_level_sensor_->publish_state(*light_level);
  }
  if (calibration.has_value() && this->calibration_binary_sensor_ != nullptr) {
    this->calibration_binary_sensor_->publish_state(*calibration);
  }
  if (this->charging_binary_sensor_ != nullptr) {
    const bool charging =
        this->has_solar_panel_ && light_level.has_value() && *light_level > 0 &&
        (!battery.has_value() || *battery < 100.0f);
    this->charging_binary_sensor_->publish_state(charging);
  }

  this->position = cover_position;
  this->current_position_ = cover_position;
  this->has_position_ = true;

  if (in_motion) {
    if (cover_position > previous_position + 0.01f) {
      this->current_operation = cover::COVER_OPERATION_OPENING;
    } else if (cover_position < previous_position - 0.01f) {
      this->current_operation = cover::COVER_OPERATION_CLOSING;
    }
  } else {
    this->current_operation = cover::COVER_OPERATION_IDLE;
  }

  this->publish_state(false);
  const int battery_value = battery.has_value() ? static_cast<int>(*battery) : -1;
  const int light_value = light_level.has_value() ? *light_level : -1;
  const int calibration_value = calibration.has_value() ? static_cast<int>(*calibration) : -1;
  ESP_LOGD(TAG,
           "Advertisement update: raw=%d normalized=%d position=%.0f%% reverse=%s battery=%d light=%d "
           "calibrated=%d moving=%s",
           raw_position, normalized_position, cover_position * 100.0f, TRUEFALSE(this->reverse_mode_),
           battery_value, light_value, calibration_value, TRUEFALSE(in_motion));
  return true;
}

}  // namespace switchbot_curtain
}  // namespace esphome
