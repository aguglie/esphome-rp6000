import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import output
from esphome.const import CONF_ID, CONF_VALUE
from .. import RP6000_COMPONENT_SCHEMA, CONF_RP6000_ID, rp6000_ns

DEPENDENCIES = ["rp6000"]

RP6000Output = rp6000_ns.class_("RP6000Output", output.FloatOutput, cg.Component)
SetOutputAction = rp6000_ns.class_("SetOutputAction", automation.Action)

CONF_POSSIBLE_VALUES = "possible_values"

CONF_CURRENT_MAX_CHARGING_CURRENT = "current_max_charging_current"
CONF_OUTPUT_SOURCE_PRIORITY = "output_source_priority"

TYPES = {
    CONF_CURRENT_MAX_CHARGING_CURRENT: ([i for i in range(0, 50 + 1)], "SSD%02.0f0\r\n"),
    CONF_OUTPUT_SOURCE_PRIORITY: ([0, 1], "SSa%01.0f\r\n"),
}

CONFIG_SCHEMA = RP6000_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(type): output.FLOAT_OUTPUT_SCHEMA.extend(
            {
                cv.Required(CONF_ID): cv.declare_id(RP6000Output),
                cv.Optional(CONF_POSSIBLE_VALUES, default=values): cv.All(
                    cv.ensure_list(cv.positive_float), cv.Length(min=1)
                ),
            }
        )
        for type, (values, _) in TYPES.items()
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_RP6000_ID])

    for type, (_, command) in TYPES.items():
        if type in config:
            conf = config[type]
            var = cg.new_Pvariable(conf[CONF_ID])
            await output.register_output(var, conf)
            cg.add(var.set_parent(paren))
            cg.add(var.set_set_command(command))
            if (CONF_POSSIBLE_VALUES) in conf:
                cg.add(var.set_possible_values(conf[CONF_POSSIBLE_VALUES]))
            await cg.register_component(var, config)


@automation.register_action(
    "output.rp6000.set_level",
    SetOutputAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(CONF_ID),
            cv.Required(CONF_VALUE): cv.templatable(cv.positive_float),
        }
    ),
)
def output_rp6000_set_level_to_code(config, action_id, template_arg, args):
    paren = yield cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = yield cg.templatable(config[CONF_VALUE], args, float)
    cg.add(var.set_level(template_))
    yield var
