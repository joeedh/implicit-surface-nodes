#define _CRT_SECURE_NO_WARNINGS

#include "thread.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surface.h"
#include "inline_stack_loop.h"
#include "funcs.h"

#include "Alloc.h"
#include "mesh.h"
#include "timer.h"

#include "spatial.h"

static FuncTable func_table[] = {
  {sm_sqrt, "sqrt", 1},
  {sm_abs, "abs", 1},
  {sm_min, "min", 1},
  {sm_max, "max", 1},
  {sm_cos, "cos", 1},
  {sm_sin, "sin", 1},
  {sm_acos, "acos", 1},
  {sm_asin, "asin", 1},
  {sm_atan, "atan", 1},
  {sm_atan2, "atan2", 1},
  {sm_log, "log", 1},
  {sm_pow, "pow", 1},
  {sm_floor, "floor", 1},
  {sm_ceil, "ceil", 1},
  {sm_fract, "fract", 1},
  {sm_perlin, "perlin", 3},
  {sm_perlin_dv, "perlin_dv", 6},
  {sm_length, "length", 3},
  {sm_trunc, "trunc", 1},
  {sm_isqrt, "isqrt", 1},
  {sm_sample_distfield, "sample_distfield", 3}
};

MYEXPORT FuncTable *sm_get_functable(int *sizeout) {
  *sizeout = sizeof(func_table) / sizeof(*func_table);
  
  return func_table;
}

MYEXPORT StackMachine *sm_copy(StackMachine *sm) {
	StackMachine *sm2 = MEM_malloc(sizeof(*sm2));
	
	*sm2 = *sm;

	if (sm2->codes) {
		sm2->codes = MEM_copyalloc(sm2->codes, MEM_size(sm2->codes));
	}

	sm2->registers = NULL;

	return sm2;
}

MYEXPORT StackMachine *sm_new() {
  static StackMachine rets[2048];
  static int cur=0;
  
  StackMachine *sm = rets + cur;
  cur = (cur + 1) & 2047;

  //seriously, I can't allocate on the heap here? mean!
  //StackMachine *sm = MEM_malloc(sizeof(StackMachine));
  
  memset(sm, 0, sizeof(*sm));
  
  sm->func_table = func_table;
  sm->totfunc = sizeof(func_table) / sizeof(*func_table);
  return sm;
}

MYEXPORT void sm_free(StackMachine *sm) {
  if (sm->codes)
    MEM_free(sm->codes);
  
  //MEM_free(sm);
}

//codes is copied to its own buffer
MYEXPORT void sm_add_opcodes(StackMachine *sm, SMOpCode *codes, int totcode) {
  sm->codes = MEM_malloc(sizeof(SMOpCode)*totcode);
  sm->totcode = totcode;
  
  memcpy(sm->codes, codes, sizeof(SMOpCode)*totcode);
}

MYEXPORT void sm_add_constant(StackMachine *sm, float constant) {
#ifdef SIMD
  sm->constants[sm->totconstant][0] = constant;
  sm->constants[sm->totconstant][1] = constant;
  sm->constants[sm->totconstant][2] = constant;
  sm->constants[sm->totconstant][3] = constant;
#else
  sm->constants[sm->totconstant] = constant;
#endif
  sm->totconstant++;
}

MYEXPORT void sm_throw(StackMachine *sm, char *message) {
  if (sm->exception) {
    fprintf(stderr, "Cannot throw '%s': Exception already set!\n", message);
  }
  
  strncpy(sm->except_msg, message, sizeof(sm->except_msg)-1);
  sm->except_msg[sizeof(sm->except_msg)-1] = 0;
  sm->exception = 1;
}

MYEXPORT floatf sm_get_stackitem(StackMachine *sm, int stackpos) {
  return sm->stack[stackpos];
}  

MYEXPORT void sm_set_stackcur(StackMachine *sm, int stackcur) {
  sm->stackcur = stackcur;
}

MYEXPORT void sm_set_global(StackMachine *sm, int stackpos, float value) {
#ifdef SIMD
  sm->stack[stackpos][0] = value;
  sm->stack[stackpos][1] = value;
  sm->stack[stackpos][2] = value;
  sm->stack[stackpos][3] = value;
#else
  sm->stack[stackpos] = value;
#endif
}

