#ifndef _SPATIAL_H
#define _SPATIAL_H

#include "BLI_utildefines.h"
#include "object.h"
#include "simd.h"
#include "surface.h"

typedef struct SceneRef {
	SceneObject *ob;
	int flag;
	int sign;
} SceneRef;

typedef struct SpatialGraph {
	int flag, size;
	SceneRef *refs;
} SpatialGraph;

MYEXPORT SpatialGraph *sg_new();
MYEXPORT void sg_free(SpatialGraph *sg);
MYEXPORT int sg_inside(SpatialGraph *sg, float *co);
MYEXPORT void sg_add_object(SpatialGraph *sh, SceneObject *ob, int sign);
MYEXPORT void sg_update(SpatialGraph *sg);
MYEXPORT float sg_sample(SpatialGraph *sg, floatf p[3], int threadnr);

//SpatialGraph->flag
enum {
	SG_UPDATE = 1
};

#endif _SPATIAL_H
