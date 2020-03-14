#include "refcnt.h"
#include "refhash.h"
#include "refalist.h"
#include "dom.h"
#include "css.h"
#include "style.h"

static int matches_simple_selector(ElementData *elem, struct SimpleSelector *simpleSelector) {
	if(simpleSelector->tag_name && strcmp(simpleSelector->tag_name, elem->tag_name))
		return 0;
	
	if(simpleSelector->id && strcmp(simpleSelector->id, elem_id(elem)))
		return 0;
	
	if(al_size(simpleSelector->class) > 0) {
		ArrayList *classes = elem_classes(elem);
		int i, j;
		for(j = 0; j < al_size(simpleSelector->class); j++) {
			const char *sel_class = al_get(simpleSelector->class, j);				
			int contains = 0;
			for(i = 0; i < al_size(classes); i++) {
				const char *elem_class = al_get(classes, i);
				if(!strcmp(elem_class, sel_class)) {
					//rc_release(classes);
					//return 0;
					contains = 1;
				}
			}
			if(!contains) {
				rc_release(classes);
				return 0;
			}
		}
		rc_release(classes);
	}
	
	return 1;
}

static int matches(ElementData *elem, Selector *selector) {
	if(selector->type == SimpleSelector) {
		return matches_simple_selector(elem, &selector->simpleSelector);
	}
	assert(0); /* Unsupported selector type */
	return 0;
}

typedef struct {
	Selector *selector;
	Rule *rule;
} MatchedRule;

static void matchedrule_dtor(void *p) {
	MatchedRule *m = p;
	rc_release(m->selector);
	rc_release(m->rule);
}

static MatchedRule *match_rule(ElementData *elem, Rule *rule) {
	int i;
	for(i = 0; i < al_size(rule->selectors); i++) {
		Selector *selector = al_get(rule->selectors, i);
		if(matches(elem, selector)) {
			MatchedRule *match = rc_alloc(sizeof *match);
			rc_set_dtor(match, matchedrule_dtor);
			match->selector = rc_retain(selector);
			match->rule = rc_retain(rule);
			return match;
		}
	}
	return NULL;
}

ArrayList *matching_rules(ElementData *elem, Stylesheet *ss) {
	ArrayList *matches = al_create();
	int i;
	for(i = 0; i < al_size(ss->rules); i++) {
		Rule *rule = al_get(ss->rules, i);
		MatchedRule *match = match_rule(elem, rule);
		if(match) {
			// printf("MatchedRule %p\n", match);fflush(stdout);
			al_add(matches, match);
		}
	}
	return matches;
}

extern int specificity_cmp(const void *, const void *); /* from css.c */

static int matched_rule_cmp(const void *p1, const void *p2) {
	
	/* Mmmm, maybe it is just a bit late, but I can't get my head around why I
	treat it as a void** here and only as a void* in parse_selectors()
	*/
	
	const MatchedRule *m1 = *(void **)p1, *m2 = *(void **)p2;
	/* This time sort ascending, hence the - */ 
	return -specificity_cmp(m1->selector, m2->selector);
}

HashTable *specified_values(ElementData *elem, Stylesheet *ss) {
	int i, j;
	HashTable *values = ht_create();
	ArrayList *rules = matching_rules(elem, ss);
	
	al_sort(rules, matched_rule_cmp);
	for(i = 0; i < al_size(rules); i++) {
		MatchedRule *match = al_get(rules, i);
		Rule *rule = match->rule;
		for(j = 0; j < al_size(rule->declarations); j++) {
			Declaration *decl = al_get(rule->declarations, j);
			ht_retain(values, decl->name, decl->value);
		}
	}
	
	rc_release(rules);
	return values;
}

static void stylenode_dtor(void *p) {
	StyledNode *node = p;
	rc_release(node->node);
	rc_release(node->specified_values);
	rc_release(node->children);
}

StyledNode *style_tree(Node *root, Stylesheet *stylesheet) {
	StyledNode *node = rc_alloc(sizeof *node);
	rc_set_dtor(node, stylenode_dtor);
	node->node = rc_retain(root);	
	if(root->type == T_ELEMENT) {		
		node->specified_values = specified_values(&root->element, stylesheet);
	} else {
		node->specified_values = ht_create();
	}
	node->children = al_create();
	if(root->children) {
		int i;
		for(i = 0; i < al_size(root->children); i++) {
			Node *child = al_get(root->children, i);
			al_add(node->children, style_tree(child, stylesheet));
		}	
	}
	return node;
}

Value *style_value(StyledNode *self, const char *name) {
	if(!self) return NULL;
	return ht_get(self->specified_values, name);
}

Value *style_lookup(StyledNode *self, const char *name, const char *fallback_name, Value *def) {
  if (self == NULL) return def;

  Value *v = style_value(self, name);
    if(!v) v = style_value(self, fallback_name);
    if(!v) v = def;
    return v;
}

Display style_display(StyledNode *node) {
	Value *v = style_value(node, "display");
	if(v && v->type == Keyword) {
		if(!strcmp(v->keyword, "block")) return Block;
		if(!strcmp(v->keyword, "none")) return None;
	}
	return Inline;
}
