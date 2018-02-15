import bpy
from math import *

from . import utils

def blend_mode_update(self, context):
  print("blend mode update!", self, context)
  
class ObjectProps (bpy.types.PropertyGroup):
  items = [
    ("POSITIVE", "Positive", "Simple arithmetic add"),
    ("NEGATIVE", "Negative", "Simple arithmetic subtract"),
    ("ISECT", "CSG Intersect", "CSG Intersect"),
    ("UNION", "CSG Union", "CSG Union"),
    ("SUBTRACT", "CSG Subtract", "CSG Subtract")
  ]
  
  node_tree = bpy.props.StringProperty(name="Node Tree")
  surface_groups = bpy.props.BoolVectorProperty(name="Surface Groups", size=32, default=[True for x in range(32)], subtype='LAYER')
  blend_mode = bpy.props.EnumProperty(name="Blend Mode", items=items, update=blend_mode_update)
  
  global_mode = bpy.props.BoolProperty(name="Global", default=False, description="Use global coordinates")
  
def register_obj_props():
  bpy.types.Object.implicit = bpy.props.PointerProperty(type=ObjectProps)
  
bpy_exports = utils.Registrar([
  ObjectProps,
  utils.CustomRegister(
    register_obj_props,
    None
  )
]);

