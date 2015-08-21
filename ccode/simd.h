#ifndef _SIMD_H
#define _SIMD_H

#define SIMD

#ifdef SIMD
typedef float floatf __attribute__ ((vector_size (16)));
typedef int   intf   __attribute__ ((vector_size (16)));
#else
typedef float floatf;
typedef int   intf;
#endif

#endif /* _SIMD_H */
