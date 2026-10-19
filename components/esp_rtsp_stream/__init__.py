import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_URL

DEPENDENCIES = ["esp32"]
AUTO_LOAD = []

rtsp_mjpeg_ns = cg.esphome_ns.namespace("rtsp_mjpeg")
RtspMjpeg = rtsp_mjpeg_ns.class_("RtspMjpeg", cg.Component)

CONF_FPS = "fps"
CONF_EXPECTED_WIDTH = "expected_width"
CONF_EXPECTED_HEIGHT = "expected_height"
CONF_LOG_FRAME_INFO = "log_frame_info"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RtspMjpeg),
    cv.Required(CONF_URL): cv.string,
    cv.Optional(CONF_FPS, default=10): cv.int_range(min=1, max=30),
    cv.Optional(CONF_EXPECTED_WIDTH, default=704): cv.int_range(min=1, max=4096),
    cv.Optional(CONF_EXPECTED_HEIGHT, default=576): cv.int_range(min=1, max=2160),
    cv.Optional(CONF_LOG_FRAME_INFO, default=True): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_url(config[CONF_URL]))
    cg.add(var.set_fps(config[CONF_FPS]))
    cg.add(var.set_expected_size(
        config[CONF_EXPECTED_WIDTH], config[CONF_EXPECTED_HEIGHT]
    ))
    cg.add(var.set_log_frame_info(config[CONF_LOG_FRAME_INFO]))
