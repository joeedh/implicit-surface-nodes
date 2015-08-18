#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surface.h"
#include "funcs.h"

#include "Alloc.h"
#include "mesh.h"

#define MAXTHREAD 1

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
};

FuncTable *sm_get_functable(int *sizeout) {
  *sizeout = sizeof(func_table) / sizeof(*func_table);
  
  return func_table;
}

StackMachine *sm_new() {
  StackMachine *sm = MEM_malloc(sizeof(StackMachine));
  memset(sm, 0, sizeof(*sm));
  
  sm->func_table = func_table;
  sm->totfunc = sizeof(func_table) / sizeof(*func_table);
}

void sm_free(StackMachine *sm) {
  if (sm->codes)
    MEM_free(sm->codes);
  
  MEM_free(sm);
}

//codes is copied to its own buffer
void sm_add_opcodes(StackMachine *sm, SMOpCode *codes, int totcode) {
  sm->codes = MEM_malloc(sizeof(SMOpCode)*totcode);
  sm->totcode = totcode;
  
  memcpy(sm->codes, codes, sizeof(SMOpCode)*totcode);
}

void sm_add_constant(StackMachine *sm, float constant) {
  sm->constants[sm->totconstant++] = constant;
}

void sm_throw(StackMachine *sm, char *message) {
  if (sm->exception) {
    fprintf(stderr, "Cannot throw '%s': Exception already set!\n", message);
  }
  
  strncpy(sm->except_msg, message, sizeof(sm->except_msg)-1);
  sm->except_msg[sizeof(sm->except_msg)-1] = 0;
  sm->exception = 1;
}

float sm_get_stackitem(StackMachine *sm, int stackpos) {
  return sm->stack[stackpos];
}

void sm_set_stackcur(StackMachine *sm, int stackcur) {
  sm->stackcur = stackcur;
}

void sm_set_global(StackMachine *sm, int stackpos, float value) {
  sm->stack[stackpos] = value;
}

static StackMachine *sampler_machines[MAXTHREAD];

void sm_set_sampler_machine(StackMachine *sm, int thread) {
  sampler_machines[thread] = sm;
}

float sm_sampler(float x, float y, float z, int thread) {
    StackMachine *sm = sampler_machines[thread];
    
    sm->stack[0] = x;
    sm->stack[1] = y;
    sm->stack[2] = z;
    
    sm->stackcur = 20;
    
    return sm_run(sm, sm->codes, sm->totcode);
}

void sm_tessellate(float **vertout, int *totvert, int **triout, int *tottri,
                    float min[3], float max[3], int ocdepth, int thread) {
  polygonize(sm_sampler, vertout, totvert, triout, tottri, min, max, ocdepth, thread);
}

void sm_free_tess(float *vertout, int *triout) {
  MEM_free(vertout);
  MEM_free(triout);
}

float sm_run(StackMachine *sm, SMOpCode *codes, int codelen) {
  SMOpCode *curcode = codes;
  
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
  
  return sm->stack[sm->stackcur-1];
}
