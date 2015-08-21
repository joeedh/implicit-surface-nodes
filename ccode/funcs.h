#ifndef _FUNCS_H
#define _FUNCS_H

#include "simd.h"

struct StackMachine;

#ifdef MSC_VER
#define INLINE static __forceline
#else
#define INLINE static inline __attribute__((always_inline)) 
#endif

void sm_sqrt(struct StackMachine *sm);
void sm_abs(struct StackMachine *sm);
void sm_min(struct StackMachine *sm);
void sm_max(struct StackMachine *sm);
void sm_cos(struct StackMachine *sm);
void sm_sin(struct StackMachine *sm);
void sm_acos(struct StackMachine *sm);
void sm_asin(struct StackMachine *sm);
void sm_atan(struct StackMachine *sm);
void sm_atan2(struct StackMachine *sm);
void sm_log(struct StackMachine *sm);
void sm_pow(struct StackMachine *sm);
void sm_floor(struct StackMachine *sm);
void sm_ceil(struct StackMachine *sm);
void sm_fract(struct StackMachine *sm);
void sm_perlin(struct StackMachine *sm);
void sm_perlin_dv(struct StackMachine *sm);
void sm_length(struct StackMachine *sm);
void sm_trunc(struct StackMachine *sm);

#endif /* _FUNCS_H */
