import bpy
from bpy.types import NodeTree, Node, NodeSocket, NodeCustomGroup, NodeGroup, NodeGroupInput
from . import codegen
from . import utils

# Implementation of custom nodes from Python

# Derived from the NodeTree base type, similar to Menu, Operator, Panel, etc.
class ImplicitTree(NodeTree):
    # Description string
    '''Implicit Surface Editor'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitTreeType'
    # Label for nice name display
    bl_label = 'Implicit Surface'
    
    # Icon identifier
    bl_icon = 'NODETREE'
  
    @classmethod
    def poll(cls, ntree):
      return True

    
class FieldCustomGroup (NodeCustomGroup):
  bl_idname = "ImplicitNodeGroup"
  bl_label = "Group"
  bl_icon = 'SOUND'
  bl_width_min = 250
  
  def init(self, context):
    pass
    
  def copy(self, b):
    pass
    
  def poll_instance(self, tree):
    return ntree.bl_idname == 'ImplicitTreeType'
    
  @classmethod
  def poll(cls, ntree):
    return ntree.bl_idname == 'ImplicitTreeType'

  def gen_code(self, cg, ins):
    cgen = codegen.CodeGen(self.node_tree)
    cgen.owner_node = self
    cgen.sort()
    ret = cgen.gen(instance_mode=True, instance_inputs=ins)
    
    return ret
    
  # Additional buttons displayed on the node.
  def draw_buttons(self, context, layout):
    if self.node_tree == None:
      return
      
    layout.label(self.node_tree.name)
    
    layout.prop(self.node_tree, "name")
    
    prop = layout.operator("node.implicit_edit_group", text="Edit Group")
    print("PATH", context.space_data.path[-1].node_tree)
    node_tree = context.space_data.path[-1].node_tree
    prop["node_path"] = utils.gen_node_path(self, node_tree) #context.space_data.path[-1]) #node_tree)
    
  def draw_buttons_ext(self, context, layout):
    pass
      
# Custom socket type
class FieldVectorSocket(NodeSocket):
    # Description string
    '''Vector Socket'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitVectorSocket'
    # Label for nice name display
    bl_label = 'Vector'

    value = bpy.props.FloatVectorProperty(default=[0.0, 0.0, 0.0], size=3)
    
    # Optional function for drawing the socket input vector
    def draw(self, context, layout, node, text):
        if self.is_output or self.is_linked:
            layout.label(text)
        else:
            layout.prop(self, "value", text=self.name)

    # Socket color
    def draw_color(self, context, node):
        return (0.4, 0.8, 1.0, 1.0)



