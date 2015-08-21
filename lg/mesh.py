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
  
def surfmesh(expr, outobj, min1=None, max1=None):
  if min1 == None: min1 = Vector([-1, -1, -1])
  if max1 == None: max1 = Vector([1, 1, 1])
  
  #"""
  rs = [
    Vector([min1[0], min1[1], min1[2]]),
    Vector([min1[0], max1[1], min1[2]]),
    Vector([max1[0], max1[1], min1[2]]),
    Vector([max1[0], min1[1], min1[2]]),
    Vector([min1[0], min1[1], max1[2]]),
    Vector([min1[0], max1[1], max1[2]]),
    Vector([max1[0], max1[1], max1[2]]),
    Vector([max1[0], min1[1], max1[2]]),
  ]
  
  min1 = outobj.matrix_world * min1
  max1 = outobj.matrix_world * max1
  
  for i in range(len(rs)):
    rs[i] = outobj.matrix_world * rs[i]
    
    for j in range(3):
      min1[j] = min(min1[j], rs[i][j])
      max1[j] = max(max1[j], rs[i][j])
  #"""
  
  mat = (c_float*16)()
  for i in range(4):
    for j in range(4):
      mat[j*4+i] = 0.0
  for i in range(4):
    mat[i*4+i] = 1
  
  loc = outobj.matrix_world * Vector()
  loc, rot, scale = rotmat_inv = outobj.matrix_world.decompose()
  
  tmat = Matrix.Translation(loc)
  tmat2 = Matrix(tmat)
  tmat.invert()
  
  mat1 = Matrix(outobj.matrix_world)
  #mat1 = rot.to_matrix();
  #mat1 = mat1.to_4x4()
  
  mat1.invert()
  #mat1 = Matrix()
  #mat1 = mat1 * tmat2;
  #mat1 = mat1 * tmat2;
  
  print("====generating shader bytecode=====")
  opcodes, constmap = expr.gen_opcodes(["_px", "_py", "_pz"])
  
  sm = c_code.create_stackmachine(opcodes, constmap)
  _px = -1
  _py = 0.2
  _pz = -4.0
  
  #pyret = expr.run_opcodes(["_px", "_py", "_pz"], [_px, _py, _pz])
  
  #print("sm.run ret: ", sm.run([-1, 0.2, -4.0]))
  #print("stack: ", sm._get_stack(16), "\n")
  
  #print("expr: ", str(expr))
  """
  try:
    print("EVAL RET:", eval(str(expr)))
  except ZeroDivisionError:
    print("EVAL RET: nan")
  """
  
  #print("pymachine ret:", pyret.stack[pyret.stackcur-1])
  #print("stack: ", pyret.stack[:16])
  
  _min = (c_float*3)()
  _max = (c_float*3)()
  
  for i in range(3):
    _min[i] = min1[i]
    _max[i] = max1[i]
  min1 = _min; max1 = _max;
  
  verts = POINTER(c_float)();
  totvert = c_int(0);
  tris = POINTER(c_int)()
  tottri = c_int(0)
  
  _lib = c_code._lib
  _lib.sm_set_sampler_machine(sm.sm);
  
  #void sm_tessellate(float **vertout, int *totvert, int **triout, int *tottri,
  #                   float min[3], float max[3], int ocdepth, int thread);

  _lib.sm_tessellate(byref(verts), byref(totvert), byref(tris), byref(tottri), 
                     min1, max1, c_int(7), mat, c_int(0));
  
  #return#XXX
  
  print("\n\n")
  print("finished tessellating; totvert:", totvert.value, "tottri:", tottri.value)
  print("\n")
  print(verts, tris)

  bm = bmesh.new()
  vs = []
  for i in range(totvert.value):
    co = Vector([verts[i*3], verts[i*3+1], verts[i*3+2]])
    co = mat1 * co
    v = bm.verts.new(co)
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
      f = bm.faces.new([v1, v2, v3])
      f.smooth = True
    except ValueError:
      pass
  
  _lib.sm_free_tess(verts, tris);
  
  bm.to_mesh(outobj.data)
  outobj.data.update()
    
    
    