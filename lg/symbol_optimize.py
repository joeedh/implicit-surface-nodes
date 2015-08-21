from .symbol_funcs import sym_funcs
from .symbol import sym

from .utils import CmpList

def optimize(expr):
  return expr
  orig_expr = expr.copy()
  
  subst = {}
  subst2 = {}
  
  expr = expr.copy()
  
  def rec(n):
    if n.value != None or n.name != None: return
    
    s = str(n)
    if s not in subst:
      subst[s] = []
      subst2[s] = n.copy()
      
    subst[s].append(n)
    
    for c in n:
      c.parent = n
      rec(c)
  
  rec(expr)
  
  def sort(a, b):
    return len(a) - len(b)
    
  keys = CmpList(subst.keys())
  keys.sort(sort)
  #keys.reverse()
  
  #keys = keys[:len(keys)//2]
  subs = []
  
  def rec2(n, subn, var):
    s1 = str(n)
    s2 = n2.str
    
    if (s1 == s2) and n.parent != None:
      var = var.copy()
      var.parent = n.parent
      n.parent.replace(n, var)
      
      return + 1
    
    ret = 0
    for c in n:
      c.parent = n
      ret = ret + rec2(c, subn, var)
      
    return ret
    
  for i, k in enumerate(keys):
    if len(k) < 20:
      continue
      
    si = sym("_SUB"+str(i+1))
    n2 = subst2[k]
    n2.str = str(n2)
    e2 = expr.copy()
    ret = rec2(e2, n2, si)
    
    if ret < 2: continue

    print("FOUND!")
    
    expr = e2
    subs.append([si, subst2[k]])
  
  gs = ["_px", "_py", "_pz"]
  
  codes = []
  for s in subs:
    codes += s[1].gen_opcodes(gs)[0]
    gs.append(s[0].name)
    print(str(s[0]) + " = " + str(s[1]))
  
  codes += expr.gen_opcodes(gs)[0]
  print(expr)
  print(gs)
  print(len(codes), len(orig_expr.gen_opcodes(["_px", "_py", "_pz"])[0]))
  
  expr.vars = subs
  print(len(expr.gen_opcodes(gs)[0]))
  
  return expr