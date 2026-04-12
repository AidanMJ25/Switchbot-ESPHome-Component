import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import (
    binary_sensor,
    cover,
    esp32_ble_client,
    esp32_ble_tracker,
    sensor,
)
from esphome.const import (
    CONF_ID,
    CONF_MAC_ADDRESS,
)

AUTO_LOAD = ["esp32_ble_client", "sensor"]
DEPENDENCIES = ["esp32_ble_tracker"]

CONF_REVERSE_MODE = "reverse_mode"
CONF_COMMAND_STATE_TIMEOUT = "command_state_timeout"
CONF_BATTERY = "battery"
CONF_LIGHT_LEVEL = "light_level"
CONF_RSSI = "rssi"
CONF_CALIBRATION = "calibration"

switchbot_curtain_ns = cg.esphome_ns.namespace("switchbot_curtain")
SwitchbotCurtain = switchbot_curtain_ns.class_(
    "SwitchbotCurtain",
    cover.Cover,
    esp32_ble_client.BLEClientBase,
)

CONFIG_SCHEMA = cover._COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SwitchbotCurtain),
        cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
        cv.Optional(CONF_REVERSE_MODE, default=True): cv.boolean,
        cv.Optional(
            CONF_COMMAND_STATE_TIMEOUT, default="5s"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_BATTERY): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            device_class="battery",
            state_class="measurement",
        ),
        cv.Optional(CONF_LIGHT_LEVEL): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class="measurement",
        ),
        cv.Optional(CONF_RSSI): sensor.sensor_schema(
            unit_of_measurement="dBm",
            accuracy_decimals=0,
            state_class="measurement",
        ),
        cv.Optional(CONF_CALIBRATION): binary_sensor.binary_sensor_schema(),
    }
).extend(cv.COMPONENT_SCHEMA).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)


async def to_code(config):
    cg.add_define("USE_ESP32_BLE_UUID")
    esp32_ble_tracker.register_ble_features(
        {esp32_ble_tracker.BLEFeatures.ESP_BT_DEVICE}
    )

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    await esp32_ble_tracker.register_client(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_reverse_mode(config[CONF_REVERSE_MODE]))
    cg.add(var.set_command_state_timeout(config[CONF_COMMAND_STATE_TIMEOUT]))

    if CONF_BATTERY in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY])
        cg.add(var.set_battery_sensor(sens))

    if CONF_LIGHT_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_LIGHT_LEVEL])
        cg.add(var.set_light_level_sensor(sens))

    if CONF_RSSI in config:
        sens = await sensor.new_sensor(config[CONF_RSSI])
        cg.add(var.set_rssi_sensor(sens))

    if CONF_CALIBRATION in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CALIBRATION])
        cg.add(var.set_calibration_binary_sensor(sens))
