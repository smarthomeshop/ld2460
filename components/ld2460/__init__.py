import esphome.codegen as cg
from esphome.components import binary_sensor, sensor, text_sensor, uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_OCCUPANCY,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_DEGREES,
    UNIT_METER,
)

AUTO_LOAD = ["binary_sensor", "number", "sensor", "text_sensor"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

ld2460_ns = cg.esphome_ns.namespace("ld2460")
LD2460Component = ld2460_ns.class_(
    "LD2460Component", cg.Component, uart.UARTDevice
)
InstallationParameterType = ld2460_ns.enum(
    "InstallationParameterType", is_class=True
)

CONF_RAW = "raw"
CONF_BAUD_SCAN = "baud_scan"
CONF_BYTE_COUNT = "byte_count"
CONF_ENABLE_REPORTING = "enable_reporting"
CONF_FLUSH_TIMEOUT = "flush_timeout"
CONF_MAX_BUFFER_SIZE = "max_buffer_size"
CONF_NO_DATA_LOG_INTERVAL = "no_data_log_interval"
CONF_PUBLISH_INTERVAL = "publish_interval"
CONF_REPORT_LOG_INTERVAL = "report_log_interval"
CONF_PRESENCE = "presence"
CONF_SUMMARY = "summary"
CONF_TARGET_COUNT = "target_count"
CONF_FIRMWARE = "firmware"
CONF_INSTALLATION_MODE = "installation_mode"
CONF_STARTUP_INSTALLATION_MODE = "startup_installation_mode"
CONF_X = "x"
CONF_Y = "y"
CONF_DISTANCE = "distance"
CONF_ANGLE = "angle"
CONF_TARGETS = [f"target_{index}" for index in range(1, 6)]

INSTALLATION_MODES = {
    "unchanged": 0,
    "side": 1,
    "top": 2,
}

TARGET_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_X): sensor.sensor_schema(
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_DISTANCE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_Y): sensor.sensor_schema(
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_DISTANCE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DISTANCE): sensor.sensor_schema(
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_DISTANCE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_ANGLE): sensor.sensor_schema(
            unit_of_measurement=UNIT_DEGREES,
            icon="mdi:angle-acute",
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LD2460Component),
            cv.Optional(CONF_BAUD_SCAN, default=True): cv.boolean,
            cv.Optional(
                CONF_STARTUP_INSTALLATION_MODE, default="unchanged"
            ): cv.enum(INSTALLATION_MODES, lower=True),
            cv.Optional(CONF_RAW): text_sensor.text_sensor_schema(
                icon="mdi:serial-port",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_SUMMARY): text_sensor.text_sensor_schema(
                icon="mdi:radar",
            ),
            cv.Optional(CONF_FIRMWARE): text_sensor.text_sensor_schema(
                icon="mdi:chip",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_INSTALLATION_MODE): text_sensor.text_sensor_schema(
                icon="mdi:radar",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_PRESENCE): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_OCCUPANCY,
            ),
            cv.Optional(CONF_TARGET_COUNT): sensor.sensor_schema(
                icon=ICON_COUNTER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_BYTE_COUNT): sensor.sensor_schema(
                unit_of_measurement="B",
                icon=ICON_COUNTER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            **{cv.Optional(target): TARGET_SCHEMA for target in CONF_TARGETS},
            cv.Optional(CONF_ENABLE_REPORTING, default=True): cv.boolean,
            cv.Optional(CONF_FLUSH_TIMEOUT, default="100ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_MAX_BUFFER_SIZE, default=48): cv.int_range(min=1, max=512),
            cv.Optional(CONF_NO_DATA_LOG_INTERVAL, default="10s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_PUBLISH_INTERVAL, default="500ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_REPORT_LOG_INTERVAL, default="1s"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "ld2460",
    baud_rate=115200,
    require_tx=True,
    require_rx=True,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    if wake_loop_on_rx := getattr(uart, "request_wake_loop_on_rx", None):
        wake_loop_on_rx()

    if raw_config := config.get(CONF_RAW):
        sens = await text_sensor.new_text_sensor(raw_config)
        cg.add(var.set_raw_text_sensor(sens))

    if summary_config := config.get(CONF_SUMMARY):
        sens = await text_sensor.new_text_sensor(summary_config)
        cg.add(var.set_summary_text_sensor(sens))

    if firmware_config := config.get(CONF_FIRMWARE):
        sens = await text_sensor.new_text_sensor(firmware_config)
        cg.add(var.set_firmware_text_sensor(sens))

    if installation_mode_config := config.get(CONF_INSTALLATION_MODE):
        sens = await text_sensor.new_text_sensor(installation_mode_config)
        cg.add(var.set_installation_mode_text_sensor(sens))

    if presence_config := config.get(CONF_PRESENCE):
        sens = await binary_sensor.new_binary_sensor(presence_config)
        cg.add(var.set_presence_binary_sensor(sens))

    if target_count_config := config.get(CONF_TARGET_COUNT):
        sens = await sensor.new_sensor(target_count_config)
        cg.add(var.set_target_count_sensor(sens))

    if byte_count_config := config.get(CONF_BYTE_COUNT):
        sens = await sensor.new_sensor(byte_count_config)
        cg.add(var.set_byte_count_sensor(sens))

    for index, target_key in enumerate(CONF_TARGETS):
        target_config = config.get(target_key)
        if target_config is None:
            continue

        if x_config := target_config.get(CONF_X):
            sens = await sensor.new_sensor(x_config)
            cg.add(var.set_target_x_sensor(index, sens))

        if y_config := target_config.get(CONF_Y):
            sens = await sensor.new_sensor(y_config)
            cg.add(var.set_target_y_sensor(index, sens))

        if distance_config := target_config.get(CONF_DISTANCE):
            sens = await sensor.new_sensor(distance_config)
            cg.add(var.set_target_distance_sensor(index, sens))

        if angle_config := target_config.get(CONF_ANGLE):
            sens = await sensor.new_sensor(angle_config)
            cg.add(var.set_target_angle_sensor(index, sens))

    cg.add(var.set_flush_timeout(config[CONF_FLUSH_TIMEOUT].total_milliseconds))
    cg.add(var.set_max_buffer_size(config[CONF_MAX_BUFFER_SIZE]))
    cg.add(var.set_baud_scan(config[CONF_BAUD_SCAN]))
    cg.add(
        var.set_startup_installation_mode(config[CONF_STARTUP_INSTALLATION_MODE])
    )
    cg.add(var.set_enable_reporting(config[CONF_ENABLE_REPORTING]))
    cg.add(var.set_no_data_log_interval(config[CONF_NO_DATA_LOG_INTERVAL].total_milliseconds))
    cg.add(var.set_publish_interval(config[CONF_PUBLISH_INTERVAL].total_milliseconds))
    cg.add(var.set_report_log_interval(config[CONF_REPORT_LOG_INTERVAL].total_milliseconds))
