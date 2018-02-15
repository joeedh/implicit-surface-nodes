import bpy

from .utils import Registrar

#ok, I imagine we're going to have to build a scene dependency graph
#here

class SceneNode:
  def __init__(self, scene, object):
    self.scene = scene
    self.object = object

def build_scene(scene):
  obs = []
  nkeys = bpy.data.node_groups.keys()
  
  for ob in scene.objects:
    if ob.implicit.node_tree in nkeys:
      obs.append(ob)
  
  for ob in obs:
    ntree = bpy.data.node_groups[ob.implicit.node_tree]
    
  
bpy_exports = Registrar([

]);
