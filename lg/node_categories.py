import bpy
from bpy.types import NodeTree, Node, NodeSocket, Operator
from bl_operators.node import NodeAddOperator

### Node Categories ###
# Node categories are a python system for automatically
# extending the Add menu, toolbar panels and search operator.
# For more examples see release/scripts/startup/nodeitems_builtins.py

import nodeitems_utils
from nodeitems_utils import NodeCategory, NodeItem

from . import utils, node
from .node import *


# Add a node and link it to an existing socket
class NODE_OT_add_implicit_group(NodeAddOperator, Operator):
    '''Add a node to the active tree and link to an existing socket'''
    bl_idname = "node.add_implicit_group_node"
    bl_label = "Add and Link Node"
    bl_options = {'REGISTER', 'UNDO'}

    tree_name = bpy.props.StringProperty()
    node_name = bpy.props.StringProperty()
    
    def execute(self, context):
        space = context.space_data
        ntree = space.edit_tree

        print("TREE NAME", self.tree_name)
        node = self.create_node(context)
        
        if self.node_name != None:
          node.name = node.bl_label = self.node_name
          
        if not node or self.tree_name == "":
            return {'CANCELLED'}
        
        if self.tree_name not in bpy.data.node_groups:
          print("ERROR! BAD TREE", self.tree_name)
          return {'CANCELLED'}
        
        
        node.node_tree = bpy.data.node_groups[self.tree_name];
        
        return {'FINISHED'}

class ImplicitNodeItem (NodeItem):
    def __init__(self, nodetype, label=None, settings=None, poll=None, tree_name=""):

        if settings is None:
            settings = {}

        self.tree_name = tree_name
        self.nodetype = nodetype
        self._label = label
        self.settings = settings
        self.poll = poll

    @property
    def label(self):
        if self._label:
            return self._label
        else:
            # if no custom label is defined, fall back to the node type UI name
            return getattr(bpy.types, self.nodetype).bl_rna.name

    # NB: is a staticmethod because called with an explicit self argument
    # NodeItemCustom sets this as a variable attribute in __init__
    @staticmethod
    def draw(self, layout, context):
        default_context = bpy.app.translations.contexts.default

        props = layout.operator("node.add_implicit_group_node", text=self.label, text_ctxt=default_context)
        props.type = self.nodetype
        props.use_transform = True
        props.tree_name = self.tree_name
        props.node_name = self.tree_name
        
        for setting in self.settings.items():
            ops = props.settings.add()
            ops.name = setting[0]
            ops.value = setting[1]

# our own base class with an appropriate poll function,
# so the categories only show up in our own tree type
class ImplicitNodeCategory(NodeCategory):
    @classmethod
    def poll(cls, context):
        return context.space_data.tree_type == 'ImplicitTreeType'

def get_groups(unused):
  for ntree in bpy.data.node_groups:
    if ntree.bl_idname != "ImplicitTreeType": continue
    
    nitem = ImplicitNodeItem("ImplicitNodeGroup", label=ntree.name, tree_name=ntree.name)
    yield nitem
    
    continue
    for node in ntree.nodes:
      if node.bl_idname == "NodeGroupInput":
        yield ntree
        break
  
# all categories in a list
node_categories = [
    # identifier, label, items list
    ImplicitNodeCategory("OUTPUTS", "Output Nodes", items=[
        NodeItem("ImplicitOutputNode"),
        NodeItem("NodeGroupOutput")
    ]),
    ImplicitNodeCategory("INPUT", "Input Nodes", items=[
        NodeItem("ImplicitInputNode"),
        NodeItem("NodeGroupInput")
    ]),
    ImplicitNodeCategory("PROCEDURAL", "Procedural Nodes", items=[
      NodeItem("ImplicitPerlinNode")
    ]),
    ImplicitNodeCategory("MATH", "Math Nodes", items=[
        NodeItem("ImplicitMathNode"),
        NodeItem("ImplicitVec3LengthNode"),
        NodeItem("ImplicitVectorMathNode"),
        NodeItem("ImplicitSeperateVecNode"),
        NodeItem("ImplicitCombineVecNode"),
        NodeItem("ImplicitVectorReduceNode"),
        NodeItem("ImplicitNodeGroup"),
        NodeItem("ImplicitFieldDiffNode"),
        NodeItem("ImplicitNormalizeNode"),
        NodeItem("ImplicitVectorDotNode")
    ]),
    ImplicitNodeCategory("TRANSFORM", "Transform Nodes", items=[
      NodeItem("ImplicitRotateNode2DNode")
    ]),
    ImplicitNodeCategory("CSG", "CSG Nodes", items=[
        NodeItem("ImplicitIntersectNode"),
        NodeItem("ImplicitUnionNode")
    ]),
    ImplicitNodeCategory("GROUPS", "Groups", items=get_groups),

#    ImplicitNodeCategory("OTHERNODES", "Other Nodes", items=[
        # the node item can have additional settings,
        # which are applied to new nodes
        # NB: settings values are stored as string expressions,
        # for this reason they should be converted to strings using repr()
#        NodeItem("CustomNodeType", label="Node A", settings={
#            "myStringProperty": repr("Lorem ipsum dolor sit amet"),
#            "myFloatProperty": repr(1.0),
#            }),
#        NodeItem("CustomNodeType", label="Node B", settings={
#            "myStringProperty": repr("consectetur adipisicing elit"),
#            "myFloatProperty": repr(2.0),
#            }),
#        ]),
    ]

nc_register = utils.Registrar.custom(
  lambda: nodeitems_utils.register_node_categories("IMPLICIT_NODES", node_categories),
  lambda: nodeitems_utils.unregister_node_categories("IMPLICIT_NODES")
)

bpy_exports = utils.Registrar([
  nc_register,
  NODE_OT_add_implicit_group
])
