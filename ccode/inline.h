#ifndef _INLINE_H
#define _INLINE_H

#ifdef _MSC_VER
#define INLINE static __forceinline
#else
#define INLINE static inline __attribute__((always_inline)) 
#endif

#include "simd.h"

#define TRUNC(f) ((float)((int)(f)))

INLINE floatf fast_floor(floatf f) {
  floatf f2 = f;
  //floatf r = {
  //  floor(f[0]),
  //  floor(f[1]),
  //  floor(f[2]),
  //  floor(f[3])
  //};
  
  ///return r;
#ifdef SIMD
  f2[0] = floor(f[0]);
  f2[1] = floor(f[1]);
  f2[2] = floor(f[2]);
  f2[3] = floor(f[3]);
  
  //f2[0] = TRUNC(f[0]); f2[1] = TRUNC(f[1]); f2[2] = TRUNC(f[2]); f2[3] = TRUNC(f[3]);
  //f2[0] -= f[0] != f2[0] && f[0] < 0 ? 1.0 : 0.0;
  //f2[1] -= f[1] != f2[1] && f[1] < 0 ? 1.0 : 0.0;
  //f2[2] -= f[2] != f2[2] && f[2] < 0 ? 1.0 : 0.0;
  //f2[3] -= f[3] != f2[3] && f[3] < 0 ? 1.0 : 0.0;
  
  /*
  f2[0] = TRUNC(f[0]) - (TRUNC(f[0]) != f[0] && f[0] < 0.0);
  f2[1] = TRUNC(f[1]) - (TRUNC(f[1]) != f[1] && f[1] < 0.0);
  f2[2] = TRUNC(f[2]) - (TRUNC(f[2]) != f[2] && f[2] < 0.0);
  f2[3] = TRUNC(f[3]) - (TRUNC(f[3]) != f[3] && f[3] < 0.0);
  */
#else
  f2 = (float)((int)f2);
  f2 -= (floatf)((f < 0.0f)*(f2 != f));
#endif  
  //f2 -= (floatf)((f < 0.0f)*(f2 != f));
  
  return f2;
}

INLINE floatf fast_ceil(floatf f) {
  floatf f2 = ((floatf)((intf)f));
//  f2 += (floatf)((f > 0.0f)*(f2 != f));
#ifdef SIMD
  f2[0] = (float)((int)f2[0]);
  f2[1] = (float)((int)f2[1]);
  f2[2] = (float)((int)f2[2]);
  f2[3] = (float)((int)f2[3]);
  
  f2[0] += (f2[0] != f[0] && f[0] > 0.0);
  f2[1] += (f2[1] != f[1] && f[1] > 0.0);
  f2[2] += (f2[2] != f[2] && f[2] > 0.0);
  f2[3] += (f2[3] != f[3] && f[3] > 0.0);
#else
  f2 = (float)((int)f2);
  f2 += (floatf)((f > 0.0f)*(f2 != f));
#endif

  return f2;
}
#endif /* _INLINE_H */

