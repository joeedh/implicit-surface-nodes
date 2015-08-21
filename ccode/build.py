import os, sys, os.path, shelve, struct
from math import *
from time import *
from random import *
import glob

basepath = os.curdir
sep = os.path.sep

#release
CFLAGS = " -c -funsigned-char -O3 "

#debug
CFLAGS = " -c -g -funsigned-char -O0 "

CFLAGS += " -Iwinpthread "

CC = "gcc "

def buildcmd(file):
  return CC + file + " " + CFLAGS

mainfile = "surface.c"
files = glob.glob("*.c") + glob.glob("*.h");

db = None
dbcount = 0

def scan_deps():
  deps = {}
  
  for f in files:
    deps[f] = set()
    
    path = basepath + sep + f
    file = open(path, "r")
    buf = file.read()
    file.close()
    
    lines = [l.strip() for l in buf.replace("\r", "").split("\n")]
    for l in lines:
      if not l.startswith("#include"): continue;
      
      ec = "\""
      i = l.find("\"")
      if i < 0:
        i = l.find("'")
        ec = "'"
      if i < 0:
        i = l.find("<")
        ec = ">"
      if i < 0:
        print("Syntax error in include define")
        continue
      
      j = l[i+1:].find(ec) + i + 1
      
      path = l[i+1:j]
      deps[f].add(path)
    
  return deps
  
def open_db():
  global db, dbcount
  
  if db == None:
    db = shelve.open(".build", "c")
  dbcount += 1
  return db
  
def close_db():
  global db, dbcount
  
  dbcount -= 1
  if dbcount <= 0 and db != None:
    db.close()
    db = None

args = sys.argv[1:];
build_all = False
if len(args) == 1 and args[0] == "clean":
  build_all = True
  print("clean build. . .")
  
def filter(path, fname, deps):
  stat = os.stat(path)

  db = open_db()
  
  if path not in db or stat.st_mtime > db[path]["time"]: return True
  if build_all: return True
  
  close_db()

  visit = set()
  def check_dep(fname):
    if fname in visit: return False
    
    visit.add(fname)
    
    for f in deps[fname]:
      if not os.path.exists(f): continue
      
      stat = os.stat(f)
      if stat.st_mtime > db[f]["time"]: return True
      
      if check_dep(f): return True
    
    return False
    
  return check_dep(fname)

def touch(path, set_to_current=False):
  stat = os.stat(path)

  db = open_db()
  
  t = 0
  if set_to_current:
    t = stat.st_mtime
  
  #print("t", t, path);
  db[path] = {"time" : t};
  
  close_db()
  return False
  
def changed_files(deps, files):
  db = open_db()
  build_files = []
  
  rebuild_main = False
  has_main = False
  
  for f in files:
    path = basepath + sep + f
    
    if path in build_files: continue
    
    if filter(path, f, deps):
      if mainfile.lower() in f.strip().lower(): has_main = True
      
      if f.endswith(".h"):
        touch(path, True)
        rebuild_main = True
        continue
      build_files.append(path)
  
  if rebuild_main and not has_main:
    build_files.append(basepath + sep + mainfile)
    touch(basepath + sep + mainfile)
  
  close_db()
  return build_files

def build_files(deps, files):
  db = open_db()
  for path in files:
    if path.endswith(".h"): continue
    
    print("building ", path+"...")
    
    cmd = buildcmd(path)
    ret = os.system(cmd)
    
    if ret == 0:
      stat = os.stat(path)
      db[path] = {"time" : stat.st_mtime}
    else:
      print("\nerror during compiling!")
      return 1
  
  for k in deps:
    for k2 in deps[k]:
      if not os.path.exists(k2): continue
      touch(k2, True)
    touch(k, True)
        
  close_db()
  return 0

def link_files(files):
  cmd = "gcc -shared winpthread/libpthreadGC2.a *.o -o libsurface.so"
  ret = os.system(cmd)
  
  if ret != 0:
    print("\nerror during linking!")
    return 1;
    
  return 0

def build():
  deps = scan_deps()
  
  files2 = changed_files(deps, files)
  if build_files(deps, files2): return 1
  print("linking...")
  if link_files(files): return 1
  print("\nsuccess!\n")
  
def do_loop():
  bad = 0
  while 1:
    files2 = changed_files(files)
    if len(files2) > 0:
      print("yay, build")
      bad = build()
    
    if bad:
      bad = build()
      
    if bad:
      sleep(0.5)
    sleep(0.3)
    
if len(args) > 0 and args[0] == "loop":
  do_loop()
else:
  build()