/*
This follows [part 3][part3] of the tutorial, but I'd like to have it look a 
bit more like the [spec][].

[part3]: https://limpet.net/mbrubeck/2014/08/13/toy-layout-engine-3-css.html
[spec]: https://www.w3.org/TR/2011/REC-CSS2-20110607/syndata.html

*/

#include "refcnt.h"
#include "refalist.h"
#include "stream.h"

#include "css.h"

extern int consume_space(Stream *strm); /* from html.c */ 

static void selector_dtor(void *p) {
    Selector *sel = p;
    if(sel->type == SimpleSelector) {
        if(sel->simpleSelector.tag_name)
			rc_release(sel->simpleSelector.tag_name);
        if(sel->simpleSelector.id)
			rc_release(sel->simpleSelector.id);
		rc_release(sel->simpleSelector.class);
    }
}

static Selector *create_simple_selector() {
	Selector *sel = rc_alloc(sizeof *sel);
	rc_set_dtor(sel, selector_dtor);
	sel->type = SimpleSelector;
	sel->simpleSelector.tag_name = NULL;
	sel->simpleSelector.id = NULL;
	sel->simpleSelector.class = al_create();
	return sel;
}

static int valid_identifier_char(int c) {
	if(isalnum(c) || strchr("-_", c)) return 1;
	return 0;
}

static char *parse_identifier(Stream *strm) { 
	//printf("parse_identifier:%d\n", strm->line);	
    size_t ba = 16, bn = 0;
    char *buffer = rc_alloc(ba);    
    int c = strm_getc(strm);
    while(c && valid_identifier_char(c)) {
        strm_next(strm);
        // assert(bn < ba);
        buffer[bn++] = c;
        if(bn == ba) {
            ba <<= 1;
            buffer = rc_realloc(buffer, ba);
        }
        c = strm_getc(strm);
    }
    // assert(bn < ba);
    buffer[bn] = '\0';
    return buffer;
}

static Selector *parse_simple_selector(Stream *strm) {
	//printf("parse_simple_selector:%d\n", strm->line);	
	Selector *selector = create_simple_selector();
	int c = strm_getc(strm);
	while(c) {
		c = strm_getc(strm);
		if(c == '#') {
			strm_next(strm);
			selector->simpleSelector.id = parse_identifier(strm);
		} else if(c == '.') {
			strm_next(strm);
			char *class = parse_identifier(strm);
			al_add(selector->simpleSelector.class, class);
		} else if(c == '*') {
			strm_next(strm);
		} else if(valid_identifier_char(c)) {
			selector->simpleSelector.tag_name = parse_identifier(strm);
		} else
			break;
	}
	return selector;
}

Specificity specificity(const Selector *sel) {
	Specificity s;
	s.a = sel->simpleSelector.id != NULL;
	s.b = sel->simpleSelector.class ? al_size(sel->simpleSelector.class) : 0;
	s.c = sel->simpleSelector.tag_name != NULL;
	return s;
}

int specificity_cmp(const void *p2, const void *p1) {
	/* Note that p1 and p2 are backwards to sort in descending order */
	Specificity s1 = specificity(p1);
	Specificity s2 = specificity(p2);
	
	//printf("Specificity (%d %d %d) vs (%d %d %d)\n", s1.a, s1.b, s1.c, s2.a, s2.b, s2.c);fflush(stdout);
	
	if(s1.a > s2.a) return 1;
	else if(s2.a > s1.a) return -1;
	if(s1.b > s2.b) return 1;
	else if(s2.b > s1.b) return -1;
	if(s1.c > s2.c) return 1;
	else if(s2.c > s1.c) return -1;
	return 0;
}

static ArrayList *parse_selectors(Stream *strm) {
	//printf("parse_selectors:%d\n", strm->line);	
	ArrayList *selectors = al_create();
	for(;;) {
		Selector *sel = parse_simple_selector(strm);
		al_add(selectors, sel);
		consume_space(strm);
		int c = strm_getc(strm);
		if(c == ',') {
			strm_next(strm);
			consume_space(strm);
		} else if(c == '{') {
			break;
		} else {
			fprintf(stderr, "error:%d: didn't expect '%c'\n", strm->line, c);
			break;
		}
	}
	
	al_sort(selectors, specificity_cmp);
	
	return selectors;
}

static float parse_float(Stream *strm) {
	//printf("parse_float:%d\n", strm->line);	
	char buffer[16];
	int i = 0, c = strm_getc(strm);
	while((isdigit(c) || c == '.') && i < sizeof(buffer)) {
		buffer[i++] = c;
		c = strm_next(strm);		
	}
	return atof(buffer);
}

static struct Length parse_length(Stream *strm) {
	//printf("parse_length:%d\n", strm->line);	
	struct Length length;
	length.v = parse_float(strm);
	char *unit = parse_identifier(strm), *c;
	for(c = unit; *c; c++) *c = tolower(*c);
	if(!strcmp(unit, "px")) {
		length.unit = Px;
	} else {
		fprintf(stderr, "error:%d: unrecognized unit '%s'\n", strm->line, unit);
	}
	rc_release(unit);
	return length;
}

