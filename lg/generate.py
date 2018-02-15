import bpy
from math import *
from mathutils import *

from . import codegen
from . import c_code


def generate():
    ob = bpy.context.object
    if ob == None: return
    if ob.name.startswith("__"): return

    from lg.symbol import sym
    from lg import codegen

    #nodetree we'll be working on
    ntree = ob.implicit.node_tree
    if ntree not in bpy.data.node_groups.keys(): return
    ntree = bpy.data.node_groups[ntree]

    cgen = codegen.CodeGen(ntree)
    cgen.sort()
    r = cgen.gen()


    from lg import mesh
    
    outname = "__" + ob.name + "_output"
    if outname not in bpy.data.objects.keys():
        outme = bpy.data.meshes.new(outname)
        outobj = bpy.data.objects.new(outname, outme)
        bpy.context.scene.objects.link(outobj)
    else:
        outobj = bpy.data.objects[outname]
    
    print("Tessellating...", outname, outobj);
    
    #print(r[0].factor())
    r[0] = r[0].factor()

    mesh.surfmesh(r[0], outobj, outmatrix=ob.matrix_world, 
                  use_local=not ob.implicit.global_mode)

    from lg import c_code
    c_code.close_library()

    from lg import appstate
    appstate.start_events()
