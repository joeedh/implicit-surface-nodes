#ifndef _ALLOC_H
#define _ALLOC_H

#include "string.h"
#include "stdio.h"

#define FREE_TAG 254363456
#define VALID_TAG 459898344

//this structure is shared by vec.h
typedef struct MemHead {
  int size, used, tag; //used is exclusively for used by vec.h
  int line;
  char *file;
} MemHead;

typedef struct MemTail {
  int size, tag; //used is exclusively for used by vec.h
  int line;
  char *file;
} MemTail;

static void *_MEM_malloc(int size, char *file, int line) {
  MemHead *mh;
  MemTail *mt;
  
  //align to 8-byte boundary
  size += 16 - (size & 15);
  
  int size2 = size + sizeof(MemHead) + sizeof(MemTail);
  
  
  mh = malloc(size2+8);
  
  mh->size = size;
  mh->used = 0;
  mh->file = file;
  mh->line = line;
  mh->tag = VALID_TAG;
  
  mt = (MemTail*)(((char*)(mh+1)) + size);
  mt->size = size;
  mt->tag = VALID_TAG;
  mt->file = file;
  mt->line = line;
  
  return mh+1;
}

static void *_MEM_calloc(int size, char *file, int line) {
  void *ret = _MEM_malloc(size, file, line);
  memset(ret, 0, size);
  
  return ret;
}


static void _MEM_check(void *mem, char *file, int line) {
  MemHead *mh = mem;
  MemTail *mt;
  
  if (!mem) return;

  mh--;
  
  if (mh->tag == FREE_TAG) {
    fprintf(stderr, "Warning: pointer to freed block detected at:\t%s:%i\n", file, line);
    return;
  } else if (mh->tag != VALID_TAG || mh->size <= 0) {
    fprintf(stderr, "Warning: corrupted memory detected at:\t%s:%i\n", file, line);
    fprintf(stderr, "  %d %d %d\n", mh->tag, mh->size, mh->used);
    return;
  }
  return;
  mt = (MemTail*)(((char*)(mh+1)) + mh->size);
  
  if (mt->tag != VALID_TAG) {
    fprintf(stderr, "Warning: corrupted memory tail detected at:\t%s:%i\n", file, line);
    fprintf(stderr, "  %d %d %d\n", mt->tag, mh->size, mh->used);
    return;
  }
}

static void _MEM_free(void *mem, char *file, int line) {
  MemHead *mh = mem;
  
  if (!mem) return;

  mh--;
  
  if (mh->tag == FREE_TAG) {
    fprintf(stderr, "Warning: double free detected at:\t%s:%i\n", file, line);
    return;
  } else if (mh->tag != VALID_TAG) {
    fprintf(stderr, "Warning: corrupted memory detected during free:\t%s:%i\n", file, line);
    fprintf(stderr, "  %d %d %d\n", mh->tag, mh->size, mh->used);
    return;
  }
  
  mh->tag = FREE_TAG;
  free(mh);
}

static void *_MEM_copyalloc(void *mem, int size, char *file, int line) {
  void *ret = _MEM_malloc(size, file, line);
  
  memcpy(ret, mem, size);
  
  return ret;
}

#define MEM_malloc(size) _MEM_malloc(size, __FILE__, __LINE__)
#define MEM_calloc(size) _MEM_calloc(size, __FILE__, __LINE__)
#define MEM_free(mem) _MEM_free(mem, __FILE__, __LINE__)
#define MEM_copyalloc(mem, size) _MEM_copyalloc(mem, size, __FILE__, __LINE__)
#define MEM_check(mem) _MEM_check(mem, __FILE__, __LINE__)

#endif /* _ALLOC_H */
