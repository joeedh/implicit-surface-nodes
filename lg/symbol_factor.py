import symbol

import sympy
from sympy import *

from .symbol_funcs import sym_funcs

opmap = {
  '*' : '__mul__',
  '/' : '__truediv__',
  '+' : '__add__',
  '-' : '__sub__',
  '**' : '__pow__',
  '>' : '__gt__',
  '<' : '__le__',
  '>=' : '__gte__',
  '<=' : '__lte__',
  '==' : '__eq__',
}
  
def optimize(expr):
  sfuncs = {}
  
  def sym_to_sympy(expr):
    if expr.name != None:
      return symbols(expr.name)
    elif expr.value != None:
      return expr.value
    elif expr.funcname != None:
      args = [sym_to_sympy(c) for c in expr]
      return sfuncs[expr.funcname](*args)
    elif expr.op != None:
      op = expr.op
      a = sym_to_sympy(expr[0])
      b = sym_to_sympy(expr[1])
      if op == "*":
        return a * b
      if op == "/":
        return a / b
      if op == "+":
        return a + b
      if op == "-":
        return a - b
      if op == "**":
        return a ** b
      if op == ">":
        return a > b
      if op == "<":
        return a < b
      if op == ">=":
        return a >= b
      if op == "<=":
        return a <= b
      if op == "==":
        return a == b
      else:
        raise RuntimeError("unknown op " + op)
  
      
  for k in sym_funcs:
    f = Function(k)
    sfuncs[k] = f
  
  #print("\n\nEXPR\n", expr, "\n\n")
  
  sexpr = factor(sym_to_sympy(expr))
  
  _px = _sym('_px')
  _py = _sym('_py')
  _pz = _sym('_pz')
  s = str(sexpr)
  
  print("\n\nSEXPR\n", sexpr, "|||\n\n")
  
  print(type(eval(s)), eval(s), "\n\n\n")
  expr = eval(s)
  
  return expr
  
from .symbol import sym as _sym

#helper functions with casting back from sympy to symbol
def gen_func_code():
  code = ""
  for k in sym_funcs:
    f = sym_funcs[k]
    print(f)
    if f == None: continue
    
    s = "def " + k + "("
    argstr = ""
    for i in range(f.totarg):
      if i > 0:
        s += ", "
        argstr += ", "
        
      s += "arg%i" % (i+1)
      argstr += "arg%i" % (i+1)
      
    s += "):\n"
    s += "    "
    s += "return _sym.func('" + k +"', [" + argstr + "])\n"
    s += "\n"
    
    code += s

  return code 
  
exec(gen_func_code())
