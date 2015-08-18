from . import utils

import bpy


def main(operator, context):
    space = context.space_data
    node_tree = space.node_tree
    node_active = context.active_node
    node_selected = context.selected_nodes

    # now we have the context, perform a simple operation
    if node_active in node_selected:
        node_selected.remove(node_active)
    if len(node_selected) != 1:
        operator.report({'ERROR'}, "2 nodes must be selected")
        return

    node_other, = node_selected

    # now we have 2 nodes to operate on
    if not node_active.inputs:
        operator.report({'ERROR'}, "Active node has no inputs")
        return

    if not node_other.outputs:
        operator.report({'ERROR'}, "Selected node has no outputs")
        return

    socket_in = node_active.inputs[0]
    socket_out = node_other.outputs[0]

    # add a link between the two nodes
    node_link = node_tree.links.new(socket_in, socket_out)


class ImplicitMakeGroup(bpy.types.Operator):
    """Make newgroup node from selected nodes"""
    bl_idname = "node.implicit_make_group"
    bl_label = "Make Group (implicit)"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
      return True
      space = context.space_data
      
      if space.type != "NODE_EDITOR":
        return False
        
      ntree = space.node_tree
      if ntree == undefined:
        return False
      
      if ntree.bl_idname != 'ImplicitTreeType':
        return False
        
      return True

    def execute(self, context):
      space = context.space_data
      ntree = space.node_tree
      active = context.active_node
      selected = context.selected_nodes
      selected = list(selected)
      
      ntree2 = bpy.data.node_groups.new('Group', 'ImplicitTreeType')
      
      nmap = {}
      sockmap = {}
      
      for n in selected:
        nmap[utils.nodeid(selected, n)] = n
        
        for i in n.inputs:
          sockmap[utils.sockid(selected, i)] = i
        for o in n.outputs:
          sockmap[utils.sockid(selected, o)] = o
          
      #build group input/outputs
      group_ins = {}
      group_outs = {}
      
      for n in selected:
        nmap[utils.nodeid(selected, n)] = n
        
        for i in n.inputs:
          isid = utils.sockid(selected, i)
          
          for l in i.links:
            sid = utils.sockid(selected, l.from_socket)
            
            if sid not in sockmap:
              if isid not in group_ins:
                group_ins[isid] = []
              group_ins[isid].append(l.from_socket)
              
        for i in n.outputs:
          osid = utils.sockid(selected, o)
          
          for l in o.links:
            sid = utils.sockid(selected, l.to_socket)
            
            if sid not in sockmap:
              if osid not in group_outs:
                group_outs[osid] = []
              group_outs[osid].append(l.to_socket)
      
      nmap2 = {}
      sockmap2 = {}
      newnodes = []
      
      for n in selected:
        n2 = ntree2.nodes.new(n.bl_idname)
        newnodes.append(n2)
        
        n2.copy(n)
        n2.location = n.location
        n2.width = n.width
        n2.height = n.height
        n2.width_hidden = n.width_hidden
      
      for n2 in newnodes:
        nmap2[utils.nodeid(newnodes, n2)] = n2
        for i in n2.inputs:
          sid = utils.sockid(newnodes, i)
          sockmap2[sid] = i
          i.value = sockmap[sid].value
          
        for o in n2.outputs:
          sid = utils.sockid(newnodes, o)
          sockmap2[sid] = o
          o.value = sockmap[sid].value
          
      xmin = 4000
      ymin = 4000
      xmax = -4000
      ymax = -4000
      
      for n in selected:
        xmin = min(xmin, n.location[0])
        xmax = max(xmax, n.location[0]+n.width)
        ymin = min(ymin, n.location[1])
        ymax = max(ymax, n.location[1]+n.height)

      #group node in original node tree
      gn = ntree.nodes.new('ImplicitNodeGroup')
      gn.node_tree = ntree2
      gn.location[0] = (xmin+xmax)*0.5-100
      gn.location[1] = (ymin+ymax)*0.5-100
      
      print(list(sockmap.keys()))
      print(list(sockmap2.keys()))
            
      group_in  = ntree2.nodes.new("NodeGroupInput")
      group_in.location[0] = xmin - 250;
      group_in.location[1] = (ymin+ymax)*0.5 - 150;
      
      group_out = ntree2.nodes.new("NodeGroupOutput")
      group_out.location[0] = xmax + 100;
      group_out.location[1] = (ymin+ymax)*0.5 - 150;
      
      group_inmap = {}
      group_outmap = {}
      
      for i, sid in enumerate(group_ins):
        sock = sockmap2[sid]
        insock = ntree2.inputs.new(sock.bl_idname, sock.name)
                
        for sock2 in group_ins[sid]:
          ntree.links.new(sock2, gn.inputs[i])
          group_inmap[utils.sockid(ntree.nodes, sock2)] = insock
        
      for i, sid in enumerate(group_outs):
        sock = sockmap2[sid]
        outsock = ntree2.outputs.new(sock.bl_idname, sock.name)
                
        for sock2 in group_outs[sid]:
          group_outmap[utils.sockid(ntree.nodes, sock2)] = [outsock, sock2, i]
        
      #group_in.input_template(bpy.types.NodeInternalSocketTemplate(
      for n in selected:
        for o in n.outputs:
          for l in o.links:
            sock = l.to_socket
            
            sid1 = utils.sockid(selected, sock)            
            sid2 = utils.sockid(selected, o)
              
            print(":::::::: sid1, sid2:", sid1, sid2, utils.sockid(selected, l.to_socket), utils.sockid(selected, l.from_socket))
            #continue
            
            if sid1 not in sockmap2:
              if sid2 not in sockmap2:
                print("EEK!!!!!!!!!!")
                continue;
              
              sid3 = utils.sockid(ntree.nodes, sock)
              outsock = group_outmap[sid3][0]
              
              idx = list(ntree2.outputs).index(outsock)
              ntree2.links.new(sockmap2[sid2], group_out.inputs[idx])
              
        for i in n.inputs:
          for l in i.links:
            sock = l.from_socket
            
            sid1 = utils.sockid(selected, sock)            
            sid2 = utils.sockid(selected, i)
              
            if sid1 not in sockmap2:
              if sid2 not in sockmap2:
                continue;
              
              sid3 = utils.sockid(ntree.nodes, sock)
              outsock = group_inmap[sid3]
              
              idx = list(ntree2.inputs).index(outsock)
              ntree2.links.new(group_in.outputs[idx], sockmap2[sid2])
              continue
            
            """
            if sid2 not in sockmap2:
              if sid1 not in sockmap2:
                continue;
              
              sid3 = utils.sockid(ntree.nodes, sock)
              outsock = group_inmap[sid3]
              
              idx = list(ntree2.outputs).index(outsock)
              ntree2.links.new(group_out.inputs[idx], sockmap2[sid2])
              continue
            #"""
            
            s1 = sockmap2[sid1]
            s2 = sockmap2[sid2]
            
            ntree2.links.new(s2, s1)
      
      for k in group_outmap:
        c = group_outmap[k]
        ntree.links.new(c[1], gn.outputs[c[2]])
      
      selected = list(selected)
      for n in selected:
        ntree.nodes.remove(n)
        
      print("push node group")
      space.path.append(ntree2)
      
      return {'FINISHED'}

class ImplicitEditGroup(bpy.types.Operator):
    """Make newgroup node from selected nodes"""
    bl_idname = "node.implicit_edit_group"
    bl_label = "Edit Group (implicit)"
    
    node_path = bpy.props.StringProperty(name="node_path")
    
    @classmethod
    def poll(cls, context):
      return True
      space = context.space_data
      
      if space.type != "NODE_EDITOR":
        return False
        
      ntree = space.node_tree
      if ntree == undefined:
        return False
      
      if ntree.bl_idname != 'ImplicitTreeType':
        return False
        
      return True

    def execute(self, context):
      space = context.space_data
      ntree = space.node_tree
      active = context.active_node
      
      if self.node_path != None:
        node = utils.get_node_from_path(self.node_path)
      else:
        node = active
      
      space.path.append(node.node_tree)
      
      return {'FINISHED'}
      
bpy_exports = utils.Registrar([
  ImplicitMakeGroup,
  ImplicitEditGroup
])
