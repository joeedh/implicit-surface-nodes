#ifndef _SIMD_H
#define _SIMD_H

//#define SIMD

#ifdef _MSC_VER
  #ifdef SIMD
  #undef SIMD //not supported on msvc yet
  #endif

#ifdef DLLEXPORT
  #define MYEXPORT __declspec(dllexport)
#else
  #define MYEXPORT __declspec(dllimport)
#endif

  //struct align
  #define MYALIGNED(n)
  
  typedef float floatf;
  typedef int   intf;
#else //gcc
  //struct field align
  #define MYALIGNED(n) __attribute__(aligned(n))
  
  #define MYEXPORT
  
  #ifdef SIMD
  typedef float floatf __attribute__ ((vector_size (16)));
  typedef int   intf   __attribute__ ((vector_size (16)));
  #else
  typedef float floatf;
  typedef int   intf;
  #endif
#endif

#endif /* _SIMD_H */
