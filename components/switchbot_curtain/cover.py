import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import cover, esp32_ble_tracker, sensor
from esphome.const import CONF_ID, CONF_MAC_ADDRESS

AUTO_LOAD = ["cover", "sensor", "esp32_ble_tracker"]
DEPENDENCIES = ["esp32"]

CONF_SWITCHBOT_CURTAIN_ID = "switchbot_curtain_id"
CONF_REVERSE_MODE = "reverse_mode"
CONF_COMMAND_STATE_TIMEOUT = "command_state_timeout"
CONF_BATTERY = "battery"

switchbot_curtain_ns = cg.esphome_ns.namespace("switchbot_curtain")
SwitchbotCurtain = switchbot_curtain_ns.class_(
    "SwitchbotCurtain",
    cover.Cover,
    cg.Component,
    esp32_ble_tracker.ESPBTDeviceListener,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SwitchbotCurtain),
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Optional(CONF_REVERSE_MODE, default=True): cv.boolean,
            cv.Optional(CONF_COMMAND_STATE_TIMEOUT, default="5s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_BATTERY): sensor.sensor_schema(
                unit_of_measurement="%",
                accuracy_decimals=0,
                device_class="battery",
                state_class="measurement",
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

COVER_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.Required(CONF_SWITCHBOT_CURTAIN_ID): cv.use_id(SwitchbotCurtain),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_reverse_mode(config[CONF_REVERSE_MODE]))
    cg.add(var.set_command_state_timeout(config[CONF_COMMAND_STATE_TIMEOUT]))

    if CONF_BATTERY in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY])
        cg.add(var.set_battery_sensor(sens))


@cover.register_cover("switchbot_curtain", SwitchbotCurtain, COVER_SCHEMA)
async def switchbot_cover_to_code(config, cover_var):
    parent = await cg.get_variable(config[CONF_SWITCHBOT_CURTAIN_ID])
    cg.add(parent.set_cover_ref(cover_var))