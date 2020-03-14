#include "refcnt.h"
#include "refhash.h"

#define HASH_SIZE 17 /* must be prime */

struct elem {
	char *name;
	void *value;
	struct elem *next;
};

struct HashTbl { 
	struct elem** buckets;
	size_t size;
};

static void free_element(struct elem *e) {
	if(!e) return;
	free_element(e->next);
	rc_release(e->value);
	free(e->name);
	free(e);
}

static void hash_tbl_dtor(void *p) {
	HashTable *tbl = p;
	int i;
	for(i = 0; i < HASH_SIZE; i++)
		free_element(tbl->buckets[i]);
	free(tbl->buckets);
}

HashTable *ht_create() {
	HashTable *tbl = rc_alloc(sizeof *tbl);
	tbl->size = HASH_SIZE;
	tbl->buckets = calloc(tbl->size, sizeof *tbl->buckets);
	rc_set_dtor(tbl, hash_tbl_dtor);
	return tbl;
}

static unsigned int hash(const char *s) {
	unsigned int i = 0x5555;
	for(;s[0];s++)
		i = i << 3 ^ (s[0] | (s[0] << 8));
	return i;
}

static struct elem *search(HashTable *tbl, const char *name) {
	struct elem* e;
	unsigned int h = hash(name) % tbl->size;
	if(tbl->buckets[h])
		for(e = tbl->buckets[h]; e; e = e->next)
			if(!strcmp(e->name, name))
				return e;
	return NULL;
}

void *ht_get(HashTable *tbl, const char *name) {
	struct elem* e = search(tbl, name);
	if(e) return e->value;
	return NULL;
}
void *ht_retain(HashTable *tbl, const char *name, void *value) {
	value = rc_retain(value);
	return ht_put(tbl, name, value);
}

void *ht_put(HashTable *tbl, const char *name, void *value) {
	struct elem* e;
	unsigned int h = hash(name) % tbl->size;
	if(tbl->buckets[h]) {
		for(e = tbl->buckets[h]; e; e = e->next)
			if(!strcmp(e->name, name)) {
				rc_release(e->value);
				e->value = value;
				return value;
			}
	}
	e = malloc(sizeof *e);
	e->name = strdup(name);
	e->value = value;
	e->next = tbl->buckets[h];
	tbl->buckets[h] = e;
	return value;
}

/* Finds the next element in the table given a specific key */
const char *ht_next(HashTable *tbl, const char *key) {
	int f;
	if (key == NULL) {
		for (f = 0; f < tbl->size && !tbl->buckets[f]; f++);
		if (f >= tbl->size)
			return NULL;
		assert (tbl->buckets[f]);
		return tbl->buckets[f]->name;
	}
	struct elem* e = search(tbl, key);
	if (!e)
		return NULL;
	if (e->next) {
		return e->next->name;
	} else {
		f = (hash(key) % tbl->size) + 1;
		while (f < tbl->size && !tbl->buckets[f])
			f++;
		if (f >= tbl->size)
			return NULL;
		assert (tbl->buckets[f]);
		return tbl->buckets[f]->name;
	}
}
