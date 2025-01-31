import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    UNIT_REVOLUTIONS_PER_MINUTE,
    STATE_CLASS_MEASUREMENT,
    ICON_FAN,
    CONF_UPDATE_INTERVAL,
)

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["output", "sensor"]

# Component namespace
emc2305_ns = cg.esphome_ns.namespace("emc2305")
EMC2305Component = emc2305_ns.class_(
    "EMC2305Component", cg.PollingComponent, i2c.I2CDevice
)
EMC2305Fan = emc2305_ns.class_("EMC2305Fan")

# Config keys for each fan
CONF_FAN1 = "fan1"
CONF_FAN2 = "fan2"
CONF_FAN3 = "fan3"
CONF_FAN4 = "fan4"
CONF_FAN5 = "fan5"

# Optional fan configuration
CONF_RPM_SENSOR = "rpm_sensor"  # If True, creates a sensor to monitor RPM
CONF_MIN_RPM = "min_rpm"  # Minimum RPM for stall detection
CONF_TACH_MODE = "tach_mode"  # TACH input configuration
CONF_PWM_DIVIDER = "pwm_divider"  # PWM frequency divider

# TACH mode configuration - number of edges to sample
TACH_MODES = {
    "1_PULSE": 0b00,  # 3 edges (1 pole)
    "2_PULSE": 0b01,  # 5 edges (2 poles - default for Noctua)
    "3_PULSE": 0b10,  # 7 edges (3 poles)
    "4_PULSE": 0b11,  # 9 edges (4 poles)
}

# Validation schema for each fan
FAN_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_NAME): cv.string,
        cv.Optional(CONF_RPM_SENSOR, default=True): cv.boolean,
        cv.Optional(CONF_MIN_RPM, default=100): cv.positive_int,
        cv.Optional(CONF_TACH_MODE, default="2_PULSE"): cv.enum(TACH_MODES, upper=True),
        cv.Optional(CONF_PWM_DIVIDER, default=1): cv.int_range(min=1, max=255),
    }
)

# Main component configuration schema
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EMC2305Component),
            cv.Optional(CONF_FAN1): FAN_SCHEMA,
            cv.Optional(CONF_FAN2): FAN_SCHEMA,
            cv.Optional(CONF_FAN3): FAN_SCHEMA,
            cv.Optional(CONF_FAN4): FAN_SCHEMA,
            cv.Optional(CONF_FAN5): FAN_SCHEMA,
            cv.Optional(CONF_UPDATE_INTERVAL, default="60s"): cv.update_interval,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x2C))  # Default address for 10k pull-up
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Configure each fan if specified
    for i, fan_key in enumerate([CONF_FAN1, CONF_FAN2, CONF_FAN3, CONF_FAN4, CONF_FAN5], 1):
        if fan_conf := config.get(fan_key):
            fan = cg.Pvariable(cg.new_Pvariable(), var.create_fan(i))
            
            # Create RPM sensor if requested
            if fan_conf[CONF_RPM_SENSOR]:
                sens = await cg.new_sensor(
                    cg.new_Pvariable(),
                    name=f"{fan_conf[CONF_NAME]} RPM",
                    unit_of_measurement=UNIT_REVOLUTIONS_PER_MINUTE,
                    accuracy_decimals=0,
                    icon=ICON_FAN,
                    state_class=STATE_CLASS_MEASUREMENT
                )
                cg.add(fan.set_rpm_sensor(sens))
            
            # Apply fan configuration
            cg.add(fan.set_name(fan_conf[CONF_NAME]))
            
            # Add to output platform registry
            suffix = f"Fan_{i}"
            output_var = await cg.get_variable(config[CONF_ID])
            cg.add(
                cg.App.register_output(
                    output_var, 
                    f"{fan_conf[CONF_NAME]} Speed", 
                    suffix
                )
            )