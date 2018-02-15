import bpy, struct, os, os.path, sys, time, random
from ctypes import *
from math import *
from . import config
import bmesh
from mathutils import *

from . import codegen

OpEnum = [
  'PUSH', #args: register to push onto stack
  'POP',  #args: register to pop stack value onto
  'CONST_TO_REG',
  'CONST_TO_STK',
  'ABS_STK_TO_REG',
  'REG_TO_STK', #args: [stack loc, register loc]
  'STK_TO_REG', #args: [stack loc, register loc]
  'MUL',  #args: [register a, register b, register out]
  'DIV',  #args: [register a, register b, register out]
  'ADD',  #args: [register a, register b, register out]
  'SUB',  #args: [register a, register b, register out]
  'GT',   #args: [register a, register b, register out]
  'LT',   #args: [register a, register b, register out]
  'GTE',  #args: [register a, register b, register out]
  'LTE',  #args: [register a, register b, register out]
  'EQ',   #args: [register a, register b, register out]
  'LOGICAL_NEGATE',  #args: [register a, register b, register out]
  'NEGATE', #args: [register a, register out]
  'POW',    #args: [register a, register out]
  'FUNC_CALL', #args: [func name id]
  'PUSH_GLOBAL',
  'LOAD_GLOBAL'
];
  
class FuncTableItem (Structure):
  _fields_  = [
    ("functionptr", c_voidp),
    ("name", c_char_p),
    ("totarg", c_int)
  ]

class SMOpCode (Structure):
  _fields_  = [
    ("code", c_short),
    ("arg1", c_short),
    ("arg2", c_short),
    ("arg3", c_short)
  ]

_lib = None
def open_library():
  global _lib
  
  if _lib != None:
    print("c library already opened")
    return
  
 
  path = config.libsurface_path

  print("\nUsing library " + path + " exists:", os.path.exists(path), "\n")
  _lib = windll.LoadLibrary(path)

  #if _lib == None:
  #    _lib = cdll.LoadLibrary(r'C:\dev\level_game\blender\ccode\libsurface.so')
  
  _lib.sm_get_functable.restype = POINTER(FuncTableItem)
  _lib.sm_new.restype = POINTER(c_long);
  _lib.sm_get_stackitem.restype = c_float;

  _lib.sg_new.restype = c_voidp;
  _lib.so_create_mesh.restype = c_voidp;
  _lib.so_create_object.restype = c_voidp;
  _lib.so_get_stackmachine.restype = c_voidp;

  _lib.sg_sample.restype = c_float

  #_lib.sm_run.restype = c_float;
  
open_library()

def close_library():
    global _lib
    if _lib == None or _lib._handle == None: return
    
    handle = _lib._handle
    
    print("closing c helper library. . .")
    
    ret = windll.kernel32.FreeLibrary(c_longlong(handle))

    if ret == 0:
        print("Error closing library! GetLastError():", windll.kernel32.GetLastError())
        return

    print(ret)
    
    while ret != 0:
        ret = windll.kernel32.FreeLibrary(c_longlong(handle))
        print(ret)
        
    for i in range(8):
      ret = windll.kernel32.FreeLibrary(c_longlong(handle))
      #time.sleep(.006)
      #print(ret)
    print(ret)

    _lib = None

funcmap = {}

def build_funcmap():
  global _lib, funcmap
  
  funcmap = {}
  
  size = c_int()
  funcs = _lib.sm_get_functable(byref(size))
  
  #print("querying function table", size, funcs)
  
  for i in range(size.value):
    name = str(funcs[i].name, "latin-1")
    funcmap[name] = i

class StackMachine:
  def __init__(self, sm, opcodes):
    self.sm = sm
    self.opcodes = opcodes
    self.oplen = len(opcodes)
  
  def __del__(self):
    #_lib.sm_free(self.sm)
    pass
    
  def _get_stack(self, count=16):
    raise RuntimeError()
    global _lib
    
    ret = []
    for i in range(count):
      val = _lib.sm_get_stackitem(self.sm, c_int(i))
      ret.append(val)
      
    return ret;
    
  def run(self, globals):
    global _lib
    raise RuntimeError()
    
    for i, g in enumerate(globals):
      _lib.sm_set_global(self.sm, c_int(i), c_float(g))
    
    _lib.sm_set_stackcur(self.sm, c_int(len(globals)+3)) #add safety buffer around globals
    return _lib.sm_run(self.sm, self.opcodes, c_int(self.oplen))
    
def create_stackmachine(opcodes, consts, sm=None):
  global _lib, funcmap 
  
  build_funcmap()
  
  #SMOpCode
  if sm is None:
    sm = _lib.sm_new();

  for f in consts:
    #print(f, sm)
    _lib.sm_add_constant(sm, c_float(f))
  
  codes = (SMOpCode*len(opcodes))()
  for i, op in enumerate(opcodes):
    if op[0] == "FUNC_CALL" and op[1] not in funcmap:
      raise RuntimeError("ERROR: C runtime lacks standard function " + op[1])
      
    c = codes[i]
    c.code = OpEnum.index(op[0])
    
    if op[0] == "FUNC_CALL":
      c.arg1 = funcmap[op[1]]
    else:
      c.arg1 = op[1]
      
    c.arg2 = op[2]
    c.arg3 = op[3]
  
  _lib.sm_add_opcodes(c_voidp(sm), codes, c_int(len(codes)))
  
  return StackMachine(sm, codes)
  
