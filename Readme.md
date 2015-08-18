=Implicit surface nodes=

This add-on is for building implicit surfaces, with a node-based
equation editor.  Symbolic differentiation is supported.

Note that this is still pre-alpha; you have to run it from within designer.blend.
Also, despite the name, the library binary in ccode is for win64, not linux.

=Optional Requirements=

* Sympy.  Note that you have to add the path to your python lib's site-packages
  in a python script inside designer.blend.  Sympy isn't necassary, if available
  will optimize generated code for speed.
* GCC

=Building=

* In theory, building should just be a matter of doing:
    cd build;
    python build.py;
    
. . . but I've not tested it one anything except windows.