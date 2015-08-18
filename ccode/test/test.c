#include "../surface.h"
#include <stdio.h>
#include <math.h>

/*
stack machine:

stack globals:
x
y
z

consts:
0.5 : 0
1.0 : 1
0.0 : 2
2.0 : 3

*/

SMOpCode test_codes[] = {
  {PUSH, 7, 0, 0}, //return value will go here
  {PUSH, 7, 0, 0}, //return value will go here
  
  {PUSH_GLOBAL, 0, 0, 0}, //x
  {PUSH_GLOBAL, 1, 0, 0}, //y
  
  {FUNC_CALL, 3, 0, 0},  //call max
  
  {PUSH_GLOBAL, 2, 0, 0}, //z
  
  {FUNC_CALL, 3, 0, 0}, //call max again
  //stack should now have result of max(max(x, y), z);
  
  {STK_TO_REG, 0, 0, 0},
  {CONST_TO_REG, 1, 0, 0},
  {SUB, 0, 1, 0},
  {REG_TO_STK, 0, 0, 0}
  //stack should now have result of max(max(x, y), z) - 0.5;
};
int testcodelen = sizeof(test_codes) / sizeof(*test_codes);

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int simple_test() {
  StackMachine *sm = sm_new();
  int i;
  float f, x, y, z;
  
  sm_add_constant(sm, 0.5);
  sm_add_constant(sm, 1.0);
  sm_add_constant(sm, 0.0);
  sm_add_constant(sm, 2.0);
  
  x = 0.11;
  y = 0.71;
  z = 0.223;
  
  sm_set_global(sm, 0, x);
  sm_set_global(sm, 1, y);
  sm_set_global(sm, 2, z);
  sm_set_stackcur(sm, 10);
  
  sm_add_opcodes(sm, test_codes, testcodelen);
  f = sm_run(sm, test_codes, testcodelen);
  
  printf("RESULT: %f, should be: %f\n", f, MAX(MAX(x, y), z)-0.5);
  printf("[");
  
  for (i=0; i<sm->stackcur+10; i++) {
    if (i > 0) {
      printf(", ");
    }
    printf("%.2f", sm->stack[i]);
  }
  printf("]\n");
}

int complicated_test() {
  StackMachine *sm = sm_new();
  int i, totvert, tottri;
  float *verts;
  int *tris;
  float f, x, y, z;
  float min[3] = {-1.0, -1.0, -1.0}, max[3] = {1.0, 1.0, 1.0};
  
  sm_add_constant(sm, 0.5);
  sm_add_constant(sm, 1.0);
  sm_add_constant(sm, 0.0);
  sm_add_constant(sm, 2.0);
  
  x = 0.11;
  y = 0.71;
  z = 0.223;
  
  sm_set_global(sm, 0, x);
  sm_set_global(sm, 1, y);
  sm_set_global(sm, 2, z);
  sm_set_stackcur(sm, 10);
  
  sm_add_opcodes(sm, test_codes, testcodelen);
  sm_set_sampler_machine(sm, 0);
  
  f = sm_sampler(x, y, z, 0);
  
  //f = sm_run(sm, sm->codes, sm->totcode);
  printf("RESULT: %f, should be: %f\n", f, MAX(MAX(x, y), z)-0.5);
  printf("[");
  
  for (i=0; i<sm->stackcur+10; i++) {
    if (i > 0) {
      printf(", ");
    }
    printf("%.2f", sm->stack[i]);
  }
  printf("]\n");
  
  //void sm_tessellate(float **vertout, int *totvert, int **triout, int *tottri,
  //                  float min[3], float max[3], int ocdepth, int thread) {
  sm_tessellate(&verts, &totvert, &tris, &tottri, min, max, 3, 0);
  printf("\ntotvert: %d, tottri: %d\n", totvert, tottri);
  printf("%p %p", verts, tris);
  
  sm_free_tess(verts, tris);
}

int main(char **argv, int argc) {
  //simple_test()
  complicated_test();
}
