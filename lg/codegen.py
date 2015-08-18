from . import symbol
sym = symbol.sym

sinknodes = {"ImplicitOutputNode", "NodeGroupOutput"}

class CodeGen:
  def __init__(self, tree=None):
    self.nodes = []
    
    if tree != None:
      self.load(tree)
    self.owner_node = None #for groups
    
  def add_library_code(self, key, code):
    pass
  
  def load(self, tree):
    for n in tree.nodes:
      self.nodes.append(n)
      
  def sort(self):
    lst = []
    self.sortlist = lst
    tag = set()
    
    def nid(n):
      return self.nodes.index(n)
      
    def rec(n):
      if nid(n) in tag: return
      tag.add(nid(n))
      
      for i in n.inputs:
        for l in i.links:
          src = l.from_node
            
          if nid(src) not in tag:
            rec(src)
      
      lst.append(n)
      
      #for o in n.outputs:
      #  for l in o.links:
      #    rec(l.to_node)
      
    for n in self.nodes:
      if n.bl_idname in sinknodes:
        rec(n)
    
  def gen(self, instance_mode=False, instance_inputs=None):
    rets = [] if not instance_mode else {}
    inmap = {}
    outmap = {}
    
    def nid(n):
      return self.nodes.index(n)
      
    def sid(sock):
      idx = list(sock.node.inputs).index(sock) if not sock.is_output else list(sock.node.outputs).index(sock)
      type = "i" if not sock.is_output else "o"
      
      return str(nid(sock.node)) + ":" + type + ":" + str(idx)
      
    for n in self.sortlist:
      if n.bl_idname == "NodeGroupInput": 
        if instance_mode:
          for i, o in enumerate(n.outputs):
            if o.name == "": continue #happens with phantom group node socket thingies
            id = sid(o)
            print(o, "ID", id, "o.name", o.name, "||")
            outmap[id] = instance_inputs[o.name]
        continue
      elif n.bl_idname == "NodeGroupOutput":
        continue
        
      ins = {}
      outs = {}
      for o in n.outputs:
        outs[o.name] = o
      
      print(outmap)
      for i in n.inputs:
        if i.name == "": #phantom node thingy
          continue
        val = None
        
        if len(i.links) == 0:
          if i.bl_idname.startswith("Implicit"):
            val = i.value
            if type(val) not in [int, float]:
              val = list(val)
        else:
          print(i.name, i.links[0].from_socket.name)
          val = outmap[sid(i.links[0].from_socket)]
          
        if val == None: 
          print(":", len(i.links), "|", i.name)
          raise RuntimeError()
          val = sym("<error!>")
        
        ins[i.name] = val
      
      expr = n.gen_code(self, ins)
      if expr == None:
        print(n, ins)
        raise RuntimeError("Got none from a gen_code() implementation")
      
      for k in outs:
        sock = outs[k]
        id = sid(sock)
        outmap[id] = expr[sock.name]
    
    if not instance_mode:
      for n in self.nodes:
        #field
        if n.bl_idname == "ImplicitOutputNode" and len(n.inputs[0].links) > 0:
          sock = n.inputs[0].links[0].from_socket
          id = sid(sock)
          rets.append(outmap[id]);
          print(outmap[id])
          print("ID", id)
        
        #color
        if n.bl_idname == "ImplicitOutputNode" and len(n.inputs[1].links) > 0:
          sock = n.inputs[1].links[0].from_socket
          id = sid(sock)
          rets.append(outmap[id]);
        elif n.bl_idname == "ImplicitOutputNode":
          rets.append([sym(val) for val in n.inputs[1].value])
    else:
      for n in self.nodes:
        if n.bl_idname == "NodeGroupOutput":
          for i in n.inputs:
            if i.name == "": #phanton node thingy
              continue
              
            if len(i.links) > 0:
              sock = i.links[0].from_socket
              id = sid(sock)
              print(outmap, self.sortlist)
              rets[i.name] = outmap[id];
            else:
              rets[i.name] = i.value if type(i.value) in [int, float] else list(i.value)
              
    #print(outmap)
    return rets

def write_js(expr, path):
  buf = """
  //AUTO-GENERATED FILE! DO NOT EDIT!
  
  var sin=Math.sin, cos=Math.cos, abs=Math.abs, log=Math.log,
  asin=Math.asin, exp=Math.exp, acos=Math.acos, fract=Math.fract,
  sign=Math.sign, tent=Math.tent, atan2=Math.atan2, atan=Math.atan,
  pow=Math.pow, sqrt=Math.sqrt, floor=Math.floor, ceil=Math.ceil,
  min=Math.min, max=Math.max, PI=Math.PI, E=2.718281828459045;

  window._generated_sampler = function(p) {
    var _px = p[0], _py = p[1], _pz = p[2];
    CODE
  }
""".replace("CODE", "return "+str(expr))

  file = open(path, "w")
  file.write(buf)
  file.close()
  
def write_shader(expr, path, inpath=None):
  if inpath == None:
    inpath = path + ".in"
  file = open(inpath, "r")
  buf = file.read()
  file.close()
  
  if type(expr) in [list, tuple]: #vector
    code = "vec3(" + str(expr[0]) + "," + str(expr[1]) + "," + str(expr[2]) + ")"
  else:
    code = "vec3(" + str(expr) + ", " + str(expr) + ", " + str(expr) + ")"
  
  buf = buf.replace("CODE_HERE", code)
  
  file = open(path, "w")
  file.write(buf)
  file.close()
  
  