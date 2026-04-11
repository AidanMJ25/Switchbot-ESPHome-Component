#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace switchbot_curtain {

class SwitchbotCurtain : public cover::Cover,
                         public Component,
                         public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { this->address_ = address; }
  void set_reverse_mode(bool reverse_mode) { this->reverse_mode_ = reverse_mode; }
  void set_command_state_timeout(uint32_t timeout_ms) { this->command_state_timeout_ms_ = timeout_ms; }
  void set_battery_sensor(sensor::Sensor *battery_sensor) { this->battery_sensor_ = battery_sensor; }

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void control(const cover::CoverCall &call) override;

 protected:
  cover::CoverTraits get_traits() override;

  void publish_position_(float position);
  void publish_operation_(cover::CoverOperation operation);
  bool matches_(const esp32_ble_tracker::ESPBTDevice &device) const;

  void handle_open_();
  void handle_close_();
  void handle_stop_();
  void handle_position_(float position);

  bool send_command_(const std::vector<uint8_t> &payload);
  void update_from_advertisement_(const esp32_ble_tracker::ESPBTDevice &device);

  uint64_t address_{0};
  bool reverse_mode_{true};
  uint32_t command_state_timeout_ms_{5000};

  sensor::Sensor *battery_sensor_{nullptr};

  float current_position_{0.0f};
  bool has_position_{false};
};

}  // namespace switchbot_curtain
}  // namespace esphome