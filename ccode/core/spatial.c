#include "spatial.h"
#include "Alloc.h"
#include "vec.h"
#include "inline_stack_loop.h"
#include "util.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <float.h>

void BLI_assert(int booleon) {

}

MYEXPORT SpatialGraph *sg_new() {
	SpatialGraph *sg = MEM_calloc(sizeof(*sg));

	sg->flag |= SG_UPDATE;

	return sg;
}

MYEXPORT void sg_free(SpatialGraph *sg) {
	int i, ilen = V_COUNT(sg->refs);

	for (i=0; i<ilen; i++) {
		sg->refs[i].ob->users--;
	}

	V_FREE(sg->refs);
}

MYEXPORT void sg_add_object(SpatialGraph *sg, SceneObject *ob, int sign) {
	SceneRef ref;

	memset(&ref, 0, sizeof(ref));

	ref.ob = ob;
	ref.sign = sign;
	ref.flag = 0;

	ob->users++;

	V_APPEND(sg->refs, ref);

	so_init_threadmachines(ob);
}

MYEXPORT void sg_update(SpatialGraph *sg) {
	sg->flag |= SG_UPDATE;
}

static void _sg_update(SpatialGraph *sg, int clear_flag) {
	if (clear_flag) {
		sg->flag &= ~SG_UPDATE;
	}


}

MYEXPORT int sg_inside(SpatialGraph *sg, float *co) {
	if (sg->flag & SG_UPDATE) {
		_sg_update(sg, 1);
	}
}

MYEXPORT float sg_distance(SpatialGraph *sg) {
	if (sg->flag & SG_UPDATE) {
		_sg_update(sg, 1);
	}
}

MYEXPORT float sg_sample(SpatialGraph *sg, floatf p[3], int threadnr) {
	int i, pass, ilen = V_COUNT(sg->refs), first=1;
	floatf ret;
	SceneRef *ref = sg->refs;
	
	//do unions first, then subtracts

	for (pass = 0; pass < 2; pass++) {
		for (i = 0, ref=sg->refs; i < ilen; i++, ref++) {
			SceneObject *ob = ref->ob;
			StackMachine *sm = ob->threadmachines[threadnr];
			float field;

			if (!!(ob->mode == FIELD_SUBTRACT) != !!pass) {
				continue;
			}
			//if ((ob->mode == FIELD_SUBTRACT) ^ !pass) {
			//	continue;
			//}

			if (ob->machine->totcode == 0) {
				continue;
			}

			//get basic distfield
			field = so_signed_distance(ob, p);

			//load globals
			sm_set_global(sm, 0, p[0]); //_px
			sm_set_global(sm, 1, p[1]); //_py
			sm_set_global(sm, 2, p[2]); //_pz
			sm_set_global(sm, 3, field); //_field

			floatf f = sm_run_inline(sm, sm->codes, sm->totcode);

			if (first) {
				ret = f; //ob->mode == FIELD_SUBTRACT ? MAX(-f, 0) : f;
				first = 0;

				continue;
			}

			if (ob->mode == FIELD_SUBTRACT) {
#ifdef SIMD
				floatf f2 = {
					MAX(-f[0], ret[0]),
					MAX(-f[1], ret[1]),
					MAX(-f[2], ret[2]),
					MAX(-f[3], ret[3])
				};//*/

				ret = f2;
#else
				ret = MAX(ret, -f);
#endif
			}
			else {
#ifdef SIMD
				floatf f2 = {
					MIN(f[0], ret[0]),
					MIN(f[1], ret[1]),
					MIN(f[2], ret[2]),
					MIN(f[3], ret[3])
				};//*/

				ret = f2;
#else
				ret = MIN(f, ret);
#endif
			}
		}
	}

#ifdef SIMD
	return first ? 0 : ret[0];
#else
	return first ? FLT_MAX : ret;
#endif
}
