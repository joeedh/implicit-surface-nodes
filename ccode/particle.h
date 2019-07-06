#ifndef _PARTICLE_H
#define _PARTICLE_H

#include "simd.h"

typedef int ParticleRef;

typedef struct Particle {
    int id, flags;
    
    double co[3], vel[3], oldco[3], oldvel[3];
    double no[3], mass, f1, f2, f3, f4;
} Particle;

struct BVHTree;
typedef struct ParticleSystem {
    Particle *ps;
    struct BVHTree *bvh;
} ParticleSystem;

MYEXPORT ParticleSystem *PSys_New();
MYEXPORT void PSys_Free(ParticleSystem *ps);
MYEXPORT ParticleRef PSys_AddParticle(ParticleSystem *ps, Particle *ptemplate);
MYEXPORT Particle *PSys_DerefParticle(ParticleSystem *ps, ParticleRef ref);

#endif /* _PARTICLE_H */
