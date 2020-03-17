#include "refcnt.h"
#include "refhash.h"
#include "refalist.h"
#include "dom.h"
#include "stream.h"
#include "html.h"
#include "css.h"
#include "style.h"
#include "layout.h"
#include "display.h"
#include "print.h"

void dump_file(const char *fname) {
    Stream *r = file_stream(fname);
    if(!r) {
        fprintf(stderr, "error: no %s\n", fname);
        return;
    }
    int c;
    while((c = strm_getc(r))) {
        // putc(c, stdout);
        strm_next(r);
    }
    strm_done(r);
}

void print_node(Node *n, int lvl) {
	printf("%*c ", lvl * 2, '-');
	switch(n->type) {
		case T_ELEMENT: {
			int i;
			const char *k;
			printf("<%s/>\n", n->element.tag_name);
			k = ht_next(n->element.attributes, NULL);
			while(k) {
				printf("%*c%s = '%s'\n", lvl * 2 + 2, '@', k, (char*)ht_get(n->element.attributes, k));
				k = ht_next(n->element.attributes, k);
			}
			
			for(i = 0; i < al_size(n->children); i++) {
				Node *np = al_get(n->children, i);
				print_node(np, lvl+1);
			}
		} break;
		case T_TEXT: 
			printf("'%s'\n", n->text);
			break;
	}
}

static void out_stylednode(StyledNode *sn, int lvl) {
	printf("%*c ", lvl * 2, '-');	
    Node *n = sn->node;	
	const char *k;
	if(n->type == T_ELEMENT) {
		printf("<%s/>\n", n->element.tag_name);			
		k = ht_next(n->element.attributes, NULL);
		while(k) {
			printf("%*c%s = '%s'\n", lvl * 2 + 2, '@', k, (char*)ht_get(n->element.attributes, k));
			k = ht_next(n->element.attributes, k);
		}
	} else { 
		assert(n->type == T_TEXT); 
		printf("'%s'\n", n->text);
	}
	for(k = ht_next(sn->specified_values, NULL); k; k = ht_next(sn->specified_values, k)) {
		Value *value = ht_get(sn->specified_values, k);
		switch(value->type) {
			case Keyword: 
				printf("%*c%s: %s\n", lvl * 2 + 2, '$', k, value->keyword);
				break;
			case Length: 
				printf("%*c%s: %f (px)\n", lvl * 2 + 2, '$', k, value->length.v);
				// the px is implicit, since no other types are supported
				break;
			case Color: 
				printf("%*c%s: %s\n", lvl * 2 + 2, '$', k, value->color_txt);
				break;
		}
	}
}

void print_stylednode(StyledNode *sn, int lvl) {
	out_stylednode(sn, lvl);
	
	int i;
	for(i = 0; i < al_size(sn->children); i++) {
		StyledNode *child = al_get(sn->children, i);
		print_stylednode(child, lvl+1);
	}
}

static void out_rect(Rect *r, int lvl) {
    printf("%*c %g, %g : %g x %g\n", lvl * 2, '-', r->x, r->y, r->width, r->height);
}
static void out_edge(EdgeSizes *e, int lvl) {
    printf("%*c %g %g %g %g\n", lvl * 2, '-', e->left, e->right, e->top, e->bottom);
}

static void out_dims(Dimensions *d, int lvl) {
    out_rect(&d->content, lvl);
    out_edge(&d->padding, lvl);
    out_edge(&d->border, lvl);
    out_edge(&d->margin, lvl);
}

void print_layoutbox(LayoutBox *lb, int lvl) {
    switch(lb->box_type.type) {
        case BlockNode:             
            printf("%*c BlockNode\n", lvl * 2, '-');  
            out_dims(&lb->dimensions, lvl);
            out_stylednode(lb->box_type.node, lvl); 
            break;
        case InlineNode:    
            printf("%*c InlineNode\n", lvl * 2, '-'); 
            out_dims(&lb->dimensions, lvl);   
            out_stylednode(lb->box_type.node, lvl); 
            break;
        case AnonymousBlock: 
            printf("%*c AnonymousBlock\n", lvl * 2, '-');
            out_dims(&lb->dimensions, lvl);
            break;
    }
	
	int i;
	for(i = 0; i < al_size(lb->children); i++) {
		LayoutBox *child = al_get(lb->children, i);
		print_layoutbox(child, lvl+1);
	}
}

void print_displaylist(ArrayList *dl) {
  int i;
  for (i = 0; i < al_size(dl); i++) {
    DisplayCommand *cmd = al_get(dl, i);
    switch (cmd->type) {
    case SolidColor:
      printf("command: SolidColor \"%s\"\n", cmd->solidColor.color);
      // out_rect(&cmd->solidColor.rect, 1);
      break;

    case Text:
      printf("command: Text %s\n", cmd->Text.text);
      break;

    case Img:
      printf("command: Img src : %s, alt : %s\n", cmd->Img.src, cmd->Img.alt);
      break;
    }
  }
}