class SceneObject:
	def __init__(self, blenderob, handle=None):
		self.handle = handle
		self.mesh = None
		self.ob = blenderob;

	def __del__(self):
		if self.handle is not None:
			_lib.so_free_object(c_voidp(self.handle))
			self.handle = None

	def free(self):
		if self.handle is not None:
			_lib.so_free_object(c_voidp(self.handle))
			self.handle = None

	def genOpCodes(self):
		if self.handle is None:
			print("error! genOpCodes() called on zombie object!")
			return

		ob = self.ob

		ntree = ob.implicit.node_tree
		if ntree not in bpy.data.node_groups.keys(): return
		ntree = bpy.data.node_groups[ntree]

		cgen = codegen.CodeGen(ntree)
		cgen.sort()

		r = cgen.gen()
		if type(r) == list and len(r) > 0:
			r = r[0]

		if type(r) == list:
			return #no code
		
		r = r.factor()

		opcodes, constmap = r.gen_opcodes(["_px", "_py", "_pz", "_field"]);
		sm = _lib.so_get_stackmachine(c_voidp(self.handle))

		print(opcodes, constmap)
		create_stackmachine(opcodes, constmap, sm)

class SceneMesh:
	def __init__(self, handle=None):
		self.handle = handle

	def __del__(self):
		if self.handle is not None:
			#_lib.so_free_mesh(c_voidp(self.handle))
			self.handle = None

	def free(self):
		if self.handle is not None:
			_lib.so_free_mesh(c_voidp(self.handle))
			self.handle = None

class SceneGraph:
	def __init__(self):
		self.handle = _lib.sg_new();
		self.objects = []
		self.meshes = []
		self.meshmap = {}
		self.objectmap = {}

	def __del__(self):
		if self.handle is not None:
			_lib.sg_free(c_voidp(self.handle));
			self.handle = None
		
	def free(self):
		if self.handle is not None:
			_lib.sg_free(c_voidp(self.handle))
			self.handle = None
	
	def test(self, x=0, y=0, z=0):
		co = (c_float*3)()
		co[0] = x
		co[1] = y
		co[2] = z

		f = _lib.sg_sample(c_voidp(self.handle), co)

		print("TEST result:", f);

	def initFromScene(self, scene):
		print("Creating implicit scene. . .")
		obs = []
		meshes = []
		meshset = set()

		def ob_visible(ob):
			if ob.hide: return False

			for i in range(len(ob.layers)):
				if ob.layers[i] and scene.layers[i]:
					return True
			return False

		for ob in scene.objects:
			if ob.name.startswith("__"): continue
			if ob.type != "MESH": continue

			if not ob_visible(ob):
				continue

			obs.append(ob)
			if ob.data.name not in meshset:
				meshes.append(ob.data)
				meshset.add(ob.data.name)

		print(obs, meshes)

		for ob in obs:
			bm = bmesh.new()
			bm.from_object(ob, scene)
			tris = bm.calc_tessface()

			verts = (c_float * (len(bm.verts) * 3))()
			outtris = (c_int * (len(tris) * 3))()

			i = 0
			for v in bm.verts:
				verts[i] = v.co[0]
				verts[i+1] = v.co[1]
				verts[i+2] = v.co[2]
				i += 3
			
			bm.verts.index_update()
			i = 0
			for tri in tris:
				outtris[i] = tri[0].vert.index
				outtris[i+1] = tri[1].vert.index
				outtris[i+2] = tri[2].vert.index
				i += 3

			m = _lib.so_create_mesh(ob.data.name, len(tris), len(bm.verts), 0, verts, outtris, None, None);

			m = SceneMesh(m)
			self.meshes.append(m)

			#MYEXPORT SceneObject *so_create_object(char name[32], SceneMesh *mesh, float *co, float *rot, float *scale, float *matrix, int mode) {
			co = (c_float * 3)()
			rot = (c_float*3)()
			scale = (c_float*3)()
			matrix = (c_float*16)()

			for i in range(3):
				co[i] = ob.location[i]
				rot[i] = ob.rotation_euler[i]
				scale[i] = ob.scale[i]

			mat = ob.matrix_world

			for i in range(4):
				for j in range(4):
					matrix[j*4+i] = mat[i][j];

			mode = ob.implicit.blend_mode
			mode = 1 if mode == 'POSITIVE' else 0

			sob = _lib.so_create_object(ob.name, c_voidp(m.handle), co, rot, scale, matrix, c_int(mode));
			sob = SceneObject(ob, sob)
			sob.genOpCodes()

			self.objects.append(sob)
			self.objectmap[ob.name] = sob

			_lib.sg_add_object(c_voidp(self.handle), c_voidp(sob.handle), mode);

