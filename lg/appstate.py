import bpy

def on_object_active(ctx, object):
  print("selected new active object", object.name)
  
  if object == None: return
  #find node editor window, if one exists
  scr = bpy.context.window.screen
  node_editor = None
  
  for area in scr.areas:
    if area.type != "NODE_EDITOR": continue
    sn = area.spaces[0]
    if sn.tree_type != "ImplicitTreeType": continue
    
    node_editor = sn
    break;
  
  if node_editor == None: return
  
  tree = object.implicit.node_tree
  print(tree, tree in bpy.data.node_groups.keys())
  
  keys = bpy.data.node_groups.keys()
  if tree in keys and bpy.data.node_groups[tree].bl_idname == "ImplicitTreeType":
    sn.node_tree = bpy.data.node_groups[tree]
    
  #ensure node trees don't get deleted
  node_editor.node_tree.id_data.use_fake_user = True
  
  
  
last_active_object = None
last_active_scene = None

def start_events():
  from . import event
  bpy.ops.wm.implicit_event_loop()
  
  def inf_job(cb):
    while 1:
      ctx = yield 1
      cb(ctx)
      
      
  def check_active_obj(ctx):
    global last_active_object
    global last_active_scene
    
    scene = ctx.scene
    obj = ctx.active_object
    
    if last_active_scene == None or scene.name != last_active_scene.name:
      last_active_scene = scene
    if last_active_object == None or obj.name != last_active_object.name:
      last_active_object = obj
      on_object_active(ctx, obj)
      
  event.add_job(inf_job(check_active_obj).__iter__())