static void value_dtor(void *p) {
	Value *value = p;
	if(value->type == Keyword && value->keyword != NULL)
		rc_release(value->keyword);
	else if(value->type == Color && value->color_txt != NULL)
		rc_release(value->color_txt);
}

Value *new_value() {
	Value *value = rc_alloc(sizeof *value);
	rc_set_dtor(value, value_dtor);
    return value;
}

Value *new_length(float len, enum Unit unit) {
    Value *v = new_value();
    v->type = Length;
    v->length.v = len;
    v->length.unit = unit;
	return v;
}

static Value *parse_value(Stream *strm) {
	//printf("parse_value:%d\n", strm->line);	
	Value *value = new_value();
	if(isdigit(strm_getc(strm))) {
		value->type = Length;
		value->length = parse_length(strm);
	} else if(strm_getc(strm) == '#') {
		// Color is handled slightly differently
		strm_next(strm);
		value->type = Color;
		value->color_txt = parse_identifier(strm);
	} else {
		value->type = Keyword;
		value->keyword = parse_identifier(strm);
	}
	return value;
}

float value_to_px(Value *v) {
    if(v->type == Length) {
        if(v->length.unit == Px) {
            return v->length.v;
        }
    }
    return 0.0;
}

static void decl_dtor(void *p) {
	Declaration *decl = p;
	rc_release(decl->name);
	rc_release(decl->value);
}
	
static Declaration *parse_declaration(Stream *strm) {	
	//printf("parse_declaration:%d\n", strm->line);	
	Declaration *decl = rc_alloc(sizeof *decl);	
	rc_set_dtor(decl, decl_dtor);
	decl->value = NULL;
	decl->name = parse_identifier(strm);
	consume_space(strm);
	if(strm_getc(strm) != ':') {
		fprintf(stderr, "error:%d: expected ':'\n", strm->line);
		goto error;
	}		
	strm_next(strm);
	consume_space(strm);
	decl->value = parse_value(strm);
	consume_space(strm);
	if(strm_getc(strm) != ';') {
		fprintf(stderr, "error:%d: expected ';'\n", strm->line);
		goto error;
	}	
	strm_next(strm);
	return decl;
error:
	rc_release(decl);
	return NULL;
}

static ArrayList *parse_declarations(Stream *strm) {
	//printf("parse_declarations:%d\n", strm->line);	
	ArrayList *declarations = al_create();
	assert(strm_getc(strm) == '{');
	strm_next(strm);
	for(;;) {
		consume_space(strm);
		if(strm_getc(strm) == '}') {
			strm_next(strm);
			break;
		}
		Declaration *decl = parse_declaration(strm);
		if(!decl) {
			rc_release(declarations);
			return NULL;
		}
		al_add(declarations, decl);
	}
	return declarations;
}
static void rule_dtor(void *p) {
	Rule *rule = p;
	rc_release(rule->selectors);
	rc_release(rule->declarations);
}

static Rule *parse_rule(Stream *strm) {
	//printf("parse_rule:%d\n", strm->line);
	Rule *rule = rc_alloc(sizeof *rule);
	rc_set_dtor(rule, rule_dtor);
	rule->selectors = parse_selectors(strm);
	rule->declarations = parse_declarations(strm);
	return rule;
}

static void ss_dtor(void *p) {
	Stylesheet *ss = p;
	rc_release(ss->rules);
}

Stylesheet *parse_rules(Stream *strm) {
	Stylesheet *ss = rc_alloc(sizeof *ss);
	rc_set_dtor(ss, ss_dtor);
	ss->rules = al_create();
	for(;;) {
		consume_space(strm);
		if(!strm_getc(strm)) break;
		al_add(ss->rules, parse_rule(strm));
	}
	return ss;
}

static void print_rule(Rule *rule) {
	int i, j;
	printf("RULE:\n");
	for(i = 0; i < al_size(rule->selectors); i++) {
		// implicit if selector->type == SimpleSelector
		printf("  Selector[%d]\n", i);
		Selector *sel = al_get(rule->selectors, i);
		if(sel->simpleSelector.id != NULL)
			printf("    id = '%s'\n", sel->simpleSelector.id);
		if(al_size(sel->simpleSelector.class)) {
			printf("    classes = ");
			for(j = 0; j < al_size(sel->simpleSelector.class); j++)
				printf("%s, ", (char*)al_get(sel->simpleSelector.class, j));
			printf("\n");
		}
		if(sel->simpleSelector.tag_name != NULL)
			printf("    tag = '%s'\n", sel->simpleSelector.tag_name);
		
	}
	printf("  Declarations:\n");		
	for(i = 0; i < al_size(rule->declarations); i++) {
	    Declaration *decl = al_get(rule->declarations, i);
		switch(decl->value->type) {
			case Keyword: 
				printf("    %s : %s\n", decl->name, decl->value->keyword);
				break;
			case Length: 
				printf("    %s : %f (px)\n", decl->name, decl->value->length.v);
				// the px is implicit, since no other types are supported
				break;
			case Color: 
				printf("    %s : %s\n", decl->name, decl->value->color_txt);
				break;
		}
	}
}
void print_styles(Stylesheet *ss) {
	int i;
	for(i = 0; i < al_size(ss->rules); i++) {
		print_rule(al_get(ss->rules, i));
	}
}
