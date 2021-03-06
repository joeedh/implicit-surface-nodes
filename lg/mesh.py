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
  
def surfmesh(expr, outobj, outmatrix=None, use_local=False, min1=None, max1=None):
  if outmatrix is None:
    outmatrix = Matrix()
    outmatrix.resize_4x4()
    
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
  
  #local coordinates?
  if use_local:
    """
    min1 = outmatrix @ min1
    max1 = outmatrix @ max1
    
    for i in range(len(rs)):
      rs[i] = outobj.matrix_world @ rs[i]
      
      for j in range(3):
        min1[j] = min(min1[j], rs[i][j])
        max1[j] = max(max1[j], rs[i][j])
        
    size = (max1-min1);
    for j in range(3):
      min1[j] = -size[j]*0.55
      max1[j] =  size[j]*0.55
    
    print(min1, max1)
    #raise "sdf"
    #"""
  else:
    min1 = outmatrix @ Vector(min1)
    max1 = outmatrix @ Vector(max1)
    
    for i in range(len(rs)):
      rs[i] = outobj.matrix_world @ rs[i]
      
      for j in range(3):
        min1[j] = min(min1[j], rs[i][j])
        max1[j] = max(max1[j], rs[i][j])
  #"""
  
  if not use_local:
    #identity matrix
    mat = (c_float*16)()
    for i in range(4):
      for j in range(4):
        mat[j*4+i] = 0.0
    for i in range(4):
      mat[i*4+i] = 1
  else:
    scale = outmatrix.decompose()[2]
    smat = Matrix.Scale(scale[0], 4, [1, 0, 0])
    smat = Matrix.Scale(scale[1], 4, [0, 1, 0])
    smat = Matrix.Scale(scale[2], 4, [0, 0, 1])
    
    wmat = smat#Matrix() #outmatrix)
    #print(wmat)
    #wmat.invert()
    #print(wmat)
    #raise "sdf"
    
    #wmat = Matrix()
    #wmat.resize_4x4()
    
    #wmat = Matrix.Translation(Vector([0,2,0]))
    #wmat = wmat @ Matrix.Scale(1.5, 4)
    
    #wmat.resize_4x4()
    mat = (c_float*16)()
    for i in range(4):
      for j in range(4):
        mat[j*4+i] = wmat[i][j]
        
  loc = outobj.matrix_world @ Vector()
  loc, rot, scale = rotmat_inv = outobj.matrix_world.decompose()
  
  tmat = Matrix.Translation(loc)
  tmat2 = Matrix(tmat)
  tmat.invert()
  
  mat1 = Matrix(outobj.matrix_world)
  mat1.invert()
  
  print("====generating shader bytecode=====")
  opcodes, constmap = expr.gen_opcodes(["_px", "_py", "_pz", "_field"])
  
  sm = c_code.create_stackmachine(opcodes, constmap)
  
  _min = (c_float*3)()
  _max = (c_float*3)()
  
  for i in range(3):
    _min[i] = min1[i]
    _max[i] = max1[i]
  min1 = _min; max1 = _max;
  
  verts = POINTER(c_float)();
  ao_out = POINTER(c_float)();
  totvert = c_int(0);
  tris = POINTER(c_int)()
  tottri = c_int(0)
  
  _lib = c_code._lib
  print("SM", sm.sm);
  _lib.sm_set_sampler_machine(sm.sm);
  
  #void sm_tessellate(sg, float **vertout, int *totvert, int **triout, int *tottri,
  #                   float min[3], float max[3], int ocdepth, int thread);

  print("MAX", max1[0], max1[1], max1[2]);

  from . import scene
  sg = scene.thescene.handle

  _lib.sm_tessellate(c_voidp(sg), byref(verts), byref(ao_out), byref(totvert), byref(tris), byref(tottri), 
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
    
    if use_local:
      co = outmatrix @ co
    else:
      co = mat1 @ co
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
    
    
    