have_sympy = True
try:
    import sympy
except:
    have_sympy = False
     
import lg
print("have sympy:", have_sympy)

from math import *

prec = {
  "&&"  : -2,
  "||"  : -2,
  ">"   : -1,
  "<"   : -1,
  ">="  : -1,
  "<="  : -1,
  "**"  : 0,
  "/"   : 1,
  "*"   : 1,
  "+"   : 2,
  "-"   : 2,
}

opmap = {
  "&&"  : "LAND",
  "||"  : "LOR",
  ">"   : "GT",
  "<"   : "LT",
  ">="  : "GTE",
  "<="  : "LTE",
  "**"  : "POW",
  "/"   : "DIV",
  "*"   : "MUL",
  "+"   : "ADD",
  "-"   : "SUB",
}

class StackMachine:
  def __init__(self, globals, consts):
    self.stack = list(range(1024*32))
    self.stackcur = 0
    
    self.constants = []
    
    for g in globals:
      self.stack[self.stackcur] = g
      self.stackcur += 1
    
    for i in range(4):
      self.stack[self.stackcur+i] = -16
    self.stackcur += 4

    for c in consts:
      self.constants.append(c)
    
    self.registers = [0 for x in range(8)]
 
  def run(self, code):
    for c in code:
      s = c[0] + " " + str(c[1]) + " " + str(c[2]) + " " + str(c[3])
      s += "\n  STACK: "
      
      for i in range(self.stackcur-3, self.stackcur):
        s += "%.1f " % self.stack[i]
      s += "REG: "
      for i in range(4):
        s += "%.1f " % self.registers[i]
        
      #print(s)
      
      getattr(self, c[0])(c[1], c[2], c[3])
    return self
    
  def PUSH(self, arg1, arg2, arg3):
    self.stack[self.stackcur] = self.registers[arg1];
    self.stackcur += 1
    
  def POP(self, arg1, arg2, arg3):
    self.stackcur -= 1
    self.registers[arg1] = self.stack[self.stackcur];
    
  def REG_TO_STK(self, arg1, arg2, arg3):
    loc = arg1 | arg2 << 16;
    self.stack[self.stackcur-1-loc] = self.registers[arg3];
    
  def CONST_TO_REG(self, arg1, arg2, arg3):
    self.registers[arg1] = self.constants[arg2];
    
  def CONST_TO_STK(self, arg1, arg2, arg3):
    loc = arg1 | arg2 << 16;
    self.stack[self.stackcur-1-loc] = self.constants[arg3];
    
  def ABS_STK_TO_REG(self, arg1, arg2, arg3):
    loc = arg1 | arg2 << 16;
    #print(arg1, arg2, "|", arg3)
    self.registers[arg3] = self.stack[loc];
    
  def STK_TO_REG(self, arg1, arg2, arg3):
    loc = arg1 | arg2 << 16;
    self.registers[arg3] = self.stack[self.stackcur-1-loc];
    
  def MUL(self, arg1, arg2, arg3):
    self.registers[arg3] = self.registers[arg1] * self.registers[arg2];
    
  def DIV(self, arg1, arg2, arg3):
    if self.registers[arg2] == 0.0:
      self.registers[arg3] = 0.0
    else:
      self.registers[arg3] = self.registers[arg1] / self.registers[arg2];
    
  def ADD(self, arg1, arg2, arg3):
    self.registers[arg3] = self.registers[arg1] + self.registers[arg2];
    
  def SUB(self, arg1, arg2, arg3):
    self.registers[arg3] = self.registers[arg1] - self.registers[arg2];
    
  def POW(self, arg1, arg2, arg3):
    self.registers[arg3] = self.registers[arg1] ** self.registers[arg2];
    
  def FUNC_CALL(self, arg1, arg2, arg3):
    cur = self.stackcur
    
    sym_funcs[arg1].sm_eval(self)
    if (cur-self.stackcur != sym_funcs[arg1].totarg):
      print(":", cur-self.stackcur, sym_funcs[arg1].totarg)
      raise RuntimeError("Stack corruption while execution function callback " + arg1)
      
    #print("==", cur-self.stackcur, sym_funcs[arg1].totarg)
  
  def PUSH_GLOBAL(self, arg1, arg2, arg3):
    loc = arg1 | arg2 << 16;
    self.stack[self.stackcur] = self.stack[loc]
    self.stackcur += 1
    
  def LOAD_GLOBAL(self, arg1, arg2, arg3):
    loc = arg1 | arg2 << 16;
    self.stack[self.stackcur-1] = self.stack[loc]
    
  def pop(self):
    ret = self.stack[self.stackcur-1]
    self.stackcur -= 1
    return ret 
  
  def push(self, val):
    self.stack[self.stackcur] = val
    self.stackcur += 1
  
  def load(self, val):
    self.stack[self.stackcur-1] = val
    