# Custom socket type
class FieldSocket(NodeSocket):
    # Description string
    '''Implicit Field'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitFieldSocket'
    # Label for nice name display
    bl_label = 'Field'

    value      = bpy.props.FloatProperty(default=0.0)
    
    # Optional function for drawing the socket input value
    def draw(self, context, layout, node, text):
        if self.is_output or self.is_linked:
            layout.label(text)
        else:
            layout.prop(self, "value", text=self.name)

    # Socket color
    def draw_color(self, context, node):
        return (1.0, 0.4, 0.216, 1.0)


  
from . import symbol
sym = symbol.sym

#stype is either 'vec' (vector) or 'field' (scalar)
def coerce(a, stype):
  if type(a) in [list, tuple] and stype != "vec":
    a = sym.func("sqrt", a[0]*a[0] + a[1]*a[1] + a[2]*a[2])
  elif type(a) not in [list, tuple] and stype != "field":
    a = [a, a, a]
  return a
  
# Mix-in class for all custom nodes in this tree type.
# Defines a poll function to enable instantiation.
class ImplicitTreeNode:
    tag = bpy.props.IntProperty(default=0) #used during sorting
    
    @classmethod
    def poll(cls, ntree):
        return ntree.bl_idname == 'ImplicitTreeType'
    
    def gen_code(self, codegen, ins):
      pass

class math_func_impl:
  def SIN(self, a, b, dva, dvb):
    return sym.func("sin", a)
  def COS(self, a, b, dva, dvb):
    return sym.func("cos", a)
  def TAN(self, a, b, dva, dvb):
    return sym.func("tan", a)#, "{dva}*(tan({a})*tan({a}) + 1.0)"
  def ASIN(self, a, b, dva, dvb):
    return sym.func("asin", a)#, "(-sqrt(-{a}*{a} + 1.0)*{dva})/({a}*{a} - 1.0)"
  def ACOS(self, a, b, dva, dvb):
    return sym.func("acos", a)#, "(sqrt(-{a}*{a} + 1.0)*{dva})/({a}*{a} - 1.0)"
  def POW(self, a, b, dva, dvb):
    return sym.func("pow", [a, b])#, "(pow({a}, {b})*({dva}*{b} + {dvb}*log({a})*{a}))/{a}"
  def ABS(self, a, b, dva, dvb):
    return sym.func("abs", a)
  def FLOOR(self, a, b, dva, dvb):
    return sym.func("floor", a)
  def CEIL(self, a, b, dva, dvb):
    return sym.func("ceil", a)
  def FRACT(self, a, b, dva, dvb):
    return sym.func("fract", a)
  def TRUNC(self, a, b, dva, dvb):
    return sym.func("trunc", a)
  def ATAN(self, a, b, dva, dvb):
    return sym.func("atan", a)#, "atan({a})", "({dva}/({a}*{a}+1.0)"
  def TENT(self, a, b, dva, dvb):
    return sym(1.0) - sym.func("abs", [sym.func("fract", a) - 0.5])*2.0
  def ATAN2(self, a, b, dva, dvb):
    return sym.func("atan2", a)#, "atan2({b}, {a})", "(atan2({a}+0.001) - atan2({a}-0.001)) / 500.0"
  def MUL(self, a, b, dva, dvb):
    return sym(a) * sym(b)
  def SUB(self, a, b, dva, dvb):
    return sym(a) - sym(b)
  def ADD(self, a, b, dva, dvb):
    return sym(a) + sym(b)
  def DIV(self, a, b, dva, dvb):
    return sym(a) / sym(b)
  def MIN(self, a, b, dva, dvb):
    return sym.func("min", [a, b])
  def MAX(self, a, b, dva, dvb):
    return sym.func("max", [a, b])#, "max({a}, {b})", "{a} > {b} ? {dva} : {dvb}"
    
  def CROSS(self, a, b, dva, dvb):
    return [
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0]
    ]
    
  #vector functions
  def DOT(self, a, b, dva, dvb):
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
  def LEN(self, a, b, dva, dvb):
    return sym.func('sqrt', [a[0]*a[0] + a[1]*a[1] + a[2]*a[2]])
      #"""({dva}[0]*{a}[0] + {dva}[1]*{a}[1] + {dva}[2]*{a}[2]) / 
      #     sqrt({a}[0]*{a}[0] + {a}[1]*{a}[1] + {a}[2]*{a}[2]")"""
  
# Derived from the Node base type.
class MathNode(Node, ImplicitTree):
    # === Basics ===
    # Description string
    '''A custom node'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitMathNode'
    # Label for nice name display
    bl_label = 'Math Node'
    # Icon identifier
    bl_icon = 'SOUND'
    bl_width_min = 200
  
    # === Custom Properties ===
    # These work just like custom properties in ID data blocks
    # Extensive information can be found under
    # http://wiki.blender.org/index.php/Doc:2.6/Manual/Extensions/Python/Properties
    #myStringProperty = bpy.props.StringProperty()
    #myFloatProperty = bpy.props.FloatProperty(default=3.1415926)

    # Enum items list
    math_funcs = [
        ("SIN", "Sin", "Sine"),
        ("COS", "Cos", "Cosine"),
        ("TENT", "Tent", "Tent"),
        ("TAN", "Tan", "Tangent"),
        ("ASIN", "Asin", "Sine"),
        ("ACOS", "Acos", "Sine"),
        ("POW", "Pow", "Sine"),
        ("ABS", "Abs", "Sine"),
        ("FLOOR", "Floor", "Sine"),
        ("CEIL", "Floor", "Sine"),
        ("FRACT", "Fract", "Sine"),
        ("TRUNC", "Truncate", "Sine"),
        ("ATAN", "Atan", "Sine"),
        ("ATAN2", "Atan2 (xy to polar)", "Sine"),
        ("MUL", "Multiply", "Sine"),
        ("SUB", "Subtract", "Sine"),
        ("ADD", "Add", "Sine"),
        ("DIV", "Divide", "Sine"),
        ("MIN", "Min", "Min"),
        ("MAX", "Max", "Max"),
    ]
    
    mathFunc = bpy.props.EnumProperty(name="Function", description="Math Functions", items=math_funcs, default='ADD')

    def init(self, context):
        self.inputs.new('ImplicitFieldSocket', "a")
        self.inputs.new('ImplicitFieldSocket', "b")

        self.outputs.new('ImplicitFieldSocket', "field")
    
    def gen_code(self, codegen, ins):
      code = getattr(math_func_impl, self.mathFunc)
      
      def isvec(v):
        return type(v) in [list, tuple]
      
      a = coerce(ins["a"], "field")
      b = coerce(ins["b"], "field")

      f = code(None, a, b, 0, 0);
      
      return {
        "field" : f
      }
      
    # Copy function to initialize a copied node from an existing one.
    def copy(self, node):
        self.mathFunc = node.mathFunc
        print("Copying from node ", node)

    # Free function to clean up on removal.
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
        layout.prop(self, "mathFunc")
        #layout.prop(self, "myFloatProperty")

    # Detail buttons in the sidebar.
    # If this function is not defined, the draw_buttons function is used instead
    def draw_buttons_ext(self, context, layout):
        pass
        #layout.prop(self, "myFloatProperty")
        # myStringProperty button will only be visible in the sidebar
        #layout.prop(self, "myStringProperty")

    # Optional: custom label
    # Explicit user label overrides this, but here we can define a label dynamically
    def draw_label(self):
        return "Math Node"

