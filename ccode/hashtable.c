#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Alloc.h"

#include "mesh.h"
#include "hashtable.h"

int hashsizes[] = {
  /*2,*/ 5, 11, 19, 37, 67, 127, 223, 383, 653, 1117, 1901, 3251, 
  5527, 9397, 15991, 27191, 46229, 78593, 133631, 227177, 38619,
  656587, 1116209, 1897561, 3225883, 5484019, 9322861, 15848867,
  26943089, 45803279, 77865577, 132371489, 225031553
};

HashTable *ht_new() {
  HashTable *ht = MEM_calloc(sizeof(HashTable));
  int i;
  
  ht->cursize = 0;
  ht->size = hashsizes[ht->cursize];
  ht->table = MEM_malloc(sizeof(HashEntry)*ht->size);
  
  for (i=0; i<ht->size; i++) {
    ht->table[i].key = KEY_UNUSED;
  }
  
  return ht;
}

void ht_resize(HashTable *ht, int newsize) {
  HashEntry *nt, *old, *en;
  int i, size, oldsize=ht->size;
  int existing;
  
  //printf("  start resizing of hash table (used: %d)\n", ht->used);
  
  while (hashsizes[++ht->cursize] < newsize);
  size = hashsizes[ht->cursize];
  
  //printf("newsize: %d, oldsize: %d\n", size, oldsize);
  
  nt = MEM_malloc(sizeof(HashEntry)*size);
  
  for (i=0; i<size; i++) {
    nt[i].key = KEY_UNUSED;
  }
  
  old = ht->table;
  
  //clear hashtable
  ht->table = nt;
  ht->size = size;
  ht->used = 0;
  
  //insert old data
  
  en = old;
  for (i=0; i<oldsize; i++, en++) {
    if (en->key == KEY_UNUSED) {
      continue;
    }
    
    //printf("resizing insert\n");
    ht_insert(ht, en->vec, en->value, &existing);
  }
  
  //printf("finished resizing\n");
  MEM_free(old);
  //printf("  end resizing of hash table\n");
}

void ht_free(HashTable *ht) {
  MEM_free(ht->table);
  MEM_free(ht);
}

//build indices in single vertex array
int ht_ensurevert(HashTable *ht, float vec[3]) {
  int existing;
  
  if (ht_insert(ht, vec, ht->vert_idgen, &existing)) {
    return existing;
  }
  
  return ht->vert_idgen++;
}

int ht_insert(HashTable *ht, float vec[3], unsigned int value, int *value_out) {
  HashEntry *en=NULL;
  int hash=0;
  int key = HASHKEY(vec[0], vec[1], vec[2]);
  int off = 0, size, basehash;
  
  //key = key == 0 ? 1 : key;
  
  //printf("key: %d : %.3f %.3f %.3f\n", key, vec[0], vec[1], vec[2]);
  
  if (ht->used > ht->size/3) {
    //printf("resizing hashtable. . .\n");
    ht_resize(ht, ht->size+1);
  }
  
  size = ht->size;
  basehash = ABS(key);
  while (1) {
    hash = (basehash + off) % size;
    
    en = ht->table + hash;
    
    if (en->key != KEY_UNUSED && en->key == key) { //see if we already exist in table
      float dx = en->vec[0]-vec[0], dy = en->vec[1]-vec[1], dz = en->vec[2]-vec[2];
      
      //printf("len: %.3f key: %d used: %d, hash : %d off: %d size: %d vert_idgen: %d\n", sqrt(dx*dx + dy*dy + dz*dz), key, ht->used, hash, off, size, ht->vert_idgen);
      
      if (dx*dx + dy*dy + dz*dz < 1e-07f) {
        *value_out = en->value;
        return 1; //we're already in the hash
      } else {
        off += (hash % 5) + 1;
        continue;
      }
    }
    
    if (en->key != KEY_UNUSED) {
      off += (hash % 5) + 1;
    } else {
      break;
    }
  }
  
  en->key = key;
  en->value = value;
  en->vec[0] = vec[0];
  en->vec[1] = vec[1];
  en->vec[2] = vec[2];
  
  //printf("ht->used: %d\n", ht->used);
  
  ht->used++;
  
  return 0;
}
