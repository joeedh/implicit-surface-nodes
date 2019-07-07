#ifndef _SURFACE_H
#define _SURFACE_H

#include "simd.h"

typedef floatf v4sf;
typedef intf   v4si;

/*
*/

//stack locs always use two args to get a full 32-bit integer
enum OpCode {
  PUSH, //0: args: register to push onto stack
  POP,  //1: args: register to pop stack value onto
  CONST_TO_REG, //2
  CONST_TO_STK, //3
  ABS_STK_TO_REG, //4
  REG_TO_STK, //args: [stack loc, register loc]
  STK_TO_REG, //args: [stack loc, register loc]
  MUL,  //args: [register a, register b, register out]
  DIV,  //args: [register a, register b, register out]
  ADD,  //args: [register a, register b, register out]
  SUB,  //args: [register a, register b, register out]
  GT,   //args: [register a, register b, register out]
  LT,   //args: [register a, register b, register out]
  GTE,  //args: [register a, register b, register out]
  LTE,  //args: [register a, register b, register out]
  EQ,   //args: [register a, register b, register out]
  LOGICAL_NEGATE,  //args: [register a, register b, register out]
  NEGATE, //args: [register a, register out]
  POW,    //args: [register a, register out]
  FUNC_CALL, //args: [func name id]
  PUSH_GLOBAL, //args: [stack loc] remember stack locs use arg1 and arg2
  LOAD_GLOBAL
};

#define _(na) case na: return #na;
static char *get_opcode_name(short opcode) {
  switch(opcode) {
    _(PUSH)
    _(POP)
    _(CONST_TO_REG)
    _(CONST_TO_STK)
    _(ABS_STK_TO_REG)
    _(REG_TO_STK)
    _(STK_TO_REG)
    _(MUL)
    _(DIV)
    _(ADD)
    _(SUB)
    _(GT)
    _(LT)
    _(LTE)
    _(EQ)
    _(LOGICAL_NEGATE)
    _(NEGATE)
    _(POW)
    _(FUNC_CALL)
    _(PUSH_GLOBAL)
    _(LOAD_GLOBAL)
    default:
      return "(bad opcode)";
  }
}
#undef _

#define MAXSTACK    1024*128
#define MAXCONST    1024*8
#define MAXREGISTER 8

typedef struct SMOpCode {
  short code;
  short arg1, arg2, arg3;
} SMOpCode;

struct StackMachine;

typedef struct FuncTable {
  void (*function)(struct StackMachine *sm);
  const char *name;
  int totarg; 
} FuncTable;

typedef struct StackMachine {
  floatf stack[MAXSTACK] MYALIGNED(32);
  floatf globals[128] MYALIGNED(32);
  floatf *registers;
  floatf constants[MAXCONST] MYALIGNED(32);
  
  SMOpCode *codes;
  int totcode, threadnr;
  
  char except_msg[512];
  int exception;
  
  int totconstant;
  
  int stackcur;
  FuncTable *func_table;

  int totfunc, totglobal;
} StackMachine;

struct SpatialGraph;

MYEXPORT FuncTable *sm_get_functable(int *sizeout);
MYEXPORT StackMachine *sm_new();
MYEXPORT StackMachine *sm_copy(StackMachine *sm);
MYEXPORT void sm_free(StackMachine *sm);

//codes is copied to its own buffer
MYEXPORT void sm_add_opcodes(StackMachine *sm, SMOpCode *codes, int totcode);
MYEXPORT void sm_add_constant(StackMachine *sm, float constant);
MYEXPORT void sm_throw(StackMachine *sm, char *message);
MYEXPORT void sm_set_stackcur(StackMachine *sm, int stackcur);
MYEXPORT void sm_set_global(StackMachine *sm, int stackpos, float value);
MYEXPORT floatf sm_run(StackMachine *sm, SMOpCode *codes, int codelen);
MYEXPORT floatf sm_get_stackitem(StackMachine *sm, int stackpos);
MYEXPORT void sm_begin(StackMachine* sm); //basically, resets stack pointer

MYEXPORT void sm_tessellate(struct SpatialGraph *sg, float **vertout, float **ao_out, int *totvert, int **triout, int *tottri,
                   float min[3], float max[3], int ocdepth, float matrix[4][4], 
                   int thread);
MYEXPORT void sm_free_tess(float *vertout, int *triout);
MYEXPORT void sm_set_sampler_machine(StackMachine *sm);
MYEXPORT floatf sm_sampler(floatf x, floatf y, floatf z, int thread);

//v4sf sm_sampler_v4sf(v4sf x, v4sf y, v4sf z, int thread);
MYEXPORT void sm_print_stack(StackMachine *sm, int start, int end);

//#define DEBUG_SM
/*
#ifdef DEBUG_SM
#define SPOP(sm)        (printf("pop: %d\n", (int)sm->stackcur), sm->stack[--sm->stackcur])
#define SPUSH(sm, val) (sm->stack[sm->stackcur++] = (val))
#define SLOAD(sm, val) (printf("load: %d\n", (int)sm->stackcur), sm->stack[sm->stackcur-1] = (val))
#else*/
#define SPOP(sm)       (sm->stack[--sm->stackcur])
#define SPUSH(sm, val) (sm->stack[sm->stackcur++] = (val))
#define SLOAD(sm, val) (sm->stack[sm->stackcur-1] = (val))
//#endif

#endif /* _SURFACE_H */

