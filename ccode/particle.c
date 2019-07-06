#include "particle.h"
#include "Alloc.h"
#include "vec.h"
#include "bvh/BLI_kdopbvh.h"
#include "simd.h"

MYEXPORT ParticleSystem *PSys_New() {
    ParticleSystem *ps = MEM_calloc(sizeof(*ps));
    
    return ps;
}

MYEXPORT void PSys_Free(ParticleSystem *ps) {
    V_FREE(ps->ps);
	BLI_bvhtree_free(ps->bvh);
    MEM_free(ps);
}

MYEXPORT ParticleRef PSys_AddParticle(ParticleSystem *ps, Particle *ptemplate) {
	V_APPEND(ps->ps, *ptemplate);

	return (ParticleRef)V_COUNT(ps->ps) - 1;
}

MYEXPORT Particle *PSys_DerefParticle(ParticleSystem *ps, ParticleRef ref) {
	return ps->ps + (int)ref;
}

