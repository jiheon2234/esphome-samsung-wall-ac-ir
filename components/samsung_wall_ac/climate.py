import esphome.codegen as cg
from esphome.components import climate_ir

AUTO_LOAD = ["climate_ir"]
CODEOWNERS = ["@jiheon2234"]

samsung_wall_ac_ns = cg.esphome_ns.namespace("samsung_wall_ac")
SamsungWallAC = samsung_wall_ac_ns.class_("SamsungWallAC", climate_ir.ClimateIR)

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(SamsungWallAC)


async def to_code(config):
    await climate_ir.new_climate_ir(config)
