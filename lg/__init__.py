import traceback

__all__ = [
  "utils",
  "sector",
  "mesh",
  "node", 
  "node_tree",
  "node_categories",
  "ops",
  "symbol",
  'codegen',
  'symbol_funcs',
  'c_code',
  'symbol_factor'
];

import imp
from . import utils, sector, mesh, node_tree, ops, symbol, codegen, c_code

try:
  node_tree.bpy_exports.unregister()
  ops.bpy_exports.unregister()
except:
  traceback.print_exc()
  print("Error unregistered add-on")

c_code.close_library()
imp.reload(c_code)

imp.reload(symbol)
imp.reload(utils)
imp.reload(codegen)

imp.reload(mesh)
imp.reload(sector)
imp.reload(node_tree)
imp.reload(ops)

bpy_exports = utils.Registrar([
  node_tree.bpy_exports,
  ops.bpy_exports
])

def register():
  bpy_exports.register()
  
def unregister():
  bpy_exports.unregister()
