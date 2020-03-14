#include "stream.h"

typedef struct {
    FILE *f;
    char buffer[64];
    int p;
} FileStreamData;

static int fr_getc(void *d) {
    FileStreamData *frd = d;
    //printf("fr_getc: '%c' (%d)\n", frd->buffer[frd->p], frd->buffer[frd->p]); fflush(stdout);
    return frd->buffer[frd->p];
}
static int fr_next(void *d) {
    FileStreamData *frd = d;
    if(!frd->buffer[frd->p]) {
        // printf("fr_next: eof\n");
        return 0;
    } 
    frd->p++;
    // printf("fr_next: %d\n", frd->p);
    if(!frd->buffer[frd->p]) {
        /*if(feof(frd->f) || !fgets(frd->buffer, sizeof(frd->buffer), frd->f)) {
            printf("fr_next: eof @ fgets\n");
            return 0;
        }*/
        if (feof(frd->f)) {
          // printf("fr_next: eof @\n");
          return 0;
        }

        if (fgets(frd->buffer, sizeof(frd->buffer), frd->f) == NULL) {
          // printf("fr_next: @ fgets\n");
          return 0;
        }
        // printf("fgets: buffer:'%s'\n",frd->buffer);
        frd->p = 0;
    }
    return frd->buffer[frd->p];
}
static void fr_done(void *d) {
    FileStreamData *frd = d;
    fclose(frd->f);
    free(frd);
}

Stream *file_stream(const char *filename) {
    Stream *r = malloc(sizeof *r);
    FileStreamData *frd = malloc(sizeof *frd);
    frd->f = fopen(filename, "r");
    if(!frd->f) return NULL;
    fgets(frd->buffer, sizeof(frd->buffer), frd->f);
    /*printf("fgets: buffer:'%s'\n",frd->buffer);*/
    frd->p = 0;
    r->data = frd;
    r->getc = fr_getc;
    r->next = fr_next;
    r->done = fr_done;
    r->line = 0;
    r->npb = 0;
    return r;
}

typedef struct {
    char *string;
    int p;
} StringStreamData;

static int ss_getc(void *d) {
    StringStreamData *sd = d;
    //printf("%s\n",sd->string);exit(0);
    return sd->string[sd->p];
}
static int ss_next(void *d) {
    StringStreamData *sd = d;
    if(!sd->string[sd->p]) {
        return 0;
    } 
    return sd->string[++sd->p];
}
static void ss_done(void *d) {
    StringStreamData *sd = d;
    free(sd->string);
    free(sd);
}

Stream *string_stream(const char *string) {
    Stream *r = malloc(sizeof *r);
    StringStreamData *sd = malloc(sizeof *sd);
    sd->string = strdup(string);
    sd->p = 0;
    r->data = sd;
    r->getc = ss_getc;
    r->next = ss_next;
    r->done = ss_done;
    r->line = 0;
    r->npb = 0;
    return r;
}

int strm_getc(Stream *r) {
    if(r->npb > 0) {
        //printf("getc with npb (%d) > 0: '%c'\n", r->npb, r->pb[r->npb - 1]);
        return r->pb[r->npb - 1];
    }
    return r->getc(r->data);
}
int strm_next(Stream *r) {
    if(r->npb > 0) {
        return r->pb[--r->npb];
    }
    int c = r->next(r->data);
    if(c == '\n')
        r->line++;
    return c;
}
void strm_ungetc(Stream *r, int c) {
    /* FIXME: Bounds checking */
    //printf("ungetc: '%c'\n", c);
    r->pb[r->npb++] = c;
}
void strm_done(Stream *r) {
    if(r->done)
        r->done(r->data);
    free(r);
}

