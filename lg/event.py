import bpy, traceback

from .utils import Registrar
from . import globals

loop_running = False

def add_timer(cb):
  globals.timers.append(cb)
  bpy.app.timers.register(cb)
def rem_timer(cb):
  bpy.app.timers.unregister(cb)
  globals.timers.remove(cb)

def clear_timers():
  for t in globals.timers[:]:
    try:
      globals.timers.remove(t)
    except:
      traceback.print_exc()

def add_job(job):
  #job.__next__()
  globals.jobs.append(job)

def kill_job(job):
  globals.jobs.remove(job)

def do_jobs():
  jobs = globals.jobs
  context = bpy.context

  for job in jobs[:]:
    try:
      job.send(context)
    except TypeError:
      print("first run of job?")
      job.__next__() #generator was just started?
    except StopIteration:
      jobs.remove(job)
    except:
      traceback.print_exc()
      jobs.remove(job)
  
  return 0.05

def clear_jobs():
  globals.jobs[:] = []

clear_timers()
clear_jobs()

def start_events():
  add_timer(do_jobs)

class ModalTimerOperator(bpy.types.Operator):
    """Operator which runs its self from a timer"""
    bl_idname = "wm.implicit_event_loop"
    bl_label = "Event Loop For Implicit Node Editor"

    _timer = None

    def modal(self, context, event):
      global jobs
      
      if event.type in {'ESC'}:
          self.cancel(context)
          return {'CANCELLED'}

      if event.type == 'TIMER':
        for job in jobs[:]:
          try:
            job.send(context)
          except TypeError:
            print("first run of job?")
            job.__next__() #generator was just started?
          except StopIteration:
            jobs.remove(job)
          except:
            traceback.print_exc()
            jobs.remove(job)
          
      return {'PASS_THROUGH'}

    def execute(self, context):
      global loop_running
      
      if loop_running:
        print("loop already running!")
        return {'CANCELLED'}

      loop_running = True  
      wm = context.window_manager
      self._timer = wm.event_timer_add(0.1, context.window)
      wm.modal_handler_add(self)
      return {'RUNNING_MODAL'}

    def cancel(self, context):
        global loop_running
        
        wm = context.window_manager
        wm.event_timer_remove(self._timer)
        
        loop_running = False
        return {'CANCELLED'}

bpy_exports = Registrar([
#  ModalTimerOperator
]);
