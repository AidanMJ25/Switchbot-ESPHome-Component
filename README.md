# Switchbot-ESPHome-Component

ESPHome external component for controlling SwitchBot Curtain devices over BLE with an ESP32.

## Example

```yaml
esphome:
  name: esphome-switchbot-hub
  friendly_name: ESPHome SwitchBot Hub
  devices:
    - id: left_curtain_device
      name: "Left Bedroom Curtain"

esp32_ble_tracker:

external_components:
  - source:
      type: git
      url: https://github.com/AidanMJ25/Switchbot-ESPHome-Component.git
      ref: main
    components: [switchbot_curtain]
    refresh: 0s

cover:
  - platform: switchbot_curtain
    device_id: left_curtain_device
    id: left_bedroom_curtain
    name: "Left Bedroom Curtain"
    mac_address: "F6:68:4A:2F:D3:FD"
    battery:
      name: "Left Bedroom Curtain Battery"
      device_id: left_curtain_device
```

## Notes

- `mac_address` is required.
- `battery` is optional and exposes a separate battery percentage sensor in Home Assistant.
- For Home Assistant device grouping, declare ESPHome `devices:` and assign `device_id` so each curtain appears as its own device instead of attaching battery entities to the USB-powered ESP32 hub.
- The component relies on `esp32_ble_tracker` to see advertisements for state and battery updates.
