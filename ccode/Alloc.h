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

static void *_MEM_malloc(int size, char *file, int line) {
  MemHead *mh;
  
  //align to 8-byte boundary
  size += 16 - (size & 15);
  
  int size2 = size + sizeof(MemHead);
  
  
  mh = malloc(size2+8);
  
  mh->size = size;
  mh->used = 0;
  mh->file = file;
  mh->line = line;
  mh->tag = VALID_TAG;
  
  return mh+1;
}

static void *_MEM_calloc(int size, char *file, int line) {
  void *ret = _MEM_malloc(size, file, line);
  memset(ret, 0, size);
  
  return ret;
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

#define MEM_malloc(size) _MEM_malloc(size, __FILE__, __LINE__)
#define MEM_calloc(size) _MEM_calloc(size, __FILE__, __LINE__)
#define MEM_free(mem) _MEM_free(mem, __FILE__, __LINE__)

#endif /* _ALLOC_H */
