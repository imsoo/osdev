#include "refcnt.h"
#include "refalist.h"

static void al_dtor(void *p) {
	ArrayList *al = p;
	unsigned int i;
	for(i = 0; i < al->n; i++)
		rc_release(al->els[i]);
	free(al->els);
}

ArrayList *al_create() {
	ArrayList *al = rc_alloc(sizeof *al);
	al->a = 8;
	al->els = calloc(al->a, sizeof *al->els);
	al->n = 0;
	rc_set_dtor(al, al_dtor);
	return al;
}

unsigned int al_size(ArrayList *al) { 
	return al->n;
}

unsigned int al_add(ArrayList *al, void *p) {
	if(al->n == al->a) {
		al->a <<= 1;
		al->els = realloc(al->els, al->a * sizeof *al->els);
	} 
	assert(al->n < al->a);
	al->els[al->n] = p;
	return al->n++;
}

unsigned int al_retain(ArrayList *al, void *p) {
	p = rc_retain(p);
	al_add(al, p);
	return al->n++;
}

void *al_get(ArrayList *al, unsigned int i) {
	// assert(i >= 0 && i < al->n);
	return al->els[i];
}

void *al_set(ArrayList *al, unsigned int i, void *p) {
	// TODO: We might want to resize if the array is too small...
	// assert(i >= 0 && i < al->n);
	if(al->els[i]) rc_release(al->els[i]);
	al->els[i] = p;
	return p;
}

void *al_first(ArrayList *al) {
	if(al->n > 0)
		return al->els[0];
	return NULL;
}

void *al_last(ArrayList *al) {
	if(al->n > 0)
		return al->els[al->n - 1];
	return NULL;
}

void al_sort(ArrayList *al, int (*cmp)(const void*, const void*)) {	
	qsort(al->els, al->n, sizeof *al->els, cmp);
}

