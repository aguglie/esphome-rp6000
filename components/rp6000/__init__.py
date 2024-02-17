import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import uart

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@aguglie"]
AUTO_LOAD = ["sensor", "switch"]
MULTI_CONF = True

CONF_RP6000_ID = "rp6000_id"

rp6000_ns = cg.esphome_ns.namespace("rp6000")
RP6000Component = rp6000_ns.class_("RP6000", cg.Component)

RP6000_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_RP6000_ID): cv.use_id(RP6000Component),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Schema({cv.GenerateID(): cv.declare_id(RP6000Component)})
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
