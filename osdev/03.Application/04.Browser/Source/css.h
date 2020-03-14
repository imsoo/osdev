#ifndef CSS_H
#define CSS_H

#include "OSLibrary.h"

typedef struct {
	enum {SimpleSelector} type;
	union {
		struct SimpleSelector {
			char *tag_name;
			char *id;
			ArrayList *class;
		} simpleSelector;
	};
} Selector;

typedef struct {
	int a, b, c;
} Specificity;

typedef struct {
	enum {
		Keyword,
		Length,
		Color
	} type;
	union {
		char *keyword;
		struct Length {
			float v;
			enum Unit {Px} unit;
		} length;
        // I have a reason for storing the color as a string 
        // rather than parsing it when I read the CSS
		char *color_txt;
	};
} Value;

typedef struct {
	char *name;
	Value *value;
} Declaration;

typedef struct {
	ArrayList *selectors;
	ArrayList *declarations;
} Rule;

typedef struct {
	ArrayList *rules;
} Stylesheet;

#  ifdef STREAM_H
Stylesheet *parse_rules(Stream *strm);
#  endif

Value *new_value();
Value *new_length(float len, enum Unit unit);

float value_to_px(Value *v);

Specificity specificity(const Selector *sel);

void print_styles(Stylesheet *ss);
#endif