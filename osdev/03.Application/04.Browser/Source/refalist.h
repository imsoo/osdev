/* ArrayList-like data structure.
 * Suitable for adding data where you don't know the maximum size beforehand,
 * so the array needs to grow dynamically.
 * Fast element lookup and iteration.
 * Not suitable for situations where items need to be removed frequently.
 */
#ifndef ALIST_H
#define ALIST_H

#include "OSLibrary.h"

typedef struct ArrayList {
    unsigned int n;     /* Number of elements in the list */
    unsigned int a;     /* Allocated number of elements */
    void **els;         /* Actual elements */
} ArrayList;

ArrayList *al_create();

unsigned int al_size(ArrayList *al);

/* Like al_add, but the list takes ownership
(so it doesn't retain the object internally) */
unsigned int al_add(ArrayList *al, void *p);

unsigned int al_retain(ArrayList *al, void *p);

void *al_get(ArrayList *al, unsigned int i);

void *al_set(ArrayList *al, unsigned int i, void *p);

void *al_first(ArrayList *al);

void *al_last(ArrayList *al);

void al_sort(ArrayList *al, int (*cmp)(const void*, const void*));

#endif
