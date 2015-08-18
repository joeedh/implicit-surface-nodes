from . import c_code, symbol;
from math import *
import bmesh
from mathutils import *

from ctypes import *

def fract(f):
  return f - floor(f)

def perlin(x, y, z):
  return 0.0;

def perlin_dv(x, y, z, dx, dy, dz):
  return 0.0;
  
def surfmesh(expr, outobj, min=None, max=None):
  if min == None: min = Vector([-1, -1, -1])
  if max == None: max = Vector([1, 1, 1])
  
  min = outobj.matrix_world * min
  max = outobj.matrix_world * max
  
  loc = outobj.matrix_world * Vector()
  
  print("====generating shader bytecode=====")
  opcodes, constmap = expr.gen_opcodes(["_px", "_py", "_pz"])
  
  sm = c_code.create_stackmachine(opcodes, constmap)
  _px = -1
  _py = 0.2
  _pz = -4.0
  
  pyret = expr.run_opcodes(["_px", "_py", "_pz"], [_px, _py, _pz])
  
  print("sm.run ret: ", sm.run([-1, 0.2, -4.0]))
  print("stack: ", sm._get_stack(16), "\n")
  print("expr: ", str(expr))
  """
  try:
    print("EVAL RET:", eval(str(expr)))
  except ZeroDivisionError:
    print("EVAL RET: nan")
  """
  
  print("pymachine ret:", pyret.stack[pyret.stackcur-1])
  print("stack: ", pyret.stack[:16])
  
  _min = (c_float*3)()
  _max = (c_float*3)()
  
  for i in range(3):
    _min[i] = min[i]
    _max[i] = max[i]
  min = _min; max = _max;
  
  verts = POINTER(c_float)();
  totvert = c_int(0);
  tris = POINTER(c_int)()
  tottri = c_int(0)
  
  _lib = c_code._lib
  _lib.sm_set_sampler_machine(sm.sm, c_int(0));
  
  #void sm_tessellate(float **vertout, int *totvert, int **triout, int *tottri,
  #                   float min[3], float max[3], int ocdepth, int thread);

  _lib.sm_tessellate(byref(verts), byref(totvert), byref(tris), byref(tottri), min, max, c_int(5), c_int(0));
  
  print("\n\n")
  print("finished tessellating; totvert:", totvert.value, "tottri:", tottri.value)
  print("\n")
  print(verts, tris)

  bm = bmesh.new()
  vs = []
  for i in range(totvert.value):
    v = bm.verts.new([verts[i*3]-loc[0], verts[i*3+1]-loc[1], verts[i*3+2]-loc[2]])
    vs.append(v)
  
  bm.verts.index_update()
  
  print("opcode len:", len(opcodes))
  
  vlen = len(bm.verts)
  for i in range(tottri.value):
    #print(tris[i*3+2], len(bm.verts))
    v1 = tris[i*3]
    v2 = tris[i*3+1]
    v3 = tris[i*3+2]
    
    if (v3 >= vlen or v2 >= vlen or v1 >= vlen):
      print("BAD TRI!")
      continue;
      
    if v1 == v2 or v2 == v3 or v1 == v3:
      continue
      
    v1 = vs[v1]; v2 = vs[v2]; v3 = vs[v3]
    #print(v1, v2, v3)
    try:
      bm.faces.new([v1, v2, v3])
    except ValueError:
      pass
  
  _lib.sm_free_tess(verts, tris);
  
  bm.to_mesh(outobj.data)
  outobj.data.update()
    
    
    