#include "simd.h"
#include "surface.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifndef EXPORT
#define EXPORT
#endif

#include "inline.h"

//1 / sqrt(f)
static floatf fast_isqrt(floatf n) {
  intf v = *(intf*)&n;
  floatf f;
  
  v = 0x5f3759df - (v >> 1);
  
  f = *(floatf*)&v;
  f = f*(3.0f-n*f*f)*0.5f;
  
  return f;
}

/* Assumes that float is in the IEEE 754 single precision floating point format
 * and that int is 32 bits. */
static floatf sqrt_approx(floatf z)
{
    intf val_int = *(intf*)&z; // Same bits, but as an int 
    floatf f;
    
    /*
    f[0] = sqrt(z[0]);
    f[1] = sqrt(z[1]);
    f[2] = sqrt(z[2]);
    f[3] = sqrt(z[3]);
    
    return f;
    //*/
    
    val_int -= 1 << 23; // Subtract 2^m. 
    val_int >>= 1;      // Divide by 2. 
    val_int += 1 << 29; // Add ((b + 1) / 2) * 2^m. 

    f = *(floatf*)&val_int;
    
    f = (z + f*f)/(f*2);
    f = (z + f*f)/(f*2);
    
    return f;
}

EXPORT void sm_isqrt(StackMachine *sm) {
  floatf f = SPOP(sm);
  
  f = fast_isqrt(f);
  
  SLOAD(sm, f);
}

EXPORT void sm_sqrt(StackMachine *sm) {
  floatf f = SPOP(sm);
  
  //f = sqrt_approx(f);
  f = sqrt(f);

  SLOAD(sm, f);
}

EXPORT void sm_length(StackMachine *sm) {
  floatf x = SPOP(sm);
  floatf y = SPOP(sm);
  floatf z = SPOP(sm);
  
  floatf l = x*x+y*y+z*z;
  
  //l = l != 0.0 ? sqrt(l) : 0.0;
  l = sqrt_approx(l+0.000001f);
  
  SLOAD(sm, l);
  }

#ifdef SIMD
static v4sf two = {2.0f, 2.0f, 2.0f, 2.0f};
static v4sf one = {1.0f, 1.0f, 1.0f, 1.0f};
#else
static floatf two = 2;
static floatf one = 1;
#endif

EXPORT void sm_abs(StackMachine *sm) {
  floatf f = SPOP(sm);
  
#ifdef SIMD
  //f *= ((floatf)(f>0.0))*two - one;
  floatf ret = {
    f[0] < 0.0 ? 0-f[0]  : f[0],
    f[1] < 0.0 ? 0-f[1]  : f[1],
    f[2] < 0.0 ? 0-f[2]  : f[2],
    f[3] < 0.0 ? 0-f[3]  : f[3],
  };
  //f = fabs(f);
#else
  floatf ret = f * ((f>0.0)*2.0-1.0);
#endif
  
  SLOAD(sm, ret);
}

EXPORT void sm_min(StackMachine *sm) {
  floatf f1 = SPOP(sm);
  floatf f2 = SPOP(sm);
  
  //floatf f = f2;//((floatf)(f1<f2))*f1 + ((floatf)(f1>=f2))*f2;
#ifdef SIMD
  floatf f = {
    MIN(f1[0], f2[0]),
    MIN(f1[1], f2[1]),
    MIN(f1[2], f2[2]),
    MIN(f1[3], f2[3])
  };//*/
#else
  floatf f = MIN(f1, f2);
#endif

  SLOAD(sm, f);
}

EXPORT void sm_max(StackMachine *sm) {
  floatf f1 = SPOP(sm);
  floatf f2 = SPOP(sm);
  
  ///*
#ifdef SIMD
  floatf f = {
    MAX(f1[0], f2[0]),
    MAX(f1[1], f2[1]),
    MAX(f1[2], f2[2]),
    MAX(f1[3], f2[3])
  };//*/
#else
   floatf f = MAX(f1, f2);
#endif
 
  //intf fi = (f1 > f2);//*f1 + ((floatf)(f1<=f2))*f2;
  //floatf f;
  
  //f = -(floatf)(*((intf*)&fi));
  //f = f2 + (f1-f2)*f;
  
  SLOAD(sm, f);
}

