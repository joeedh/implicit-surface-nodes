
#define KEY_UNUSED  0x7FFFFFFF

//hash table inserts unique keys

typedef struct HashEntry {
  int key;
  float vec[3];
  unsigned int value;
} HashEntry;

typedef struct HashTable {
  int cursize;
  int size, used;
  int vert_idgen;
  
  HashEntry *table;
} HashTable;

#define HASHKEY(x, y, z) ((int)((x)*1024.0f*1024.0f + (y)*1024.0f + (z)))
#define ABS(n) ((n) < 0 ? 0-(n) : (n))

HashTable *ht_new();
void ht_resize(HashTable *ht, int newsize);

//build indices in single vertex array
//returns index of vertex at coordinates vec
int ht_ensurevert(HashTable *ht, float vec[3]);
int ht_insert(HashTable *ht, float vec[3], unsigned int value, int *value_out);
void ht_free(HashTable *ht);