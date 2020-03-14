/*
https://limpet.net/mbrubeck/2014/08/08/toy-layout-engine-1.html
*/
#include "refcnt.h"
#include "refhash.h"
#include "refalist.h"
#include "dom.h"

static void node_dtor(void *p) {
    Node *n = p;
    if(n->children) rc_release(n->children);    
	
    if(n->type == T_TEXT) {
        free(n->text);
    } else if(n->type == T_ELEMENT) {
        free(n->element.tag_name);
        rc_release(n->element.attributes);
    }
}

Node *text_node(char *text) {
    Node *n = rc_alloc(sizeof *n);
    n->children = NULL;
    n->type = T_TEXT;
    n->text = text; // malloced in html.c
	rc_set_dtor(n, node_dtor);
    return n;
}

Node *elem_node(char *name, HashTable *attrs, ArrayList *children) {
    Node *n = rc_alloc(sizeof *n);
    n->type = T_ELEMENT;
    n->element.tag_name = name;
    n->element.attributes = attrs;
    //n->children = rc_retain(children);
    n->children = children; // Children is already retained in html.c
	rc_set_dtor(n, node_dtor);
    return n;
}

const char *elem_id(ElementData *ed) {
	const char *id = ht_get(ed->attributes, "id");
	if(!id) return "";
	return id;
}

ArrayList *elem_classes(ElementData *ed) {
	ArrayList *list = al_create();
	const char *classes = ht_get(ed->attributes, "class");
	if(classes) {
		for(;;) {
			while(isspace(classes[0])) classes++;
			if(!classes[0]) break;
			const char *start = classes;
			while(classes[0] && !isspace(classes[0])) classes++;
			size_t len = classes - start;
			char *str = rc_alloc(len + 1);
			memcpy(str, start, len);
			str[len] = '\0';
			al_add(list, str);
		}
	}
	return list;
}