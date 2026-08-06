import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DISTANCE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DEGREES,
    UNIT_METER,
)

from .. import InstallationParameterType, LD2460Component, ld2460_ns

DEPENDENCIES = ["ld2460"]

CONF_LD2460_ID = "ld2460_id"
CONF_INSTALLATION_HEIGHT = "installation_height"
CONF_INSTALLATION_ANGLE = "installation_angle"

LD2460InstallationParameterNumber = ld2460_ns.class_(
    "LD2460InstallationParameterNumber", number.Number
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LD2460_ID): cv.use_id(LD2460Component),
        cv.Optional(CONF_INSTALLATION_HEIGHT): number.number_schema(
            LD2460InstallationParameterNumber,
            unit_of_measurement=UNIT_METER,
            device_class=DEVICE_CLASS_DISTANCE,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_INSTALLATION_ANGLE): number.number_schema(
            LD2460InstallationParameterNumber,
            unit_of_measurement=UNIT_DEGREES,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LD2460_ID])

    if height_config := config.get(CONF_INSTALLATION_HEIGHT):
        n = cg.new_Pvariable(
            height_config[CONF_ID], InstallationParameterType.HEIGHT
        )
        await number.register_number(
            n, height_config, min_value=0.1, max_value=10.0, step=0.01
        )
        await cg.register_parented(n, config[CONF_LD2460_ID])
        cg.add(hub.set_installation_height_number(n))

    if angle_config := config.get(CONF_INSTALLATION_ANGLE):
        n = cg.new_Pvariable(
            angle_config[CONF_ID], InstallationParameterType.ANGLE
        )
        await number.register_number(
            n, angle_config, min_value=0.0, max_value=90.0, step=0.1
        )
        await cg.register_parented(n, config[CONF_LD2460_ID])
        cg.add(hub.set_installation_angle_number(n))
