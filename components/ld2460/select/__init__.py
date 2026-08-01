import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from .. import LD2460Component, ld2460_ns

DEPENDENCIES = ["ld2460"]

CONF_LD2460_ID = "ld2460_id"
CONF_INSTALLATION_MODE = "installation_mode"

LD2460InstallationModeSelect = ld2460_ns.class_(
    "LD2460InstallationModeSelect", select.Select
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(cg.EntityBase),
        cv.GenerateID(CONF_LD2460_ID): cv.use_id(LD2460Component),
        cv.Optional(CONF_INSTALLATION_MODE): select.select_schema(
            LD2460InstallationModeSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
    }
)


async def to_code(config):
    if install_config := config.get(CONF_INSTALLATION_MODE):
        mode_select = cg.new_Pvariable(install_config[CONF_ID])
        await select.register_select(
            mode_select, install_config, options=["side", "top"]
        )
        await cg.register_parented(mode_select, config[CONF_LD2460_ID])
        hub = await cg.get_variable(config[CONF_LD2460_ID])
        cg.add(hub.set_installation_mode_select(mode_select))
