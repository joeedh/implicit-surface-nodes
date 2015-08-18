from math import *
from .symbol import sym

def fract(n):
  return n - floor(n)

class SymFunc:
  name = "(unnamed)"
  totarg = None
  
  @staticmethod
  def sm_eval(sm):
    pass
  
  @staticmethod
  def get_dv(args, s):
    pass
    
def perlin(x, y, z):
  return 0.5

class PerlinDv (SymFunc):
  name = "perlin_dv"
  totarg = 6
  
  @staticmethod
  def sm_eval(sm):
    x = sm.pop();
    y = sm.pop();
    z = sm.pop();
    dx = sm.pop();
    dy = sm.pop();
    dz = sm.pop();
    sm.load(0.0)
  
  @staticmethod
  def get_dv(args, s):
    def find(n, s):
      if n.has(s):
        return n.df(s)
      else:
        return sym(0)
    
    dx = args[0].df(s)
    dy = args[1].df(s)
    dz = args[2].df(s)
    
    return sym.func("perlin_dv2", [args[0], args[1], args[2], dx, dy, dz]);
    
class Perlin (SymFunc):
  name = "perlin"
  totarg = 3
  
  @staticmethod
  def sm_eval(sm):  
    sm.load(perlin(sm.pop(), sm.pop(), sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    def find(n, s):
      if n.has(s):
        return n.df(s)
      else:
        return sym(0)
    
    dx = args[0].df(s)
    dy = args[1].df(s)
    dz = args[2].df(s)
    
    return sym.func("perlin_dv", [args[0], args[1], args[2], dx, dy, dz]);
    
class Sin (SymFunc):
  name = "sin"
  totarg = 1
  
  @staticmethod
  def sm_eval(sm):
    sm.load(sin(sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    return sym.func("cos", args[0]) * args[0].df(s)

class Cos (SymFunc):
  name = "cos"
  totarg = 1
  
  @staticmethod
  def sm_eval(sm):
    sm.load(sin(sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    return sym.func("sin", args[0]) * args[0].df(s) *  -1

def safe_sqrt(f):
  return sqrt(abs(f)) if f != 0.0 else 0.0
  
class Sqrt(SymFunc):
  name = "sqrt"
  totarg = 1
  
  @staticmethod
  def sm_eval(sm):
    sm.load(safe_sqrt(sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    return args[0].df(s) / (sym.func("sqrt", args[0])*2)


class Sign (SymFunc):
  name = "sign"
  totarg = 1
  
  @staticmethod
  def sm_eval(sm):
    return sm.load(-1 if sm.pop() < 0 else 1)
  
  @staticmethod
  def get_dv(args, s):
    return sym(0)
    
class Sign (SymFunc):
  name = "sign"
  totarg = 1
  
  @staticmethod
  def sm_eval(sm):
    return sm.load(-1 if sm.pop() < 0 else 1)
  
  @staticmethod
  def get_dv(args, s):
    return sym(0)
  
class Min (SymFunc):
  name = "min"
  totarg = 2
  
  @staticmethod
  def sm_eval(sm):
    sm.load(min(sm.pop(), sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    delt = args[1] - args[0]
    mix = sym.func("sign", delt)*0.5 + 0.5;
    
    return args[0].df(s)*(1.0 - mix) + args[1].df(s)*mix

class Fract (SymFunc):
  name = "fract"
  totarg = 1

  @staticmethod
  def sm_eval(sm):
    sm.load(fract(sm.pop()))

  @staticmethod
  def get_dv(args, s):
    return args[0].df(s)

class Max (SymFunc):
  name = "max"
  totarg = 2
  
  @staticmethod
  def sm_eval(sm):
    sm.load(max(sm.pop(), sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    delt = args[0] - args[1]
    mix = sym.func("sign", delt)*0.5 + 0.5;
    
    return args[0].df(s)*(1.0 - mix) + args[1].df(s)*mix

class Abs(SymFunc):
  name = "abs"
  totarg = 1
  
  @staticmethod
  def sm_eval(sm):
    sm.load(abs(sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    return (sym.func("abs", args[0]) * args[0].df(s)) / args[0];

class Pow(SymFunc):
  name = "pow"
  totarg = 2

  @staticmethod
  def sm_eval(sm):
    sm.load(pow(sm.pop(), sm.pop()))

  @staticmethod
  def get_dv(args, s):
    #(pow({a}, {b})*({dva}*{b} + {dvb}*log({a})*{a}))/{a}
    return (sym.func("pow", [args[0], args[1]])*(args[0].df(s)*args[1] + args[1].df(s)*sym.func("log", args[0])*args[0])) / args[0]

class Log(SymFunc):
  name = "log"
  totarg = 1
  
  @staticmethod
  def sm_eval(sm):
    sm.load(log(sm.pop()))
  
  @staticmethod
  def get_dv(args, s):
    return args[0].df(s) / args[0];

sym_funcs = {}

gs = dict(globals())
v = None

sym_funcs = {}

gs = dict(globals())
v = None

for k in gs:
  v = gs[k]
  print("K", k)
  
  if type(v) == type and issubclass(v, SymFunc) and v is not SymFunc:
    sym_funcs[v.name] = v