class VectorMathNode(Node, ImplicitTree):
    '''A custom node'''
    bl_idname = 'ImplicitVectorMathNode'
    bl_label = 'Vector Math Node'
    bl_icon = 'SOUND'
    bl_width_min = 300
    
    tag = bpy.props.IntProperty(default=0) #used during sorting
  
    # Enum items list
    math_funcs = [
        ("POW", "Pow", "Sine"),
        ("ABS", "Abs", "Sine"),
        ("FLOOR", "Floor", "Sine"),
        ("CEIL", "Floor", "Sine"),
        ("FRACT", "Fract", "Sine"),
        ("TRUNC", "Truncate", "Sine"),
        ("MUL", "Multiply", "Sine"),
        ("SUB", "Subtract", "Sine"),
        ("ADD", "Add", "Sine"),
        ("DIV", "Divide", "Sine"),
        ("MIN", "Min", "Min"),
        ("MAX", "Max", "Max"),
        ("CROSS", "Cross", "Cross"),
        ("DOT", "Dot", "Dot")
    ]
    
    mathFunc = bpy.props.EnumProperty(name="Function", description="Math Functions", items=math_funcs, default='ADD')

    def init(self, context):
        self.inputs.new('ImplicitVectorSocket', "a")
        self.inputs.new('ImplicitVectorSocket', "b")

        self.outputs.new('ImplicitVectorSocket', "vector")
    
    def gen_code(self, codegen, ins):
      code = getattr(math_func_impl, self.mathFunc)
      
      def isvec(v):
        return type(v) in [list, tuple]
      
      a = coerce(ins["a"], "vec")
      b = coerce(ins["b"], "vec")
      
      f = []
      if self.mathFunc != "CROSS":
        for i in range(3):
          f.append(code(None, a[i], b[i], 0, 0));
      else:
        f = code(None, a, b, 0, 0)
      
      return {
        "vector" : f
      }
      
    # Copy function to initialize a copied node from an existing one.
    def copy(self, node):
        self.mathFunc = node.mathFunc
        print("Copying from node ", node)

    # Free function to clean up on removal.
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
        layout.prop(self, "mathFunc")

    def draw_buttons_ext(self, context, layout):
        pass

    def draw_label(self):
        return "Vector Math Node"

