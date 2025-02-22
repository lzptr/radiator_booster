import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, output
from esphome.const import CONF_ID, CONF_NAME, CONF_OUTPUT_ID

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["output"]

CONF_FAN1 = "fan1"
CONF_FAN2 = "fan2"
CONF_FAN3 = "fan3"
CONF_FAN4 = "fan4"
CONF_FAN5 = "fan5"

emc2305_ns = cg.esphome_ns.namespace("emc2305")
EMC2305Component = emc2305_ns.class_("EMC2305Component", cg.Component, i2c.I2CDevice)
EMC2305Output = emc2305_ns.class_("EMC2305Output", output.FloatOutput)

FAN_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_NAME): cv.string,
        cv.Required(CONF_OUTPUT_ID): cv.declare_id(EMC2305Output),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EMC2305Component),
            cv.Optional(CONF_FAN1): FAN_SCHEMA,
            cv.Optional(CONF_FAN2): FAN_SCHEMA,
            cv.Optional(CONF_FAN3): FAN_SCHEMA,
            cv.Optional(CONF_FAN4): FAN_SCHEMA,
            cv.Optional(CONF_FAN5): FAN_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x2C))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    for i, fan_key in enumerate(
        [CONF_FAN1, CONF_FAN2, CONF_FAN3, CONF_FAN4, CONF_FAN5], 1
    ):
        if fan_conf := config.get(fan_key):
            output_var = cg.new_Pvariable(fan_conf[CONF_OUTPUT_ID], var, i)
            await output.register_output(
                output_var,
                {
                    CONF_ID: fan_conf[CONF_OUTPUT_ID],
                    CONF_NAME: fan_conf[CONF_NAME],
                },
            )
