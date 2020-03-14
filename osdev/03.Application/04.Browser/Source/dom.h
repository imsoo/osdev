#ifndef DOM_H
#define DOM_H

#include "OSLibrary.h"

typedef enum NodeType {
    T_TEXT, T_ELEMENT
} NodeType;

typedef struct ElementData {
    char *tag_name;
    HashTable *attributes;
} ElementData;

typedef struct Node {
    ArrayList *children;
    NodeType type;
    
    union {
        char *text;
        ElementData element;
    };
} Node;

Node *text_node(char *text);

Node *elem_node(char *name, HashTable *attrs, ArrayList *children);

const char *elem_id(ElementData *ed);

ArrayList *elem_classes(ElementData *ed);

#endif