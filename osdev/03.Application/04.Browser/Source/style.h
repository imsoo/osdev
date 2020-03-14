#ifndef STYLE_H
#define STYLE_H

#include "OSLibrary.h"

typedef struct {
	Node *node;
	HashTable *specified_values;
	ArrayList *children;
} StyledNode;

typedef enum { Inline, Block, None, } Display;

StyledNode *style_tree(Node *root, Stylesheet *stylesheet);

Value *style_value(StyledNode *node, const char *name);

Value *style_lookup(StyledNode *self, const char *name, const char *fallback_name, Value *def);

Display style_display(StyledNode *node);

#endif
