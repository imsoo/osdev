/*
https://limpet.net/mbrubeck/2014/08/11/toy-layout-engine-2.html

You might want to look at
https://www.html5rocks.com/en/tutorials/internals/howbrowserswork/#HTML_Parser
for more details on how a more authentic HTML parser might work.
*/
#include "refcnt.h"
#include "refhash.h"
#include "refalist.h"
#include "dom.h"
#include "stream.h"
#include "html.h"

#if 0
#define PRINTF(s) do{printf s;fflush(stdout);}while(0)
#else
#define PRINTF(s) 
#endif
//#define TRACE(s)  do{printf("%d: %s\n", __LINE__, s);fflush(stdout);}while(0);

static char* self_closed_tag[] = { "link", "img", };

static int consume_char(Stream *strm, char c) {
  int r = strm_getc(strm);
  if (!r) return 0;
  strm_next(strm);
  return r == c;
}

int consume_space(Stream *strm) {
  for (;;) {
    int r = strm_getc(strm);
    if (!r) return 0;
    if (!isspace(r)) {
      return 1;
    }
    strm_next(strm);
  }
}

char *parse_tag_name(Stream *strm) {
  size_t ba = 32, bn = 0;
  char *buffer = malloc(ba);
  int c = strm_getc(strm);
  while (c && isalnum(c)) {
    strm_next(strm);
    assert(bn < ba);
    buffer[bn++] = c;
    if (bn == ba) {
      ba <<= 1;
      buffer = realloc(buffer, ba);
    }
    c = strm_getc(strm);
  }
  assert(bn < ba);
  buffer[bn] = '\0';
  return buffer;
}
char *parse_attr_key(Stream *strm) {
  return parse_tag_name(strm);
}
char *parse_attr_value(Stream *strm) {
  int open = strm_getc(strm);
  if (open != '"')
    return NULL;

  size_t ba = 32, bn = 0;
  char *buffer = rc_alloc(ba);

  int c = strm_next(strm);
  while (c && c != open && c != '\n') {
    strm_next(strm);
    assert(bn < ba);
    buffer[bn++] = c;
    if (bn == ba) {
      ba <<= 1;
      buffer = rc_realloc(buffer, ba);
    }
    c = strm_getc(strm);
  }
  if (c == open)
    strm_next(strm);
  else {
    free(buffer);
    return NULL;
  }
  assert(bn < ba);
  buffer[bn] = '\0';
  return buffer;
}

HashTable *parse_attrs(Stream *strm) {
  HashTable *attrs = ht_create();
  for (;;) {
    consume_space(strm);
    int c = strm_getc(strm);
    if (!c || c == '>') {
      break;
    }
    char *key = parse_tag_name(strm);
    if (!key) goto error;
    if (!consume_char(strm, '=')) {
      free(key);
      fprintf(stderr, "error:%d: expected '='\n", strm->line);
      goto error;
      return NULL;
    }
    char *val = parse_attr_value(strm);
    if (!val) {
      fprintf(stderr, "error:%d: bad attribute '%s'\n", strm->line, key);
      free(key);
      goto error;
    }

    PRINTF(("ATTR: [%s] = [%s]\n", key, val));
    ht_put(attrs, key, val);
    free(key);
  }
  return attrs;
error:
  rc_release(attrs);
  return NULL;
}

Node *parse_node(Stream *strm);

ArrayList *parse_nodes(Stream *strm) {
  ArrayList *al = al_create();
  for (;;) {
    if (!consume_space(strm)) break;
    int c = strm_getc(strm);
    if (!c) break;
    if (c == '<') {
      c = strm_next(strm);
      strm_ungetc(strm, '<');
      if (c == '/') {
        break;
      }
    }
    Node *np = parse_node(strm);
    if (!np) {
      rc_release(al);
      return NULL;
    }
    al_add(al, np);
  }
  PRINTF(("NODES: %d\n", al_size(al)));
  return al;
}

Node *parse_element(Stream *strm) {
  int i;
  char *tag_name = NULL;
  HashTable *attrs = NULL;
  ArrayList *children = NULL;

  int c = strm_getc(strm);
  assert(c == '<'); (void)c;
  strm_next(strm);
  tag_name = parse_tag_name(strm);

  if (!tag_name) goto error;
  attrs = parse_attrs(strm);
  if (!attrs) goto error;
  assert(strm_getc(strm) == '>');
  c = strm_next(strm);

  for (i = 0; i < sizeof(self_closed_tag) / sizeof(char*); i++) {
    if (strcmp(tag_name, self_closed_tag[i]) == 0) {
      Node *n = elem_node(tag_name, attrs, children);
      return n;
    }
  }

  children = parse_nodes(strm);
  if (!children) goto error;

  if (!consume_char(strm, '<') || !consume_char(strm, '/')) {
    fprintf(stderr, "error:%d: expected </%s>\n", strm->line, tag_name);
    goto error;
  }

  char *etag_name = parse_tag_name(strm);
  if (strcmp(tag_name, etag_name)) {
    fprintf(stderr, "error:%d: closing </%s> doesn't match opening <%s>\n", strm->line, etag_name, tag_name);
    goto error;
  }
  if (!consume_char(strm, '>')) {
    fprintf(stderr, "error:%d: expected </%s>\n", strm->line, tag_name);
    goto error;
  }
  PRINTF(("ELEM NODE (%s)\n", tag_name));

  Node *n = elem_node(tag_name, attrs, children);
  return n;
error:
  if (tag_name) free(tag_name);
  if (attrs) rc_release(attrs);
  if (children) rc_release(children);
  return NULL;
}

Node *parse_text(Stream *strm) {
  size_t ba = 32, bn = 0;
  char *buffer = malloc(ba);

  int c = strm_getc(strm);
  while (c && c != '<') {
    int n = strm_next(strm);
    /* Gobble sequences of whitespace, then make sure that
    all whitespace is space (' ') characters.
    Mmm, this might not be a great idea if you want to support
    <pre> tags, etc */
    if (isspace(c) && isspace(n))
      continue;
    if (isspace(c)) c = ' ';

    assert(bn < ba);
    buffer[bn++] = c;
    if (bn == ba) {
      ba <<= 1;
      buffer = realloc(buffer, ba);
    }
    c = strm_getc(strm);
  }
  assert(bn < ba);
  buffer[bn] = '\0';
  PRINTF(("TEXT: [%s]\n", buffer));
  return text_node(buffer);
}

Node *parse_node(Stream *strm) {
  if (strm_getc(strm) == '<') {
    return parse_element(strm);
  }
  else {
    return parse_text(strm);
  }
}

Node *parse(Stream *strm) {
  ArrayList *nodes = parse_nodes(strm);
  if (!nodes) return NULL;
  if (al_size(nodes) == 1) {
    Node *n = rc_retain(al_get(nodes, 0));
    rc_release(nodes);
    return n;
  }
  else
    return elem_node("html", ht_create(), nodes);
}
