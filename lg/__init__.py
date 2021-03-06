import traceback

__all__ = [
  "utils",
  "sector",
  "generate",
  "mesh",
  "node", 
  "node_tree",
  "node_categories",
  "ops",
  "symbol",
  'codegen',
  'symbol_funcs',
  'c_code',
  'symbol_factor',
  'props',
  'panel_ui',
  'symbol_optimize',
  'event',
  'fields',
  'appstate',
  'config',
  'scene',
  "codegen_intel"
];

import imp
from . import config

imp.reload(config)

from . import utils, sector, mesh, node_tree, ops, symbol, codegen, \
              c_code, props, panel_ui, event, fields, appstate, generate,\
              scene, codegen_intel

imp.reload(c_code)
imp.reload(appstate)

try:
  node_tree.bpy_exports.unregister()
  ops.bpy_exports.unregister()
  event.bpy_exports.unregister()
  panel_ui.bpy_exports.unregister()
except:
  traceback.print_exc()
  print("Error unregistered add-on")

c_code.close_library()
imp.reload(c_code)

imp.reload(symbol)
imp.reload(utils)
imp.reload(codegen)
imp.reload(codegen_intel)

imp.reload(props)
imp.reload(node_tree)

imp.reload(mesh)
imp.reload(sector)
imp.reload(scene)
imp.reload(event)
imp.reload(fields)

imp.reload(ops)

imp.reload(panel_ui)
imp.reload(generate)
imp.reload(scene)
bpy_exports = utils.Registrar([
  node_tree.bpy_exports,
  ops.bpy_exports,
  props.bpy_exports,
  panel_ui.bpy_exports,
  event.bpy_exports,
  fields.bpy_exports
])

def register():
  bpy_exports.register()
  
def unregister():
  bpy_exports.unregister()
