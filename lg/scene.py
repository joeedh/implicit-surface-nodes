from . import c_code
import bpy

thescene = None

def initScene():
	global thescene

	if thescene is not None:
		thescene.free()
		thescene = None

	thescene = c_code.SceneGraph()
	thescene.initFromScene(bpy.context.scene)
	
	return thescene

def test():
	global thescene
	thescene.test()

def closeScene():
	global thescene

	if thescene is None: return

	thescene.free()
	thescene = None
