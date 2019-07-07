#ifndef _IMPLICIT_EXPORT_H
#define _IMPLICIT_EXPORT_H

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
#endif

#endif /* _IMPLICIT_EXPORT_H */