EXPORT void sm_cos(StackMachine *sm) {
  floatf f = SPOP(sm);
  
#ifdef SIMD
  f[0] = cos(f[0]);
  f[1] = cos(f[1]);
  f[2] = cos(f[2]);
  f[3] = cos(f[3]);
#else
  f = cos(f);
#endif

  SLOAD(sm, f);
}

EXPORT void sm_sin(StackMachine *sm) {
  floatf f = SPOP(sm);
  
#ifdef SIMD
  f[0] = sin(f[0]);
  f[1] = sin(f[1]);
  f[2] = sin(f[2]);
  f[3] = sin(f[3]);
#else
  f = sin(f);
#endif
  
  SLOAD(sm, f);
}

EXPORT void sm_acos(StackMachine *sm) {
  floatf f = SPOP(sm);
  
#ifdef SIMD
  f[0] = acos(f[0]);
  f[1] = acos(f[1]);
  f[2] = acos(f[2]);
  f[3] = acos(f[3]);
#else
  f = acos(f);
#endif
  
  SLOAD(sm, f);
}

EXPORT void sm_asin(StackMachine *sm) {
  floatf f = SPOP(sm);

#ifdef SIMD
  f[0] = asin(f[0]);
  f[1] = asin(f[1]);
  f[2] = asin(f[2]);
  f[3] = asin(f[3]);
#else
  f = asin(f);
#endif
  
  SLOAD(sm, f);
}


EXPORT void sm_atan(StackMachine *sm) {
  floatf f = SPOP(sm);
  
#ifdef SIMD
  f[0] = atan(f[0]);
  f[1] = atan(f[1]);
  f[2] = atan(f[2]);
  f[3] = atan(f[3]);
#else
  f = atan(f);
#endif
    
  SLOAD(sm, f);
}

EXPORT void sm_atan2(StackMachine *sm) {
  floatf f1 = SPOP(sm);
  floatf f2 = SPOP(sm);
  floatf f;
  
#ifdef SIMD
  f[0] = atan2(f1[0], f2[0]);
  f[1] = atan2(f1[1], f2[1]);
  f[2] = atan2(f1[2], f2[2]);
  f[3] = atan2(f1[3], f2[3]);
#else
  f = atan2(f1, f2);
#endif

  SLOAD(sm, f);
}

EXPORT void sm_log(StackMachine *sm) {
  floatf f = SPOP(sm);
  
#ifdef SIMD
  f[0] = log(f[0]);
  f[1] = log(f[1]);
  f[2] = log(f[2]);
  f[3] = log(f[3]);
#else
  f = log(f);
#endif
  
  SLOAD(sm, f);
}

EXPORT void sm_pow(StackMachine *sm) {
  floatf f1 = SPOP(sm);
  floatf f2 = SPOP(sm);
  floatf f;

#ifdef SIMD
  f[0] = pow(f1[0], f2[0]);
  f[1] = pow(f1[1], f2[1]);
  f[2] = pow(f1[2], f2[2]);
  f[3] = pow(f1[3], f2[3]);
#else
  f = pow(f1, f2);
#endif

  SLOAD(sm, f);
}

EXPORT void sm_floor(StackMachine *sm) {
  floatf f = SPOP(sm);
  
  f = fast_floor(f);
  
  SLOAD(sm, f);
}

EXPORT void sm_trunc(StackMachine *sm) {
  floatf f = SPOP(sm);
  
  f = (floatf)((intf)f);
  
  SLOAD(sm, f);
}

EXPORT void sm_ceil(StackMachine *sm) {
  floatf f = SPOP(sm);
  f = fast_ceil(f);
  
  SLOAD(sm, f);
}

EXPORT void sm_fract(StackMachine *sm) {
  floatf f = SPOP(sm);
  
  f -= fast_floor(f);
  
  SLOAD(sm, f);
}

EXPORT void sm_sample_distfield(StackMachine *sm) {
	floatf x = SPOP(sm);
	floatf y = SPOP(sm);
	floatf z = SPOP(sm);

	SLOAD(sm, 0.0); //IMPLEMENT ME!
};
