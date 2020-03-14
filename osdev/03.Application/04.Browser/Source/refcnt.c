/*
 * Reference counter for C.
 *
 * It works by allocating a hidden header before each object
 * containing the reference count.
 * A `rc_retain()` operation increments this reference count.
 * A `rc_release()` operation decrements the reference count.
 * If the reference count reaches 0 the object is deallocated.
 *
 * The header also has a pointer to a _destructor_ function that
 * will be called (if set) when the object is deallocated so
 * that the object can release any references to other objects
 * it may hold.
 *
 * In _debug_ mode (if `NDEBUG` is not defined) the objects are
 * added and removed from a linked list as they are allocated
 * and deallocated. Each object also tracks where it was
 * allocated, retained and released using some `__FILE__` and
 * `__LINE__` trickery, and will print a report to
 * `stdout` of objects that are still allocated when the program
 * terminates in order to troubleshoot memory leaks.
 *
 * This adds a lot of overhead and is not reentrant because of
 * its reliance on a global linked list.
 *
 * See `refcnt.h` for usage instructions
 *
 * Based on this article:
 * http://www.xs-labs.com/en/archives/articles/c-reference-counting/
 */
#include "refcnt.h"

/* NDEBUG (Release mode) */
typedef unsigned int RefType;

typedef struct refobj {
    RefType refcnt;
    ref_dtor dtor;
} RefObj;

void rc_init() {
}

void *rc_alloc(size_t size) {
    void *data;

    RefObj *r = malloc((sizeof *r) + size);
    if(!r)
        return NULL;
    data = (char*)r + sizeof *r;
    r->refcnt = 1;
    r->dtor = NULL;
    return data;
}

void *rc_realloc(void *p, size_t size) {
    RefObj *r;
    if(!p)
        return NULL;
    r = (RefObj *)((char *)p - sizeof *r);
    // assert(r->refcnt == 1);
    r = realloc(r, (sizeof *r) + size);
    return (char*)r + sizeof *r;
}

char *rc_strdup(const char *s) {
    size_t len = strlen(s);
    char *n = rc_alloc(len + 1);
    if(!n)
        return NULL;
    memcpy(n, s, len + 1);
    return n;
}


void *rc_memdup(const void *p, size_t size) {
    char *n = rc_alloc(size);
    if(!n)
        return NULL;
    memcpy(n, p, size);
    return n;
}


void *rc_retain(void *p) {
    RefObj *r;
    if(!p)
        return NULL;
    r = (RefObj *)((char *)p - sizeof *r);
    r->refcnt++;
    return p;
}

void rc_release(void *p) {
    RefObj *r;
    if(!p)
        return;
    r = (RefObj *)((char *)p - sizeof *r);
    r->refcnt--;
    if(r->refcnt == 0) {
        if(r->dtor != NULL) {
            r->dtor(p);
        }
    }
}

void rc_set_dtor(void *p, ref_dtor dtor) {
    RefObj *r;
    if(!p) return;
    r = (RefObj *)((char *)p - sizeof *r);
    r->dtor = dtor;
}

void *rc_assign(void **p, void *val) {
    if(*p) {
        rc_release(*p);
    }
    *p = val;
    return val;
}
