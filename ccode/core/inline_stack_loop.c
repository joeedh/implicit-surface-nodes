#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#include "simd.h"
#include "surface.h"

#include "Alloc.h"
#include "mesh.h"

#ifdef EXPORT
#undef EXPORT
#endif

#define EXPORT INLINE

#include "funcs.c"
#include "func_perlin.c"

floatf sm_run_inline(StackMachine *sm, SMOpCode *codes, int codelen) {
  floatf registers[MAXREGISTER] MYALIGNED(16) = {0}; //hrm, wonder if compiler will put these in actual registers for me. . .
  SMOpCode *curcode = codes;
  
  sm->registers = registers;
  
  #ifdef DEBUG_SM
  printf("\n\nStarting run: %i %p %p\n", codelen, curcode, sm);
  #endif
  
  while (curcode < codes + codelen) {
    #ifdef DEBUG_SM
    printf("%s %i %i %i %p : %d\n", get_opcode_name(curcode->code), curcode->arg1, curcode->arg2, curcode->arg3, sm->func_table, sm->stackcur);
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
        sm->stack[sm->stackcur++] = sm->globals[loc];
        break;
      }
      case LOAD_GLOBAL:
      {
        int loc = curcode->arg1 | curcode->arg2<<16;
        sm->stack[sm->stackcur-1] = sm->globals[loc];
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
      {
        floatf f1, f2, f;
        
        f1 = sm->registers[curcode->arg1];
        f2 = sm->registers[curcode->arg2];
        
        #ifdef SIMD
        
        f[0] = pow(f1[0], f2[0]);
        f[1] = pow(f1[1], f2[1]);
        f[2] = pow(f1[2], f2[2]);
        f[3] = pow(f1[3], f2[3]);
        
        #else
          
        f = pow(f1, f2);
        
        #endif
      
        sm->registers[curcode->arg3] = f; //pow(sm->registers[curcode->arg1], sm->registers[curcode->arg2]);
        break;
      }
      case FUNC_CALL: //args: [func name id]
      {
        switch (curcode->arg1) {
          case 0:
            sm_sqrt(sm);
            break;
          case 1:
            sm_abs(sm);
            break;
          case 2:
            sm_min(sm);
            break;
          case 3:
            sm_max(sm);
            break;
          case 4:
            sm_cos(sm);
            break;
          case 5:
            sm_sin(sm);
            break;
          case 6:
            sm_acos(sm);
            break;
            break;
          case 7:
            sm_asin(sm);
            break;
            break;
          case 8:
            sm_atan(sm);
            break;
            break;
          case 9:
            sm_atan2(sm);
            break;
            break;
          case 10:
            sm_log(sm);
            break;
          case 11:
            sm_pow(sm);
            break;
          case 12:
            sm_floor(sm);
            break;
          case 13:
            sm_ceil(sm);
            break;
          case 14:
            sm_fract(sm);
            break;
          case 15:
            sm_perlin(sm);
            break;
          case 16:
            sm_perlin_dv(sm);
            break;
          case 17:
            sm_length(sm);
            break;
          case 18:
            sm_trunc(sm);
            break;
          case 19:
            sm_isqrt(sm);
            break;
		  case 20:
			sm_sample_distfield(sm);
			break;
        }
        
        break;
      }
      default:
        printf("Unimplemented opcode %d!\n", curcode->code);
        break;
      
    }
    curcode++;
  }
  
  sm->registers = NULL;
  return sm->stack[sm->stackcur-1];
}