class VectorReducNode(Node, ImplicitTree):
    '''A custom node'''
    bl_idname = 'ImplicitVectorReduceNode'
    bl_label = 'Vector Reduce Node'
    bl_icon = 'SOUND'
    
    tag = bpy.props.IntProperty(default=0) #used during sorting
  
    # Enum items list
    math_funcs = [
        ("POW", "Pow", "Sine"),
        ("ABS", "Abs", "Sine"),
        ("FLOOR", "Floor", "Sine"),
        ("CEIL", "Ceil", "Sine"),
        ("FRACT", "Fract", "Sine"),
        ("TRUNC", "Truncate", "Sine"),
        ("MUL", "Multiply", "Sine"),
        ("SUB", "Subtract", "Sine"),
        ("ADD", "Add", "Sine"),
        ("DIV", "Divide", "Sine"),
        ("MIN", "Min", "Min"),
        ("MAX", "Max", "Max"),
        ("TENT", "Tent", "Tent"),
        ("SIN", "Sin", "Sin"),
        ("COS", "Cos", "Cos"),
    ]
    
    mathFunc = bpy.props.EnumProperty(name="Function", description="Math Functions", items=math_funcs, default='ADD')

    def init(self, context):
        self.inputs.new('ImplicitVectorSocket', "vector")
        self.outputs.new('ImplicitFieldSocket', "value")
    
    def gen_code(self, codegen, ins):
      code = getattr(math_func_impl, self.mathFunc)
      
      def isvec(v):
        return type(v) in [list, tuple]
      
      vec = coerce(ins["vector"], "vec")
      val = vec[0]
      
      for i in range(1, len(vec)):
        val = code(None, val, vec[i], 0, 0)
      
      return {
        "value" : val
      }
      
    # Copy function to initialize a copied node from an existing one.
    def copy(self, node):
        self.mathFunc = node.mathFunc
        print("Copying from node ", node)

    # Free function to clean up on removal.
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
        layout.prop(self, "mathFunc")

    def draw_buttons_ext(self, context, layout):
        pass

    def draw_label(self):
        return "Vector Reduce Node"

class SeperateVecNode(Node, ImplicitTree):
    '''A custom node'''
    bl_idname = 'ImplicitSeperateVecNode'
    bl_label = 'Seperate Vector'
    bl_icon = 'SOUND'

    def init(self, context):
        self.inputs.new('ImplicitVectorSocket', "vector")

        self.outputs.new('ImplicitFieldSocket', "x")
        self.outputs.new('ImplicitFieldSocket', "y")
        self.outputs.new('ImplicitFieldSocket', "z")
    
    def gen_code(self, codegen, ins):
      f = coerce(ins["vector"], "vec")

      return {
        "x" : f[0],
        "y" : f[1],
        "z" : f[2]
      }
      
    # Copy function to initialize a copied node from an existing one.
    def copy(self, node):
        print("Copying from node ", node)
    # Free function to clean up on removal.
    def free(self):
        print("Removing node ", self, ", Goodbye!")
    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
      pass

    def draw_buttons_ext(self, context, layout):
        pass

    def draw_label(self):
        return "Vector Math Node"
        