static StackMachine *sampler_machines[MAXTHREAD];

MYEXPORT void sm_print_stack(StackMachine *sm, int start, int end) {
  int i;
  
  printf("[");
  for (i=start; i<=end; i++) {
    if (i > start) {
      printf(", ");
    }
    
    #ifdef SIMD
    printf("{%.3f, %.3f, %.3f, %.3f}", sm->stack[i][0], sm->stack[i][1], sm->stack[i][2], sm->stack[i][3]);
    #else
    printf("%.4f", sm->stack[i]);
    #endif
  }
  
  printf("]\n");
}

MYEXPORT void sm_set_sampler_machine(StackMachine *sm) {
  int i;
  
  sampler_machines[0] = sm;
  sm->threadnr = 0;
  
  for (i=1; i<MAXTHREAD; i++) {
    StackMachine *sm2;
    
    sm2 = sm_new();
    memcpy(sm2, sm, sizeof(*sm2));
    
    sm2->codes = MEM_copyalloc(sm->codes, sizeof(SMOpCode)*sm->totcode);
    sm2->registers = NULL;
    sm2->totcode = sm->totcode;
    sm2->threadnr = i;
    
    sampler_machines[i] = sm2;
  }
}

MYEXPORT void sm_destroy_thread_samplers() {
  int i;
  
  for (i=1; i<MAXTHREAD; i++) {
	  if (sampler_machines[i]) {
		  sm_free(sampler_machines[i]);
		  sampler_machines[i] = NULL;
	  }
  }
}

MYEXPORT floatf sm_sampler(floatf x, floatf y, floatf z, int thread, void *userdata) {
	floatf p[3] = { x, y, z };

	return sg_sample(userdata, p, thread);
#if 0
    StackMachine *sm = sampler_machines[thread];
    
    //printf("stack: %p %i\n", sm->stack, (int)(((unsigned long long)sm->stack) % 32));
    
    sm->stack[0] = x;
    sm->stack[1] = y;
    sm->stack[2] = z;
    
    sm->stackcur = 20;
    
    return sm_run_inline(sm, sm->codes, sm->totcode);
    //return sm_run(sm, sm->codes, sm->totcode);
#endif
}

/*
v4sf sm_sampler_v4sf(v4sf x, v4sf y, v4sf z, int thread) {
  v4sf ret = {sm_sampler(x[0], y[0], z[0], thread),
              sm_sampler(x[1], y[1], z[1], thread),
              sm_sampler(x[2], y[2], z[2], thread),
              sm_sampler(x[3], y[3], z[3], thread)
             };
             
  return ret;
}
*/

MYEXPORT void sm_tessellate(SpatialGraph *sg, float **vertout, float **ao_out, int *totvert, int **triout, int *tottri,
                    float min[3], float max[3], int ocdepth, float matrix[4][4], int thread) {
  double start_time = getPerformanceTime();
  
  threaded_polygonize(sm_sampler, vertout, ao_out, totvert, triout, tottri, min, max, ocdepth, matrix, thread, sg);
  
  start_time = getPerformanceTime() - start_time;
  printf("  time taken: %.4lfms\n", start_time);
  
  //sm_print_stack(sampler_machines[thread], 0, 33);
}

MYEXPORT void sm_free_tess(float *vertout, int *triout) {
  MEM_free(vertout);
  MEM_free(triout);
} 

