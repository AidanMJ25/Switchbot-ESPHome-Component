#pragma once

#include <vector>

#include "esphome/components/cover/cover.h"
#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace switchbot_curtain {

class SwitchbotCurtain : public cover::Cover, public esp32_ble_client::BLEClientBase {
 public:
  void set_reverse_mode(bool reverse_mode) { this->reverse_mode_ = reverse_mode; }
  void set_command_state_timeout(uint32_t timeout_ms) { this->command_state_timeout_ms_ = timeout_ms; }
  void set_battery_sensor(sensor::Sensor *battery_sensor) { this->battery_sensor_ = battery_sensor; }
  void set_light_level_sensor(sensor::Sensor *light_level_sensor) { this->light_level_sensor_ = light_level_sensor; }
  void set_rssi_sensor(sensor::Sensor *rssi_sensor) { this->rssi_sensor_ = rssi_sensor; }
  void set_calibration_binary_sensor(binary_sensor::BinarySensor *calibration_binary_sensor) {
    this->calibration_binary_sensor_ = calibration_binary_sensor;
  }
  void set_charging_binary_sensor(binary_sensor::BinarySensor *charging_binary_sensor) {
    this->charging_binary_sensor_ = charging_binary_sensor;
  }
  void set_has_solar_panel(bool has_solar_panel) { this->has_solar_panel_ = has_solar_panel; }

  void setup() override;
  void dump_config() override;
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  void control(const cover::CoverCall &call) override;
  cover::CoverTraits get_traits() override;

 protected:
  bool matches_(const esp32_ble_tracker::ESPBTDevice &device) const;
  void publish_position_(float position);
  void publish_operation_(cover::CoverOperation operation);
  void handle_open_();
  void handle_close_();
  void handle_stop_();
  void handle_position_(float position);
  uint8_t cover_position_to_device_position_(float position) const;
  void queue_command_(std::vector<uint8_t> payload, cover::CoverOperation operation,
                      optional<float> target_position = {});
  void schedule_connection_();
  bool send_pending_command_();
  void update_from_advertisement_(const esp32_ble_tracker::ESPBTDevice &device);
  bool parse_switchbot_service_data_(const esp32_ble_tracker::ServiceData &service_data);
  bool parse_switchbot_manufacturer_data_(const esp32_ble_tracker::ServiceData &manufacturer_data);
  bool apply_curtain_data_(bool in_motion, int raw_position, optional<float> battery, optional<int> light_level,
                           optional<bool> calibration);

  bool reverse_mode_{true};
  uint32_t command_state_timeout_ms_{5000};
  sensor::Sensor *battery_sensor_{nullptr};
  sensor::Sensor *light_level_sensor_{nullptr};
  sensor::Sensor *rssi_sensor_{nullptr};
  binary_sensor::BinarySensor *calibration_binary_sensor_{nullptr};
  binary_sensor::BinarySensor *charging_binary_sensor_{nullptr};
  bool has_solar_panel_{false};

  uint16_t command_handle_{0};
  float current_position_{cover::COVER_OPEN};
  bool has_position_{false};
  bool has_seen_device_{false};
  bool command_pending_{false};
  std::vector<uint8_t> pending_command_{};
  optional<float> pending_target_position_{};
  cover::CoverOperation pending_operation_{cover::COVER_OPERATION_IDLE};
};

}  // namespace switchbot_curtain
}  // namespace esphome
