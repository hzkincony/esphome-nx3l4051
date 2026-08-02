import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import nx3l4051, sensor
from esphome.components.max31865 import sensor as max31865
from esphome.const import (
    CONF_CLK_PIN,
    CONF_CS_PIN,
    CONF_MAINS_FILTER,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
    CONF_REFERENCE_RESISTANCE,
    CONF_RTD_NOMINAL_RESISTANCE,
    CONF_RTD_WIRES,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

CODEOWNERS = ["@idreamshen"]
AUTO_LOAD = ["max31865", "spi"]
DEPENDENCIES = ["nx3l4051"]

CONF_MUX_ID = "mux_id"

nx3l4051_max31865_ns = cg.esphome_ns.namespace("nx3l4051_max31865")
NX3L4051MAX31865Sensor = nx3l4051_max31865_ns.class_(
    "NX3L4051MAX31865Sensor", max31865.MAX31865Sensor
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        NX3L4051MAX31865Sensor,
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Required(CONF_MUX_ID): cv.use_id(nx3l4051.NX3L4051Component),
            cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_MISO_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_REFERENCE_RESISTANCE): cv.All(
                cv.resistance, cv.Range(min=100, max=10000)
            ),
            cv.Required(CONF_RTD_NOMINAL_RESISTANCE): cv.All(
                cv.resistance, cv.Range(min=100, max=1000)
            ),
            cv.Optional(CONF_MAINS_FILTER, default="60HZ"): cv.enum(
                max31865.FILTER, upper=True, space=""
            ),
            cv.Optional(CONF_RTD_WIRES, default=4): cv.int_range(min=2, max=4),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    mux = await cg.get_variable(config[CONF_MUX_ID])
    cg.add(var.set_mux(mux))
    clk = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
    cg.add(var.set_clk_pin(clk))
    mosi = await cg.gpio_pin_expression(config[CONF_MOSI_PIN])
    cg.add(var.set_mosi_pin(mosi))
    miso = await cg.gpio_pin_expression(config[CONF_MISO_PIN])
    cg.add(var.set_miso_pin(miso))
    cs = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin_direct(cs))
    cg.add(var.set_reference_resistance(config[CONF_REFERENCE_RESISTANCE]))
    cg.add(var.set_nominal_resistance(config[CONF_RTD_NOMINAL_RESISTANCE]))
    cg.add(var.set_filter(config[CONF_MAINS_FILTER]))
    cg.add(var.set_num_rtd_wires(config[CONF_RTD_WIRES]))
