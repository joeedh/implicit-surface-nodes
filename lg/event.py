import bpy, traceback

from .utils import Registrar

jobs = []
loop_running = False

def add_job(job):
  #job.__next__()
  jobs.append(job)

def kill_job(job):
  jobs.remove(job)
  
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
  ModalTimerOperator
]);