MYEXPORT floatf sm_run(StackMachine *sm, SMOpCode *codes, int codelen) {
#if 0
  float registers[9]={0}; //hrm, wonder if compiler will put these in actual registers for me. . .
  SMOpCode *curcode = codes;
  
  sm->registers = registers;
  
  #ifdef DEBUG_SM
  printf("\n\nStarting run: %i %p %p\n", codelen, curcode, sm);
  #endif
  
  while (curcode < codes + codelen) {
    #ifdef DEBUG_SM
    printf("%s %i %i %i %p %p : %d\n", get_opcode_name(curcode->code), curcode->arg1, curcode->arg2, curcode->arg3, sm->func_table, func_table, sm->stackcur);
    #endif
    
    if (sm->exception) {
      fprintf(stderr, "Detected exception;\n");
      fprintf(stderr, "Exception: %s\n", sm->except_msg);
      break;
    }
    
    switch(curcode->code) {
      case PUSH: //args: register to push onto stack #register 7 is /dev/null?
        sm->stack[sm->stackcur++] = sm->registers[curcode->arg1];
        break;
      case POP: //args: register to pop stack value onto
        sm->registers[curcode->arg1] = sm->stack[--sm->stackcur];
        break;
      case PUSH_GLOBAL:
      {
        int loc = curcode->arg1 | curcode->arg2<<16;
        sm->stack[sm->stackcur++] = sm->stack[loc];
        break;
      }
      case LOAD_GLOBAL:
      {
        int loc = curcode->arg1 | curcode->arg2<<16;
        sm->stack[sm->stackcur-1] = sm->stack[loc];
        break;
      }
      case REG_TO_STK: //args: location in stack, register to load into stack
      {
        int loc = curcode->arg1 | curcode->arg2<<16;
        sm->stack[sm->stackcur-1-loc] = sm->registers[curcode->arg3];
        break;
      }
      case CONST_TO_REG: //[register, index in constant table]
        sm->registers[curcode->arg1] = sm->constants[curcode->arg2];
        break;
      case CONST_TO_STK: //[stack (two args), index in constant table]
      {
        int loc = curcode->arg1 | curcode->arg2<<16;
        
        sm->stack[sm->stackcur-1-loc] = sm->constants[curcode->arg3];
        break;
      }
      case ABS_STK_TO_REG:
      {
        int loc = curcode->arg1 | curcode->arg2<<16;
        
        sm->registers[curcode->arg3] = sm->stack[loc];
        break;
      }
      case STK_TO_REG: //args: location in stack, register to load stack entry into
      {
        int loc = curcode->arg1 | curcode->arg2<<16;
        sm->registers[curcode->arg3] = sm->stack[sm->stackcur-1-loc];
        break;
      }
      case MUL: //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] * sm->registers[curcode->arg2];
        break;
      case DIV: //args: [register a:register b:register out]
        if (sm->registers[curcode->arg2] == 0.0)
          sm->registers[curcode->arg3] = 0.0;
        else
          sm->registers[curcode->arg3] = sm->registers[curcode->arg1] / sm->registers[curcode->arg2];
        break;
      case ADD: //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] + sm->registers[curcode->arg2];
        break;
      case SUB: //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] - sm->registers[curcode->arg2];
        break;
      case GT:  //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] > sm->registers[curcode->arg2];
        break;
      case LT:  //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] < sm->registers[curcode->arg2];
        break;
      case GTE: //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] >= sm->registers[curcode->arg2];
        break;
      case LTE: //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] <= sm->registers[curcode->arg2];
        break;
      case EQ:  //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = sm->registers[curcode->arg1] == sm->registers[curcode->arg2];
        break;
      case LOGICAL_NEGATE: //args: [register a:register out]
        sm->registers[curcode->arg2] = sm->registers[curcode->arg1] == 0.0;
        break;
      case NEGATE: //args: [register a:register out]
        sm->registers[curcode->arg2] = -sm->registers[curcode->arg1];
        break;
      case POW:   //args: [register a:register b:register out]
        sm->registers[curcode->arg3] = pow(sm->registers[curcode->arg1], sm->registers[curcode->arg2]);
        break;
      case FUNC_CALL: //args: [func name id]
        sm->func_table = func_table;
        
        #ifdef DEBUG_SM
        printf("  %p %d\n", sm->func_table, sm->totfunc);
        printf("    %p\n", sm->func_table + curcode->arg1);
        printf("  %s %p\n", sm->func_table[curcode->arg1].name, sm->func_table[curcode->arg1].function);
        #endif
        
        sm->func_table[curcode->arg1].function(sm);
        //return value from called function should now be on stack
        //the called function also popped the arguments, itself
        break;
      default:
        printf("Unimplemented opcode %d!\n", curcode->code);
        break;
      
    }
    curcode++;
  }
  
  sm->registers = NULL;
  return sm->stack[sm->stackcur-1];
#endif 
  return 0.0;
}
