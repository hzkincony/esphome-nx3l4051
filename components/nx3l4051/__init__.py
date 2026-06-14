import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.const import CONF_DELAY, CONF_ID

CODEOWNERS = ["@idreamshen"]

DEPENDENCIES = []
MULTI_CONF = True

nx3l4051_ns = cg.esphome_ns.namespace("nx3l4051")
NX3L4051Component = nx3l4051_ns.class_("NX3L4051Component", cg.Component)
SelectAction = nx3l4051_ns.class_("SelectAction", automation.Action)
DisableAction = nx3l4051_ns.class_("DisableAction", automation.Action)

CONF_PIN_S1 = "pin_s1"
CONF_PIN_S2 = "pin_s2"
CONF_PIN_S3 = "pin_s3"
CONF_ENABLE_PIN = "enable_pin"
CONF_CHANNEL = "channel"

FIXED_LOW = "LOW"
FIXED_HIGH = "HIGH"


def validate_select_pin(value):
    if isinstance(value, str) and value.upper() in (FIXED_LOW, FIXED_HIGH):
        return value.upper()
    return pins.gpio_output_pin_schema(value)


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(NX3L4051Component),
        cv.Required(CONF_PIN_S1): validate_select_pin,
        cv.Required(CONF_PIN_S2): validate_select_pin,
        cv.Required(CONF_PIN_S3): validate_select_pin,
        cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_DELAY, default="20ms"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def set_select_pin(var, config, key, setter):
    value = config[key]
    if value == FIXED_LOW:
        cg.add(setter(False))
        return
    if value == FIXED_HIGH:
        cg.add(setter(True))
        return
    pin = await cg.gpio_pin_expression(value)
    cg.add(setter(pin))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await set_select_pin(var, config, CONF_PIN_S1, var.set_s1)
    await set_select_pin(var, config, CONF_PIN_S2, var.set_s2)
    await set_select_pin(var, config, CONF_PIN_S3, var.set_s3)

    if CONF_ENABLE_PIN in config:
        enable_pin = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
        cg.add(var.set_enable_pin(enable_pin))

    cg.add(var.set_delay(config[CONF_DELAY]))


NX3L4051_SELECT_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(NX3L4051Component),
        cv.Required(CONF_CHANNEL): cv.templatable(cv.int_range(min=0, max=7)),
    }
)


@automation.register_action(
    "nx3l4051.select",
    SelectAction,
    NX3L4051_SELECT_ACTION_SCHEMA,
    synchronous=True,
)
async def nx3l4051_select_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    channel = await cg.templatable(config[CONF_CHANNEL], args, cg.uint8)
    cg.add(var.set_channel(channel))
    return var


NX3L4051_DISABLE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(NX3L4051Component),
    }
)


@automation.register_action(
    "nx3l4051.disable",
    DisableAction,
    NX3L4051_DISABLE_ACTION_SCHEMA,
    synchronous=True,
)
async def nx3l4051_disable_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)
