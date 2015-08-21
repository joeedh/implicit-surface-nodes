#ifndef _MESH_H
#define _MESH_H

#include "simd.h"

#define VECLOAD(a, b) ((a)[0] = (b)[0], (a)[1] = (b)[1], (a)[2] = (b)[2])
#define VECSUB(r, a, b) (r[0] = a[0]-b[0], r[1] = a[1]-b[1], r[2] = a[2]-b[2])
#define VECADD(r, a, b) (r[0] = a[0]+b[0], r[1] = a[1]+b[1], r[2] = a[2]+b[2])
#define VECMUL(r, a, b) (r[0] = a[0]*b[0], r[1] = a[1]*b[1], r[2] = a[2]*b[2])
#define VECMULF(v, f) ((v)[0] *= (f), (v)[1] *= (f), (v)[2] *= (f))
#define DOT(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])

#include "surface.h"

void polygonize(floatf (*sample)(floatf, floatf, floatf,int), float **vertout, int *totvert, 
               int **triout, int *tottri, float min[3], float max[3], int ocdepth, 
               float matrix[4][4], int thread);

void threaded_polygonize(floatf (*sample)(floatf, floatf, floatf,int), float **vertout, int *totvert, int **triout, int *tottri,
                float min[3], float max[3], int ocdepth, float matrix[4][4], int thread);

#endif /* _MESH_H */
