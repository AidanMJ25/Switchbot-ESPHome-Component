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
    CONF_DEVICE_CLASS,
    CONF_DEVICE_ID,
    CONF_ID,
    CONF_MAC_ADDRESS,
    CONF_NAME,
)

AUTO_LOAD = ["esp32_ble_client", "sensor", "binary_sensor"]
DEPENDENCIES = ["esp32_ble_tracker"]

CONF_REVERSE_MODE = "reverse_mode"
CONF_COMMAND_STATE_TIMEOUT = "command_state_timeout"
CONF_BATTERY = "battery"
CONF_LIGHT_LEVEL = "light_level"
CONF_RSSI = "rssi"
CONF_CALIBRATION = "calibration"
CONF_CHARGING = "charging"
CONF_DIAGNOSTICS = "diagnostics"
CONF_HAS_SOLAR_PANEL = "has_solar_panel"

switchbot_curtain_ns = cg.esphome_ns.namespace("switchbot_curtain")
BATTERY_CONFIG_VALIDATOR = cv.maybe_simple_value(
    sensor.sensor_schema(
        unit_of_measurement="%",
        accuracy_decimals=0,
        device_class="battery",
        state_class="measurement",
        entity_category="diagnostic",
    ),
    key=CONF_NAME,
)
BATTERY_SCHEMA = cv.Any(cv.boolean, BATTERY_CONFIG_VALIDATOR)

LIGHT_LEVEL_CONFIG_VALIDATOR = cv.maybe_simple_value(
    sensor.sensor_schema(
        accuracy_decimals=0,
        state_class="measurement",
        entity_category="diagnostic",
    ),
    key=CONF_NAME,
)
LIGHT_LEVEL_SCHEMA = cv.Any(cv.boolean, LIGHT_LEVEL_CONFIG_VALIDATOR)

RSSI_CONFIG_VALIDATOR = cv.maybe_simple_value(
    sensor.sensor_schema(
        unit_of_measurement="dBm",
        accuracy_decimals=0,
        state_class="measurement",
        entity_category="diagnostic",
    ),
    key=CONF_NAME,
)
RSSI_SCHEMA = cv.Any(cv.boolean, RSSI_CONFIG_VALIDATOR)

CALIBRATION_CONFIG_VALIDATOR = cv.maybe_simple_value(
    binary_sensor.binary_sensor_schema(entity_category="diagnostic"),
    key=CONF_NAME,
)
CALIBRATION_SCHEMA = cv.Any(cv.boolean, CALIBRATION_CONFIG_VALIDATOR)

CHARGING_CONFIG_VALIDATOR = cv.maybe_simple_value(
    binary_sensor.binary_sensor_schema(entity_category="diagnostic"),
    key=CONF_NAME,
)
CHARGING_SCHEMA = cv.Any(cv.boolean, CHARGING_CONFIG_VALIDATOR)

DIAGNOSTICS_SCHEMA = cv.Any(
    cv.boolean,
    cv.Schema(
        {
            cv.Optional(CONF_DEVICE_ID): cv.sub_device_id,
            cv.Optional(CONF_BATTERY, default=True): cv.boolean,
            cv.Optional(CONF_LIGHT_LEVEL, default=True): cv.boolean,
            cv.Optional(CONF_RSSI, default=True): cv.boolean,
            cv.Optional(CONF_CALIBRATION, default=True): cv.boolean,
            cv.Optional(CONF_CHARGING, default=True): cv.boolean,
        }
    ),
)


def _normalize_diagnostics(config):
    config = dict(config)
    config.setdefault(CONF_DEVICE_CLASS, "curtain")
    cover_name = config.get(CONF_NAME, "Curtain")
    diagnostics = config.pop(CONF_DIAGNOSTICS, None)
    default_device_id = config.get(CONF_DEVICE_ID)
    diagnostics_device_id = default_device_id

    diagnostics_defaults = None
    if diagnostics is True:
        diagnostics_defaults = {
            CONF_BATTERY: True,
            CONF_LIGHT_LEVEL: True,
            CONF_RSSI: True,
            CONF_CALIBRATION: True,
        }
        if config.get(CONF_HAS_SOLAR_PANEL):
            diagnostics_defaults[CONF_CHARGING] = True
    elif isinstance(diagnostics, dict):
        diagnostics_device_id = diagnostics.get(CONF_DEVICE_ID, default_device_id)
        diagnostics_defaults = diagnostics

    for key, suffix in (
        (CONF_BATTERY, "Battery"),
        (CONF_LIGHT_LEVEL, "Light Level"),
        (CONF_RSSI, "RSSI"),
        (CONF_CALIBRATION, "Calibrated"),
        (CONF_CHARGING, "Charging"),
    ):
        value = config.get(key)
        if value is False:
            config.pop(key, None)
            continue

        if value is True:
            value = {CONF_NAME: f"{cover_name} {suffix}"}
        elif value is None and diagnostics_defaults and diagnostics_defaults.get(key):
            value = {CONF_NAME: f"{cover_name} {suffix}"}

        if isinstance(value, dict):
            if diagnostics_device_id is not None and CONF_DEVICE_ID not in value:
                value[CONF_DEVICE_ID] = diagnostics_device_id
            if key == CONF_BATTERY:
                config[key] = BATTERY_CONFIG_VALIDATOR(value)
            elif key == CONF_LIGHT_LEVEL:
                config[key] = LIGHT_LEVEL_CONFIG_VALIDATOR(value)
            elif key == CONF_RSSI:
                config[key] = RSSI_CONFIG_VALIDATOR(value)
            elif key == CONF_CALIBRATION:
                config[key] = CALIBRATION_CONFIG_VALIDATOR(value)
            elif key == CONF_CHARGING:
                config[key] = CHARGING_CONFIG_VALIDATOR(value)

    return config


SwitchbotCurtain = switchbot_curtain_ns.class_(
    "SwitchbotCurtain",
    cover.Cover,
    esp32_ble_client.BLEClientBase,
)

CONFIG_SCHEMA = cv.All(
    cover._COVER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(SwitchbotCurtain),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Optional(CONF_REVERSE_MODE, default=True): cv.boolean,
            cv.Optional(
                CONF_COMMAND_STATE_TIMEOUT, default="5s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_BATTERY): BATTERY_SCHEMA,
            cv.Optional(CONF_LIGHT_LEVEL): LIGHT_LEVEL_SCHEMA,
            cv.Optional(CONF_RSSI): RSSI_SCHEMA,
            cv.Optional(CONF_CALIBRATION): CALIBRATION_SCHEMA,
            cv.Optional(CONF_CHARGING): CHARGING_SCHEMA,
            cv.Optional(CONF_HAS_SOLAR_PANEL, default=False): cv.boolean,
            cv.Optional(CONF_DIAGNOSTICS, default=True): DIAGNOSTICS_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA),
    _normalize_diagnostics,
)


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
    cg.add(var.set_has_solar_panel(config[CONF_HAS_SOLAR_PANEL]))

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

    if CONF_CHARGING in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_CHARGING])
        cg.add(var.set_charging_binary_sensor(sens))
