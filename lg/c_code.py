import bpy, struct, os, os.path, sys, time, random
from ctypes import *
from math import *

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
    
  _lib = CDLL(r'C:\dev\level_game\blender\ccode\libsurface.so')
  if _lib == None:
      _lib = cdll.LoadLibrary(r'C:\dev\level_game\blender\ccode\libsurface.so')
  
  _lib.sm_get_functable.restype = POINTER(FuncTableItem)
  _lib.sm_new.restype = POINTER(c_int);
  _lib.sm_get_stackitem.restype = c_float;
  _lib.sm_run.restype = c_float;
  
open_library()

def close_library():
    global _lib
    if _lib == None or _lib._handle == None: return
    
    handle = _lib._handle
    
    print("closing c helper library. . .")
    
    ret = windll.kernel32.FreeLibrary(handle)
    
    while ret != 0:
        ret = windll.kernel32.FreeLibrary(handle)
    ret = windll.kernel32.FreeLibrary(handle)
    ret = windll.kernel32.FreeLibrary(handle)
    ret = windll.kernel32.FreeLibrary(handle)
    print(ret) 
    _lib = None

funcmap = {}

def build_funcmap():
  global _lib, funcmap
  
  funcmap = {}
  
  size = c_int()
  funcs = _lib.sm_get_functable(byref(size))
  
  print("querying function table", size, funcs)
  
  for i in range(size.value):
    name = str(funcs[i].name, "latin-1")
    funcmap[name] = i

class StackMachine:
  def __init__(self, sm, opcodes):
    self.sm = sm
    self.opcodes = opcodes
    self.oplen = len(opcodes)
  
  def __del__(self):
    _lib.sm_free(self.sm)
    
  def _get_stack(self, count=16):
    global _lib
    
    ret = []
    for i in range(count):
      val = _lib.sm_get_stackitem(self.sm, c_int(i))
      ret.append(val)
      
    return ret;
    
  def run(self, globals):
    global _lib
    for i, g in enumerate(globals):
      _lib.sm_set_global(self.sm, c_int(i), c_float(g))
    
    _lib.sm_set_stackcur(self.sm, c_int(len(globals)+3)) #add safety buffer around globals
    return _lib.sm_run(self.sm, self.opcodes, c_int(self.oplen))
    
def create_stackmachine(opcodes, consts):
  global _lib, funcmap 
  
  build_funcmap()
  
  #SMOpCode
  sm = _lib.sm_new();
  for f in consts:
    print(f, sm)
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
  
  _lib.sm_add_opcodes(sm, codes, c_int(len(codes)))
  
  return StackMachine(sm, codes)
  