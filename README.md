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
    - id: right_curtain_device
      name: "Right Bedroom Curtain"

esp32_ble_tracker:

external_components:
  - source:
      type: git
      url: https://github.com/AidanMJ25/Switchbot-ESPHome-Component.git
      ref: main
    components: [switchbot_curtain]

cover:
  - platform: switchbot_curtain
    device_id: left_curtain_device
    id: left_bedroom_curtain
    name: "Left Bedroom Curtain"
    mac_address: "AA:BB:CC:DD:EE:FF"
    
  - platform: switchbot_curtain
    device_class: curtain
    device_id: right_curtain_device
    id: right_bedroom_curtain
    name: "Right Bedroom Curtain"
    mac_address: "AA:BB:CC:DD:EE:FF"
```

## Notes

- `mac_address` is required.
- Diagnostics are enabled by default and auto-create `battery`, `rssi`, `light_level`, and `calibration` entities with default names on the same curtain device.
- Set `diagnostics: false` if you want to disable all of them.
- You can still configure `battery`, `rssi`, `light_level`, or `calibration` individually if you want explicit names or selective exposure.
- For Home Assistant device grouping, declare ESPHome `devices:` and assign `device_id` so each curtain appears as its own device instead of attaching battery entities to the USB-powered ESP32 hub.
- The component relies on `esp32_ble_tracker` to see advertisements for state and battery updates.
