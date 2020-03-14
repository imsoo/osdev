#ifndef STREAM_H
#define STREAM_H

#include "OSLibrary.h"

typedef struct {
    int (*getc)(void *);
    int (*next)(void *);
    void (*done)(void *);
    void *data;
    int line;
    int pb[10];
    int npb;
} Stream;

Stream *file_stream(const char *filename);
Stream *string_stream(const char *string);

int strm_getc(Stream *r);
void strm_ungetc(Stream *r, int c);

int strm_next(Stream *r);

void strm_done(Stream *r);
#endif