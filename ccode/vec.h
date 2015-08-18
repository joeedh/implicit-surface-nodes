
#include "Alloc.h"

static void *vec_resize(void *v, int typesize, int size) {
  MemHead *mh = v, *newmh;
  
  if (v == NULL) {
    mh = MEM_malloc(typesize*size);    mh--;
    mh->used = 0;
    
    return mh+1;
  } else {
    mh--;
  }
  
  //printf("vec resize...used1: %d\n", mh->used);
  
  newmh = MEM_malloc(typesize*size);

  newmh--;
  memcpy(newmh+1, mh+1, mh->size);
  
  newmh->used = mh->used;
  //printf("used: %d %d", newmh->used, mh->used);
  
  MEM_free(mh+1);
  
  //printf("    used1: %d\n", newmh->used);
  
  return newmh+1;
}

static void *vec_growone(void *v, int typesize) {
  MemHead *mh = v;
  
  if (v == NULL) {
    mh = MEM_malloc(typesize*32);
    mh--;
    mh->used = 0;
  } else {
    mh--;
  }
  
  if (mh->used*typesize >= mh->size) {
    mh = vec_resize(mh+1, typesize, (mh->used|5)*2);
    mh--;
  }
  
  //printf("mh->used: %d\n", mh->used);
  mh->used++;
  
  return mh+1;
}

#define V_COUNT(v)          (v) == NULL ? 0 : ((((MemHead*)v)-1)->used)
#define V_APPEND(v, item) (((v) = vec_growone(v, sizeof(*(v)))), (v)[V_COUNT((v))-1] = item, (v))
#define V_FREE(v)           (v  ? _MEM_free(v, __FILE__, __LINE__), v)
#define V_RESIZE(v, sz)     (v  = vec_resize(v, sizeof(*v), sz), v)
