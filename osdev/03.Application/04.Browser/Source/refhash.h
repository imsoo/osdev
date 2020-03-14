#include "OSLibrary.h"

struct HashTbl;
typedef struct HashTbl HashTable;


HashTable *ht_create();

void *ht_get(HashTable *tbl, const char *name);

void *ht_retain(HashTable *tbl, const char *name, void *value);

void *ht_put(HashTable *tbl, const char *name, void *value);

const char *ht_next(HashTable *tbl, const char *key);