class CombineVecNode(Node, ImplicitTree):
    '''A custom node'''
    bl_idname = 'ImplicitCombineVecNode'
    bl_label = 'Combine Vector'
    bl_icon = 'SOUND'

    def init(self, context):
        self.outputs.new('ImplicitVectorSocket', "vector")

        self.inputs.new('ImplicitFieldSocket', "x")
        self.inputs.new('ImplicitFieldSocket', "y")
        self.inputs.new('ImplicitFieldSocket', "z")
    
    def gen_code(self, codegen, ins):
      x = coerce(ins["x"], "field")
      y = coerce(ins["y"], "field")
      z = coerce(ins["z"], "field")
      
      return {
        "vector" : [x, y, z]
      }
      
    # Copy function to initialize a copied node from an existing one.
    def copy(self, node):
        print("Copying from node ", node)
    # Free function to clean up on removal.
    def free(self):
        print("Removing node ", self, ", Goodbye!")
    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
      pass

    def draw_buttons_ext(self, context, layout):
        pass

    def draw_label(self):
        return "Vector Math Node"

class FieldDiffNode(Node, ImplicitTree):
    '''A custom node'''
    bl_idname = 'ImplicitFieldDiffNode'
    bl_label = 'Scalar Derivative'
    bl_icon = 'SOUND'

    def init(self, context):
        self.inputs.new('ImplicitFieldSocket', "value")
        self.outputs.new('ImplicitVectorSocket', "derivative")
    
    def gen_code(self, codegen, ins):
      a = coerce(ins["value"], "field")
      
      return {
        "derivative" : [a.df("_px"), a.df("_py"), a.df("_pz")]
      }
      
    # Copy function to initialize a copied node from an existing one.
    def copy(self, node):
        print("Copying from node ", node)
    # Free function to clean up on removal.
    def free(self):
        print("Removing node ", self, ", Goodbye!")
    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
      pass

    def draw_buttons_ext(self, context, layout):
        pass

    def draw_label(self):
        return "Scalar Derivative"

# Derived from the Node base type.
class InputNode(Node, ImplicitTree):
    # === Basics ===
    # Description string
    '''Input Node'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitInputNode'
    
    # Label for nice name display
    bl_label = 'Input Position'
    
    # Icon identifier
    bl_icon = 'SOUND'

    tag = bpy.props.IntProperty(default=0) #used during sorting

    def init(self, context):
        self.outputs.new('ImplicitVectorSocket', "position")
    
    def gen_code(self, codegen, ins):
      return {
        "position" : [sym("_px"), sym("_py"), sym("_pz")]
      }
      
    def copy(self, node):
        print("Copying from node ", node)
        
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
    def draw_buttons_ext(self, context, layout):
        pass
    def draw_label(self):
        return "Input Node"

# Derived from the Node base type.
class OutputNode(Node, ImplicitTree):
    # === Basics ===
    # Description string
    '''Output Node'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitOutputNode'
    
    # Label for nice name display
    bl_label = 'Field Output'
    
    # Icon identifier
    bl_icon = 'SOUND'
    bl_width_min = 250

    tag = bpy.props.IntProperty(default=0) #used during sorting

    def init(self, context):
        self.inputs.new('ImplicitFieldSocket', "field")
        self.inputs.new('ImplicitVectorSocket', "color")
        
    def gen_code(self, codegen, ins):
      return {} #do nothing

    def copy(self, node):
        print("Copying from node ", node)
        
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
    def draw_buttons_ext(self, context, layout):
        pass
    def draw_label(self):
        return "Output Node"

