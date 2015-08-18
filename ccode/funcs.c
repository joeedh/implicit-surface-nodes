#include "surface.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void sm_sqrt(StackMachine *sm) {
  float f = SPOP(sm);
  f = sqrt(f);
  
  SLOAD(sm, f);
}

void sm_abs(StackMachine *sm) {
  float f = SPOP(sm);
  f = fabs(f);
  
  SLOAD(sm, f);
}

void sm_min(StackMachine *sm) {
  float f1 = SPOP(sm);
  float f2 = SPOP(sm);
  
  float f = f1 < f2 ? f1 : f2;
  
  SLOAD(sm, f);
}

void sm_max(StackMachine *sm) {
  float f1 = SPOP(sm);
  float f2 = SPOP(sm);
  
  float f = f1 > f2 ? f1 : f2;
  
  SLOAD(sm, f);
}

void sm_cos(StackMachine *sm) {
  float f = SPOP(sm);
  f = cos(f);
  
  SLOAD(sm, f);
}

void sm_sin(StackMachine *sm) {
  float f = SPOP(sm);
  f = sin(f);
  
  SLOAD(sm, f);
}

void sm_acos(StackMachine *sm) {
  float f = SPOP(sm);
  f = acos(f);
  
  SLOAD(sm, f);
}

void sm_asin(StackMachine *sm) {
  float f = SPOP(sm);
  f = asin(f);
  
  SLOAD(sm, f);
}


void sm_atan(StackMachine *sm) {
  float f = SPOP(sm);
  f = atan(f);
  
  SLOAD(sm, f);
}

void sm_atan2(StackMachine *sm) {
  float f1 = SPOP(sm);
  float f2 = SPOP(sm);
  
  float f = atan2(f1, f2);
  
  SLOAD(sm, f);
}

void sm_log(StackMachine *sm) {
  float f = SPOP(sm);
  f = log(f);
  
  SLOAD(sm, f);
}

void sm_pow(StackMachine *sm) {
  float f1 = SPOP(sm);
  float f2 = SPOP(sm);
  
  float f = pow(f1, f2);
  
  SLOAD(sm, f);
}

void sm_floor(StackMachine *sm) {
  float f = SPOP(sm);
  f = floor(f);
  
  SLOAD(sm, f);
}

void sm_ceil(StackMachine *sm) {
  float f = SPOP(sm);
  f = ceil(f);
  
  SLOAD(sm, f);
}

void sm_fract(StackMachine *sm) {
  float f = SPOP(sm);
  
  f -= floor(f);
  
  SLOAD(sm, f);
}