def func_dv(name, args, s):
  return sym_funcs[name].get_dv(args, s)
  
class sym (list):
  def __init__(self, name_or_val=None):
    list.__init__(self)
    
    self.parent = None
    self.vars = None
    
    if (isinstance(name_or_val, sym)):
      s = name_or_val
      
      self.name = s.name
      self.value = s.value
      self.op = s.op
      self.use_parens = s.use_parens
      self.funcname = s.funcname
      if s.vars != None:
        self.vars = [[n[0].copy(), n[1].copy()] for n in s.vars]
        
      for c in s:
        self.append(c.copy())
      return
      
    name = None
    value = None
    
    if isinstance(name_or_val, str):
      name = name_or_val
    elif type(name_or_val) in [int, float]:
      value = name_or_val
    elif name_or_val != None:
      print(":", name_or_val, type(name_or_val))
      raise RuntimeError()
      
    self.name = name
    self.value = value
    self.funcname = None
    
    self.op = None
    self.use_parens = True
  
  def replace(self, a, b):
    self[self.index(a)] = b
    b.parent = self
    
  def copy(self):
    ret = sym()
    
    ret.name = self.name
    ret.value = self.value
    ret.op = self.op
    ret.use_parens = self.use_parens
    ret.funcname = self.funcname
    if self.vars != None:
      ret.vars = [[n[0].copy(), n[1].copy()] for n in self.vars]
    
    for c in self:
      ret.append(c.copy())
    return ret
  
  def has(self, name):
    if type(name) == sym:
      name = name.name
    
    def rec(n):
      if n.name == name:
        return True
      ret = False
      
      for c in n:
        ret = ret or rec(c)
      
      return ret
    
    return rec(self)
    
  def run_opcodes(self, globals, global_values):
    opcodes, constmap = self.gen_opcodes(globals)
    
    global_values = list(global_values)
    
    sm = StackMachine(global_values, constmap)
    return sm.run(opcodes)
  
  def factor(self):
    if have_sympy:
      expr = symbol_factor.optimize(self)
    else:
      expr = self
    
    return symbol_optimize.optimize(expr)
    
  def gen_opcodes(self, globals, consts=None, constmap=None, constcur=None):
    buf = []
    
    if constcur == None:
      constcur = [0]
      consts = []
      constmap = {}
        
    def out(s):
      s = s.split(" ")
      
      for i in range(1, len(s)):
        try:
          s[i] = int(s[i])
        except:
          pass
          
      while len(s) < 4:
        s.append(0)
      
      #print(str(s[0]) + " " + str(s[1]) + " " + str(s[2]) + " " + str(s[3]))
      buf.append(s)
    
    if self.vars != None:
      for v in self.vars:
        buf += v[1].gen_opcodes(globals, consts, constmap, constcur)[0]
        globals.append(v[0].name)
        out("PUSH 7")
    
    def get_const(n):
      if n not in constmap:
        constmap[n] = constcur[0]
        consts.append(n)
        constcur[0] += 1
        
      return constmap[n]
    
    def rec(n):
      if n.op != None: # and n.value == None and n.name == None:
        rec(n[0])
        
        out("PUSH 7")
        rec(n[1])
        
        out("STK_TO_REG 0 0 1");
        out("STK_TO_REG 1 0 0")
        out("POP 7")
        
        out(opmap[n.op] + " 0 1 2 0")
        out("REG_TO_STK 0 0 2 0")
        
      elif n.funcname != None:
        args1 = list(n)
        args1.reverse()
        
        if n.funcname not in sym_funcs:
          raise RuntimeError("Unknown function " + n.funcname)
          
        sf = sym_funcs[n.funcname]
        
        if sf.totarg != len(args1):
          raise RuntimeError("Invalid numuber of arguments to " + n.funcname + "; expected " + str(sf.totarg) + " but got " + str(len(args1)))
          
        for c in args1:
          out("PUSH 7")
          rec(c)
          
        out("FUNC_CALL " + n.funcname)
        
      elif n.name != None:
        if n.name not in globals:
          raise RuntimeError("Unknown global " + n.name)
        
        loc = globals.index(n.name);
        ia = loc & 65535
        ib = (loc>>16) & 65535
        
        #print("IA, IB:", ia, ib)
        
        out("LOAD_GLOBAL " + str(ia) + " " +str(ib) + " 0 0")
        #out("ABS_STK_TO_REG " + str(ia) + " " + str(ib) + " " + "4");
        #out("REG_TO_STK 0 0 4");
      elif n.value != None:
        out("CONST_TO_STK 0 0 " + str(get_const(n.value)))
      else:
        raise Error("EEK!")
        for c in n:
          rec(c)
    
    rec(self)
    
    return buf, consts
    
  def binop(self, b, op):
    if b == None: raise RuntimeError()
    if not isinstance(b, sym):
      b = sym(b)
    else:
      b = b.copy()
    
    ret = sym()
    
    if op == "*" and (self.value == 0.0 or b.value == 0.0):
      ret.value = 0.0
      return ret
      
    if self.value != None and b.value != None:
      ret.value = eval(str(self.value) + " " + op + " " + str(b.value))
      if ret.value == None:
        raise RuntimeError()
      
      return ret
    else:
      ret.append(self.copy())
      ret.append(b)
    
    #if self.op != None and b.op != None and self.op in prec and b.op in prec:
    #  p1 = prec.index(self.op)
    #  p2 = prec.index(self.op)
    #  self.use_parens = p1 > p2
    
    if b.op != None: # and self.op in prec:
      p1 = prec[b.op]
      p2 = prec[op]
      ret[1].use_parens = 1 #p1 >= p2 # if (op != b.op or op not in ["+", "*"]) else p1 > p2
      
    ret.op = op
    
    return ret
  
  def df(self, s):
    if type(s) != str:
      raise TypeError("s must be a variable name")
      
    if self.value != None:
      return sym(0)
    elif self.name != None:
      return sym(1) if self.name == s else sym(0)
    elif self.op != None:
      if self.op == "*":
        return self[0]*self[1].df(s) + self[1]*self[0].df(s)
      elif self.op == "/":
       ret = self[0].df(s)*self[1] + self[1].df(s)*self[0]
       ret = ret / (self[1]*self[1])
       return ret
      elif self.op == "+":
        return self[0].df(s) + self[1].df(s)
      elif self.op == "-":
        return self[0].df(s) - self[1].df(s)
      elif self.op == "**":
        ret = self[0]**self[1]
        logx = sym.func("log", self[0])
        
        ret *= self[0].df(s)*self[1] + self[1].df(s)*self[0]*logx
        ret /= self[0]
        return ret
      else:
        return 0
    elif self.funcname != None:
      args = self[:]
      
      return func_dv(self.funcname, args, s)
    
  def __repr__(self):
    return str(self)
  
  def __str__(self):
    ret = ""
    if self.use_parens and self.name == None and self.value == None:
      ret += "("
      
    if self.name != None:
      ret += self.name
    elif self.value != None:
      a = str(self.value)
      b = "%.8f" % self.value
      
      ret += a if len(a) < len(b) else b
      if not "." in ret:
        ret += ".0"
        
    elif self.funcname:
      ret += str(self.funcname) + "("
      for i, c in enumerate(self):
        if i > 0:
          ret += ", "
        ret += str(c)
      ret += ")"
    elif len(self) == 2:
      ret += self[0].__str__() + " " + self.op + " " + self[1].__str__()
    else:
      ret += "?" + str(dir(self))
      
    if self.use_parens and self.name == None and self.value == None:
      ret += ")"
    
    return ret
  
  def append(self, item):
    item.parent = self
    list.append(self, item)
    
  @staticmethod
  def func(func, args):
    if type(args) not in [list, tuple]:
      args = [args]
      
    ret = sym()
    ret.funcname = func
      
    for i in range(len(args)):
      args[i] = sym(args[i])
      ret.append(args[i])
    
    return ret
  
  def __neg__(self):
    return self.binop(-1, "*")
  def __add__(self, b):
    return self.binop(b, "+")
  def __radd__(self, b):
    return sym(b).binop(self, "+")
  def __sub__(self, b):
    return self.binop(b, "-")
  def __rsub__(self, b):
    return sym(b).binop(self, "-")
  def __mul__(self, b):
    return self.binop(b, "*")
  def __rmul__(self, b):
    return sym(b).binop(self, "*")
  def __truediv__(self, b):
    return self.binop(b, "/")
  def __rtruediv__(self, b):
    return sym(b).binop(self, "/")
  def __pow__(self, b):
    return self.binop(b, "**")
  def __rpow__(self, b):
    return sym(b).binop(self, "**")

from . import symbol_funcs
import imp

imp.reload(symbol_funcs)

from .symbol_funcs import sym_funcs
from . import symbol_optimize

imp.reload(symbol_optimize)

if have_sympy:
  from . import symbol_factor
  imp.reload(symbol_factor)

def test():  
  x = sym('x');

  print(1+(x/2*3*3+2))
  print(x*x*x)
  f = x*x*x*sym.func("sin", x)
  print(f.df("x"))

