# Switchbot-ESPHome-Component

ESPHome external component for controlling SwitchBot Curtain devices over BLE with an ESP32.

## Example

```yaml
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
    id: left_bedroom_curtain
    name: "Left Bedroom Curtain"
    mac_address: "F6:68:4A:2F:D3:FD"
    battery:
      name: "Left Bedroom Curtain Battery"
```

## Notes

- `mac_address` is required.
- `battery` is optional and exposes a separate battery percentage sensor in Home Assistant.
- The component relies on `esp32_ble_tracker` to see advertisements for state and battery updates.
