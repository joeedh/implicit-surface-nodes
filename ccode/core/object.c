#define _CRT_SECURE_NO_WARNINGS

#include "BLI_math.h"

#include "simd.h"
#include <string.h>
#include "object.h"
#include "Alloc.h"
#include "vec.h"
#include "surface.h"
#include "thread.h"

MYEXPORT SceneMesh *so_create_mesh(char name[32], int tottri, int totvert, int totuv, float *verts, int *tris, 
	                      int *triuvs, float *uvs)
{
	SceneMesh *m = MEM_calloc(sizeof(*m));
	int i;

	if (name) {
		strncpy(m->name, name, 32);
		m->name[31] = 0;
	} else {
		strncpy(m->name, "unnamed-mesh", 32);
		m->name[31] = 0;
	}

	if (totvert == 0 && tottri == 0) {
		return m;
	}

	m->tottri = tottri;
	m->totvert = totvert;
	m->totuv = totuv;

	m->verts = MEM_malloc(sizeof(float)*totvert*3);
	m->tris = MEM_malloc(sizeof(int)*tottri*3);

	memcpy(m->verts, verts, sizeof(float)*totvert*3);
	memcpy(m->tris, tris, sizeof(int)*tottri*3);

	if (uvs && triuvs) {
		m->uvs = MEM_malloc(sizeof(float)*totuv*2);
		m->triuvs = MEM_malloc(sizeof(float)*tottri*3);

		memcpy(m->uvs, uvs, sizeof(float)*totuv*2);
		memcpy(m->triuvs, triuvs, sizeof(int)*tottri*3);
	}

	m->bvh = BLI_bvhtree_new(tottri, 0.0001f, 8, 8);

	for (i = 0; i < tottri; i++) {
		float cos[9];
		int *tri = tris + i*3;

		memcpy(cos,   verts + tri[0] * 3, sizeof(float) * 3);
		memcpy(cos+3, verts + tri[1] * 3, sizeof(float) * 3);
		memcpy(cos+6, verts + tri[2] * 3, sizeof(float) * 3);

		BLI_bvhtree_insert(m->bvh, i, cos, 3);
	}

	BLI_bvhtree_balance(m->bvh);

	return m;
}

MYEXPORT SceneObject *so_create_object(char name[32], SceneMesh *mesh, float *co, float *rot, float *scale, float *matrix, int mode) {
	SceneObject *ob = MEM_calloc(sizeof(*ob));
	
	ob->mode = (ObjectFieldMode)mode;
	ob->machine = sm_new();

	if (name) {
		strncpy(ob->name, name, 32);
		ob->name[31] = 0;
	} else {
		strncpy(ob->name, "unnamed-object", 32);
		ob->name[31] = 0;
	}

	if (mesh) {
		ob->data = mesh;
		mesh->users++;
	}

	if (co)
		memcpy(ob->loc, co, sizeof(float)*3);
	if (rot)
		memcpy(ob->rot, rot, sizeof(float)*3);
	if (matrix) {
		memcpy(ob->matrix, matrix, sizeof(float)*16);
		invert_m4(ob->matrix);
	} else { //make identity matrix
		ob->matrix[0][0] = ob->matrix[1][1] = ob->matrix[2][2] = ob->matrix[3][3] = 1.0f;
	}

	if (scale) {
		memcpy(ob->scale, scale, sizeof(float)*3);
	} else {
		ob->scale[0] = ob->scale[1] = ob->scale[2] = 1.0f;
	}

	return ob;
}

MYEXPORT void so_assign_opcodes(SceneObject *ob, struct SMOpCode *opcodes, int opcode_len) {
	ob->opcodes = MEM_calloc(sizeof(SMOpCode)*opcode_len);
	memcpy(ob->opcodes, opcodes, sizeof(SMOpCode)*opcode_len);

	sm_add_opcodes(ob->machine, opcodes, opcode_len);
	ob->opcode_len = opcode_len;
}

MYEXPORT StackMachine *so_get_stackmachine(SceneObject *ob) {
	return ob->machine;
}

MYEXPORT void so_free_mesh(SceneMesh *me) {
	MEM_free(me->verts);
	MEM_free(me->tris);
	
	if (me->uvs)
		MEM_free(me->uvs);
	if (me->triuvs)
		MEM_free(me->triuvs);

	BLI_bvhtree_free(me->bvh);

	MEM_free(me);
}

typedef struct callbackdata {
	int flag;
	SceneMesh *me;
	SceneObject *ob;
	float dist;
} callbackdata;

static nearest_callback(void *userdata, int index, const float co[3], BVHTreeNearest *nearest) {
	callbackdata *data = userdata;
	float p[3], vec[3], n1[3], n2[3], n[3];

	int *tri = data->me->tris + index * 3;

	float *v1 = data->me->verts + tri[0] * 3;
	float *v2 = data->me->verts + tri[1] * 3;
	float *v3 = data->me->verts + tri[2] * 3;
	float sign, dist;

	closest_on_tri_to_point_v3(p, co, v1, v2, v3);

	sub_v3_v3v3(vec, co, v1);

	sub_v3_v3v3(n1, v2, v1);
	sub_v3_v3v3(n2, v3, v1);

	cross_v3_v3v3(n, n1, n2);

	sign = dot_v3v3(n, vec) < 0.0f ? -1.0f : 1.0f;

	dist = len_v3v3(p, co)*sign;

	if (dist*dist < data->dist*data->dist) {// nearest->dist_sq) {
		copy_v3_v3(nearest->co, p);
		copy_v3_v3(nearest->no, vec);

		nearest->dist_sq = dist*dist;
		nearest->index = index;
		data->dist = dist;
	}
}

static ray_callback(void *userdata, int index, const BVHTreeRay *ray, BVHTreeRayHit *hit) {

}

MYEXPORT float so_signed_distance(SceneObject *ob, float *co) {
//	int BLI_bvhtree_find_nearest(BVHTree *tree, const float co[3], BVHTreeNearest *nearest,
//		BVHTree_NearestPointCallback callback, void *userdata);
	BVHTreeNearest nearest;
	callbackdata userdata;
	float co2[3];

	userdata.flag = 0;
	userdata.me = ob->data;
	userdata.ob = ob;
	userdata.dist = FLT_MAX;

	memset(&nearest, 0, sizeof(nearest));
	nearest.dist_sq = FLT_MAX;
	nearest.index = -1;

	copy_v3_v3(co2, co);
	mul_v3_m4v3(co2, ob->matrix, co);

	BLI_bvhtree_find_nearest(ob->data->bvh, co2, &nearest, nearest_callback, &userdata);

	return userdata.dist;
}

MYEXPORT void so_init_threadmachines(SceneObject *ob) {
	int i;

	if (!ob->machine)
		return;

	for (i = 0; i < MAXTHREAD; i++) {
		if (ob->threadmachines[i])
			sm_free(ob->threadmachines[i]);
		
		ob->threadmachines[i] = sm_copy(ob->machine);
		ob->threadmachines[i]->threadnr = i;
	}
}

MYEXPORT void so_free_object(SceneObject *ob) {
	int i;

	sm_free(ob->machine);

	for (i = 0; i < MAXTHREAD; i++) {
		if (ob->threadmachines[i]) {
			sm_free(ob->threadmachines[i]);
		}

		ob->threadmachines[i] = NULL;
	}

	if (ob->data) {
		ob->data->users--;
	}

	if (ob->opcodes) {
		MEM_free(ob->opcodes);
	}

	MEM_free(ob);
}

MYEXPORT void so_release_mesh(SceneMesh *me) {
	if (me->users == 0) {
		so_free_mesh(me);
	}
}
