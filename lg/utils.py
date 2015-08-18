import bpy, traceback

class Register:
  def register(self):
    pass
  def unregister(self):
    pass
    
class CustomRegister (Register):
  def __init__(self, reg, unreg):
    self.reg = reg
    self.unreg = unreg
    self.is_reg = False
    
  def register(self):
    if self.is_reg: return
    self.reg()
    self.is_reg = True
  
  def unregister(self):
    if not self.is_reg: return 
    self.unreg()
    self.is_reg = False
    
class Registrar (Register):
  def __init__(self, classes):
    self.classes = classes
    self.is_reg = False
  
  def register(self):
    if self.is_reg: return
    
    for cls in self.classes:
      if isinstance(cls, Register):
        cls.register()
      else:
        bpy.utils.register_class(cls)
    
    self.is_reg = True
  
  def unregister(self):
    if not self.is_reg: return
    
    for cls in self.classes:
      if isinstance(cls, Register):
        cls.unregister()
      else:
        bpy.utils.unregister_class(cls)
    
    self.is_reg = False
  
  @classmethod
  def custom(self, register, unregister):
    return CustomRegister(register, unregister)

def nodeid(nodelist, n):
  for i, n2 in enumerate(nodelist):
    if n == n2: return i
  return -1
  
def sockid(nodelist, sock):
  idx = list(sock.node.inputs).index(sock) if not sock.is_output else list(sock.node.outputs).index(sock)
  type = "i" if not sock.is_output else "o"
  
  return str(nodeid(nodelist, sock.node)) + ":" + type + ":" + str(idx)

def get_node_from_path(node_path):
  print(node_path)
  return eval(node_path)
  
def gen_node_path(node, ntree):
  s = "bpy.data.node_groups['" + ntree.name + "'].nodes['" + node.name + "']"
  return s
  