# Derived from the Node base type.
class NormalizeNode(Node, ImplicitTree):
    # === Basics ===
    # Description string
    '''Normalize Node'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitNormalizeNode'
    
    # Label for nice name display
    bl_label = 'Normalize'
    
    # Icon identifier
    bl_icon = 'SOUND'
    bl_width_min = 250

    tag = bpy.props.IntProperty(default=0) #used during sorting

    def init(self, context):
        self.inputs.new('ImplicitVectorSocket', "vector")
        self.outputs.new('ImplicitVectorSocket', "vector")
        
    def gen_code(self, codegen, ins):
      v = coerce(ins["vector"], "vec");
      
      #l = sym.func("sqrt", v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
      l = sym.func('length', v)
      
      return {
        "vector" : [v[0] / l, v[1] / l, v[2] / l]
      }

    def copy(self, node):
        print("Copying from node ", node)
        
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
    def draw_buttons_ext(self, context, layout):
        pass
    def draw_label(self):
        return "Normalize"

# Derived from the Node base type.
class PerlinNode(Node, ImplicitTree):
    # === Basics ===
    # Description string
    '''PerlinNode Node'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitPerlinNode'
    
    # Label for nice name display
    bl_label = 'Perlin Noise'
    
    # Icon identifier
    bl_icon = 'SOUND'

    tag = bpy.props.IntProperty(default=0) #used during sorting

    def init(self, context):
        self.inputs.new('ImplicitVectorSocket', "position")
        self.inputs.new('ImplicitFieldSocket',  'size')
        
        self.outputs.new('ImplicitFieldSocket', "value")
        
    def gen_code(self, codegen, ins):
      p = coerce(ins["position"], "vec")
      sz = coerce(ins["size"], "field")
      
      return {
        "value" : sym.func("perlin", [p[0]/sz, p[1]/sz, p[2]/sz])
      } #do nothing

    def copy(self, node):
        print("Copying from node ", node)
        
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
    def draw_buttons_ext(self, context, layout):
        pass
    def draw_label(self):
        return "Perlin Noise"

# Derived from the Node base type.
class Intersect(Node, ImplicitTree):
    # === Basics ===
    # Description string
    '''Intersect Node'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitIntersectNode'
    # Label for nice name display
    bl_label = 'Intersect'
    # Icon identifier
    bl_icon = 'SOUND'
    
    smoothness = bpy.props.FloatProperty(default=0.0)

    def init(self, context):
        self.inputs.new('ImplicitFieldSocket', "a")
        self.inputs.new('ImplicitFieldSocket', "b")
        self.outputs.new('ImplicitFieldSocket', "field")

    def copy(self, node):
        print("Copying from node ", node)
    
    def gen_code(self, codegen, ins):
      def isvec(v):
        return type(v) in [list, tuple]
        
      a = ins["a"]
      b = ins["b"]
      
      if isvec(a):
        a = sym.func("sqrt", a[0]*a[0] + a[1]*a[1] + a[2]*a[2])
      if isvec(b):
        b = sym.func("sqrt", b[0]*b[0] + b[1]*b[1] + b[2]*b[2])
      
      f = sym.func("min", [a, b])
      
      return {
        "field" : f
      }
      
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
        layout.prop(self, "smoothness")
        
    def draw_buttons_ext(self, context, layout):
        pass
    def draw_label(self):
        return "Intersect Node"
        
# Derived from the Node base type.
class Union(Node, ImplicitTree):
    # === Basics ===
    # Description string
    '''Union Node'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'ImplicitUnionNode'
    # Label for nice name display
    bl_label = 'Union'
    # Icon identifier
    bl_icon = 'SOUND'
    
    smoothness = bpy.props.FloatProperty(default=0.0)

    def init(self, context):
        self.inputs.new('ImplicitFieldSocket', "a")
        self.inputs.new('ImplicitFieldSocket', "b")
        self.outputs.new('ImplicitFieldSocket', "field")

    def copy(self, node):
        print("Copying from node ", node)
    
    def gen_code(self, codegen, ins):
      a = coerce(ins["a"], 'field')
      b = coerce(ins["b"], 'field')
      
      f = sym.func("min", [a*-1, b])
      
      return {
        "field" : f
      }
      
    def free(self):
        print("Removing node ", self, ", Goodbye!")

    # Additional buttons displayed on the node.
    def draw_buttons(self, context, layout):
        layout.label("Node settings")
        layout.prop(self, "smoothness")
        
    def draw_buttons_ext(self, context, layout):
        pass
    def draw_label(self):
        return "Union Node"
  
bpy_exports = utils.Registrar([
  ImplicitTree,
  FieldSocket,
  FieldVectorSocket,
  MathNode,
  VectorMathNode,
  OutputNode,
  Intersect,
  Union,
  InputNode,
  CombineVecNode,
  SeperateVecNode,
  VectorReducNode,
  FieldCustomGroup,
  FieldDiffNode,
  NormalizeNode,
  PerlinNode
]);
