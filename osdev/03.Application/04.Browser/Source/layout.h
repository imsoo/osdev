#include "OSLibrary.h"

typedef struct {
	float x, y;
	float width, height;
} Rect;

typedef struct {
	float left;
	float right;
	float top;
	float bottom;
} EdgeSizes;

typedef struct {
	Rect content;
	EdgeSizes padding;
	EdgeSizes border;
	EdgeSizes margin;
} Dimensions;

typedef struct {
	enum { BlockNode, InlineNode, AnonymousBlock } type;
	StyledNode *node;
} BoxType;

typedef struct {
	Dimensions dimensions;
	BoxType box_type;
	ArrayList *children;
} LayoutBox;

LayoutBox *layout_tree(StyledNode *node, Dimensions containing_block);

Rect border_box(Dimensions *self);
