import bpy

from .utils import Registrar

#ok, I imagine we're going to have to build a scene dependency graph
#here

class SceneNode:
  def __init__(self, scene, object):
    self.scene = scene
    self.object = object
  
class Effector (SceneNode):
  pass

class ImplicitDomain (SceneNode):
  pass

class PolygonMeshes (SceneNode):
  pass

bpy_exports = Registrar([
]);
