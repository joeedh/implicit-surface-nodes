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

MYEXPORT float sg_sample(SpatialGraph *sg, float p[3]) {
	int i, ilen = V_COUNT(sg->refs), first=1;
	floatf ret;
	SceneRef *ref = sg->refs;

	for (i = 0; i < ilen; i++, ref++) {
		SceneObject *ob = ref->ob;
		float field;

		if (ob->machine->totcode == 0) {
			continue;
		}

		//get basic distfield
		field = so_signed_distance(ob, p);

		//load globals
		sm_set_global(ob->machine, 0, p[0]); //_px
		sm_set_global(ob->machine, 1, p[1]); //_py
		sm_set_global(ob->machine, 2, p[2]); //_pz
		sm_set_global(ob->machine, 3, field); //_field

		floatf f = sm_run_inline(ob->machine, ob->machine->codes, ob->machine->totcode);
		
		if (ob->mode == FIELD_SUBTRACT) {
			f = -f;
		}

		if (first) {
			ret = f;
			first = 0;
		}

		#ifdef SIMD
				floatf f2 = {
					MAX(f[0], ret[0]),
					MAX(f[1], ret[1]),
					MAX(f[2], ret[2]),
					MAX(f[3], ret[3])
				};//*/

				ret = f2;
		#else
				ret = MAX(f, ret);
		#endif
	}

#ifdef SIMD
	return first ? 0 : ret[0];
#else
	return first ? FLT_MAX : ret;
#endif
}
