import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_VALUE

from esphome.const import (
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ICON_CURRENT_AC,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_HERTZ,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_VOLT_AMPS,
    UNIT_WATT,
    CONF_BUS_VOLTAGE,
    CONF_BATTERY_VOLTAGE,
)
from .. import RP6000_COMPONENT_SCHEMA, CONF_RP6000_ID, rp6000_ns


DEPENDENCIES = ["rp6000"]

RP6000Sensor = rp6000_ns.class_("RP6000Sensor", sensor.Sensor, cg.Component)

CONF_GRID_VOLTAGE = "grid_voltage"
CONF_GRID_FREQUENCY = "grid_frequency"
CONF_OUT_VOLTAGE = "out_voltage"
CONF_OUT_FREQUENCY = "out_frequency"
CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_BATTERY_CHARGE_CURRENT = "battery_charge_current"
CONF_POWER_VOLT_AMPERE = "power_volt_ampere"
CONF_POWER_WATT = "power_watt"
CONF_LOAD_PERCENTAGE = "load_percentage"
CONF_TEMPERATURE = "temperature"

TYPES = {
    CONF_GRID_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
    ),
    CONF_GRID_FREQUENCY: sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        icon=ICON_CURRENT_AC,
        accuracy_decimals=2,
    ),
    CONF_OUT_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
    ),
    CONF_OUT_FREQUENCY: sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        icon=ICON_CURRENT_AC,
        accuracy_decimals=2,
    ),
    CONF_BATTERY_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
    ),
    CONF_BATTERY_CHARGE_CURRENT: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        icon=ICON_CURRENT_AC,
        accuracy_decimals=2,
    ),
    CONF_POWER_VOLT_AMPERE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT_AMPS,
        device_class=DEVICE_CLASS_POWER,
        accuracy_decimals=0,
    ),
    CONF_POWER_WATT: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        device_class=DEVICE_CLASS_POWER,
        accuracy_decimals=0,
    ),
    CONF_LOAD_PERCENTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
    ),
    CONF_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        device_class=DEVICE_CLASS_TEMPERATURE,
        accuracy_decimals=2,
    ),
}

CONFIG_SCHEMA = RP6000_COMPONENT_SCHEMA.extend(
    {cv.Optional(type): schema for type, schema in TYPES.items()}
).extend(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RP6000Sensor),
        }
    )
)

async def to_code(config):
    paren = await cg.get_variable(config[CONF_RP6000_ID])

    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_parent(paren))
    await cg.register_component(var, config)

    for type, _ in TYPES.items():
        if type in config:
            conf = config[type]
            sens = await sensor.new_sensor(conf)
            cg.add(getattr(var, f"set_{type}")(sens))
