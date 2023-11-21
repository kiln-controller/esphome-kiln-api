import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.components import web_server
from esphome.components import web_server_base
from esphome.components.pid import climate

AUTO_LOAD = ["web_server_base", "pid"]

kiln_api_ns = cg.esphome_ns.namespace("kiln_api")
KilnApiHandler = kiln_api_ns.class_("KilnApi", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(KilnApiHandler),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(
            web_server_base.WebServerBase
        ),
        cv.GenerateID("kiln"): cv.use_id(
            climate.PIDClimate
        ),
    },
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    web_base = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    climate = await cg.get_variable(config["kiln"])

    var = cg.new_Pvariable(config[CONF_ID], web_base, climate)
    await cg.register_component(var, config)
