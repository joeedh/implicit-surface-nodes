#ifndef _IMPLICIT_OBJECT_H
#define _IMPLICIT_OBJECT_H

#include "simd.h"
#include "object.h"
#include "surface.h"
#include "thread.h"

#include "BLI_kdopbvh.h"

struct StackMachine;

typedef struct SceneMesh {
	struct SceneMesh *next, *prev;
	char name[32];
	int flag, users;

	int tottri, totvert, totuv;
	float *verts, *uvs;
	int *tris, *triuvs;

	BVHTree *bvh;
} SceneMesh;

typedef enum {
	FIELD_SUBTRACT,
	FIELD_ADD
} ObjectFieldMode;

typedef struct SceneObject {
	struct SceneObject *next, *prev;
	char name[32];
	int flag, users;

	SceneMesh *data;

	//rotations are in euler
	float loc[3], rot[3], scale[3];
	float matrix[4][4];

	struct StackMachine *machine;
	struct SMOpCode *opcodes;
	int opcode_len;

	struct StackMachine *threadmachines[MAXTHREAD];

	ObjectFieldMode mode;
} SceneObject;

struct SMOpCode; //see surface.h

MYEXPORT void so_init_threadmachines(SceneObject *ob);

MYEXPORT SceneMesh *so_create_mesh(char name[32], int tottri, int totvert, int totuv, float *verts, int *tris, 
	                     int *triuvs, float *uvs);
MYEXPORT SceneObject *so_create_object(char name[32], SceneMesh *mesh, float *co, float *rot, float *scale, float *matrix, int mode);
MYEXPORT void so_assign_opcodes(SceneObject *ob, struct SMOpCode *opcodes, int opcode_len); //copies opcodes
MYEXPORT StackMachine *so_get_stackmachine(SceneObject *ob);
MYEXPORT float so_signed_distance(SceneObject *ob, float *co);

MYEXPORT void so_free_mesh(SceneMesh *me);
MYEXPORT void so_free_object(SceneObject *ob);

#endif /* _IMPLICIT_OBJECT_H */
