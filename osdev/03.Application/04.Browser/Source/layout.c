#include "refcnt.h"
#include "refalist.h"
#include "refhash.h"
#include "dom.h"
#include "css.h"
#include "style.h"
#include "layout.h"

static LayoutBox *build_layout_tree(StyledNode *style_node);
static void layout(LayoutBox *self, Dimensions *containing_block);

static void layoutbox_dtor(void *p) {
	LayoutBox *b = p;
	if(b->box_type.node) rc_release(b->box_type.node);
	rc_release(b->children);
}

static Rect expanded_by(Rect rect, EdgeSizes edge) {
    rect.x -= edge.left;
    rect.y -= edge.top;
    rect.width += edge.left + edge.right;
    rect.height += edge.top + edge.bottom;
    return rect;
}

static Rect padding_box(Dimensions *self) {
    return expanded_by(self->content, self->padding);
}

Rect border_box(Dimensions *self) {
    return expanded_by(padding_box(self), self->border);
}

static Rect margin_box(Dimensions *self) {
    return expanded_by(border_box(self), self->margin);
}

static LayoutBox *new_layout_box() {
	LayoutBox *root = rc_alloc(sizeof *root);
	rc_set_dtor(root, layoutbox_dtor);
	root->children = al_create();
	root->box_type.type = BlockNode;
	root->box_type.node = NULL;
	memset(&root->dimensions, 0, sizeof root->dimensions);
	return root;
}

static LayoutBox *get_inline_container(LayoutBox *self) {
	switch(self->box_type.type) {
	case InlineNode:
	case AnonymousBlock: return self;
	case BlockNode: {
		LayoutBox *last = al_last(self->children);
		if(!last) {
			last = new_layout_box();
			last->box_type.type = AnonymousBlock;
			al_add(self->children, last);
		}
		return last;
	}
	}
	assert(0); // Should be unreachable...
	return self;
}

static LayoutBox *build_layout_tree(StyledNode *style_node) {
	LayoutBox *root = new_layout_box();
	
	switch(style_display(style_node)) {
	case Block : {
		root->box_type.type = BlockNode;
		root->box_type.node = rc_retain(style_node);
	} break;
	case Inline : {
		root->box_type.type = InlineNode;
		root->box_type.node = rc_retain(style_node);
	} break;
	case None : {
		/* TODO: Perhaps better error handling? */
		fprintf(stderr, "Root node has display:none\n"); 
		rc_release(root);
		return NULL; 
	}
	}
	
	int i;
	for(i = 0; i < al_size(style_node->children); i++) {
		StyledNode *child = al_get(style_node->children, i);
		switch(style_display(child)) {
			case Block : {
				LayoutBox *child_tree = build_layout_tree(child);
				al_add(root->children, child_tree); 
			} break;
			case Inline : {
				LayoutBox *child_tree = build_layout_tree(child);
				LayoutBox *container = get_inline_container(root);
				al_add(container->children, child_tree); 
			} break;
			default: break;
		}
	}
	
	return root;
}

static StyledNode *get_style_node(LayoutBox *self) {
    if(self->box_type.type != BlockNode && self->box_type.type != InlineNode) {
        // fprintf(stderr, "Anonymous block box has no style node");
        // abort(); /* FIXME: Error handing */
    }
    return self->box_type.node;
}

static int is_auto(Value *v) {
    return v->type == Keyword && !strcmp(v->keyword, "auto");
}

static void calculate_block_width(LayoutBox *self, Dimensions *containing_block) {
	StyledNode *style = get_style_node(self);
    assert(style);
    
    Value *Auto = new_value();
    Auto->type = Keyword;
    Auto->keyword = rc_strdup("auto");
        
    Value *zero = new_length(0.0, Px);
    
    Value *width = style_value(style, "width");    
    if(!width) width = Auto;
    width = rc_retain(width);
    
    Value *margin_left = style_lookup(style, "margin-left", "margin", zero);
    Value *margin_right = style_lookup(style, "margin-right", "margin", zero);
    
    Value *border_left = style_lookup(style, "border-left-width", "border", zero);
    Value *border_right = style_lookup(style, "border-right-width", "border", zero);
    
    Value *padding_left = style_lookup(style, "padding-left", "padding", zero);
    Value *padding_right = style_lookup(style, "padding-right", "padding", zero);
    
    float total = value_to_px(margin_left) + value_to_px(margin_right) +
            value_to_px(border_left) + value_to_px(border_right) +
            value_to_px(padding_left) + value_to_px(padding_right) +
            value_to_px(width);
    
    if(is_auto(width) && total > containing_block->content.width) {
        if(is_auto(margin_left)) {
            margin_left = zero;
        }
        if(is_auto(margin_right)) {
            margin_right = zero;
        }
    }
    margin_left = rc_retain(margin_left);
    margin_right = rc_retain(margin_right);
    
    float underflow = containing_block->content.width - total;
    
    if(is_auto(width)) {
        // (true, _, _)
        if(is_auto(margin_left)) margin_left = zero;
        if(is_auto(margin_right)) margin_right = zero;
                
        if(underflow >= 0) {
            rc_assign((void**)&width, new_length(underflow, Px));
        } else {
            rc_assign((void**)&width, new_length(0.0, Px)); 
            rc_assign((void**)&margin_right, new_length(value_to_px(margin_right) + underflow, Px));
        }        
    } else {
        if(is_auto(margin_left)) {            
            if(is_auto(margin_right)) {
                // (false, true, true)
                rc_assign((void**)&margin_left, new_length(underflow / 2.0, Px));
                rc_assign((void**)&margin_right, new_length(underflow / 2.0, Px));
            } else {                
                // (false, true, false)
                rc_assign((void**)&margin_left, new_length(underflow, Px));
            }
        } else {
            if(is_auto(margin_right)) {
                // (false, false, true)
                rc_assign((void**)&margin_right, new_length(underflow, Px));
            } else {
                // (false, false, false)                
                rc_assign((void**)&margin_right, new_length(value_to_px(margin_right) + underflow, Px));
            }        
        }
    }
    
    Dimensions *d = &self->dimensions;
    d->content.width = value_to_px(width);
    d->padding.left = value_to_px(padding_left);
    d->padding.right = value_to_px(padding_right);
    d->border.left = value_to_px(border_left);
    d->border.right = value_to_px(border_right);
    d->margin.left = value_to_px(margin_left);
    d->margin.right = value_to_px(margin_right);
    
    rc_release(width);    
    rc_release(margin_right);    
    rc_release(margin_left);    
    
    rc_release(zero);    
    rc_release(Auto);        
}

static void calculate_block_position(LayoutBox *self, Dimensions *containing_block) {
    StyledNode *style = get_style_node(self);
    assert(style);
    Dimensions *d = &self->dimensions;
	Value *zero = new_length(0.0, Px);
    
    d->margin.top = value_to_px(style_lookup(style, "margin-top", "margin", zero));
    d->margin.bottom = value_to_px(style_lookup(style, "margin-bottom", "margin", zero));
    
    d->border.top = value_to_px(style_lookup(style, "border-top-width", "border", zero));
    d->border.bottom = value_to_px(style_lookup(style, "border-bottom-width", "border", zero));
    
    d->padding.top = value_to_px(style_lookup(style, "padding-top", "padding", zero));
    d->padding.bottom = value_to_px(style_lookup(style, "padding-bottom", "padding", zero));
    
    d->content.x = containing_block->content.x + d->margin.left + d->border.left + d->padding.left;
    d->content.y = containing_block->content.height + containing_block->content.y + 
                    d->margin.top + d->border.top + d->padding.top;
    
    rc_release(zero);
}

static void layout_block_children(LayoutBox *self) {
	Dimensions *d = &self->dimensions;
    int i;
    for(i = 0; i < al_size(self->children); i++) {
        LayoutBox *child = al_get(self->children, i);
        layout(child, d);
        Rect cmb = margin_box(&child->dimensions);
        d->content.height = d->content.height + cmb.height;
    }
}

static void calculate_block_height(LayoutBox *self) {
	Value *height = style_value(get_style_node(self), "height");
	if(height && height->type == Length) {
		self->dimensions.content.height = value_to_px(height);
	}
}

static void layout_block(LayoutBox *self, Dimensions *containing_block) {
	calculate_block_width(self, containing_block);
	calculate_block_position(self, containing_block);
	layout_block_children(self);
	calculate_block_height(self);
}

static void calculate_inline_width_height(LayoutBox *self) {
  StyledNode *style = get_style_node(self);
  assert(style);

  Value *zero = new_length(0.0, Px);

  // margin, border, and padding have initial value 0.
  Value *margin_left = style_lookup(style, "margin-left", "margin", zero);
  Value *margin_right = style_lookup(style, "margin-right", "margin", zero);
  Value *margin_top = style_lookup(style, "margin-top", "margin", zero);
  Value *margin_bottom = style_lookup(style, "margin-bottom", "margin", zero);

  Value *border_left = style_lookup(style, "border-left-width", "border", zero);
  Value *border_right = style_lookup(style, "border-right-width", "border", zero);
  Value *border_top = style_lookup(style, "border-top", "border", zero);
  Value *border_bottom = style_lookup(style, "borde-bottom", "border", zero);

  Value *padding_left = style_lookup(style, "padding-left", "padding", zero);
  Value *padding_right = style_lookup(style, "padding-right", "padding", zero);
  Value *padding_top = style_lookup(style, "padding-top", "padding", zero);
  Value *padding_bottom = style_lookup(style, "padding-bottom", "padding", zero);

  // calculate text size
  float w, h;
  if (style->node->text != NULL) {
    w = strlen(style->node->text) * 1.2;
    h = 10;
  }
  else {
    w = h = 0;
  }
  Value *width = style_lookup(style, "width", "width", new_length(w, Px));
  width = rc_retain(width);

  Value *height = new_length(h, Px);
  height = rc_retain(height);

  // make room for children, if any
  int i;
  for (i = 0; i < al_size(self->children); i++) {
    LayoutBox *child = al_get(self->children, i);
    //if (child->box_type.type == Image)
    //  calculate_image_width_height(child);  // TODO
    //else
      calculate_inline_width_height(child);

    Rect cmb = margin_box(&child->dimensions);

    if (cmb.width > value_to_px(width))
      rc_assign((void**)&width, new_length(cmb.width, Px));

    if (cmb.height > value_to_px(height))
      rc_assign((void**)&height, new_length(cmb.height, Px));
  }

  Dimensions *d = &self->dimensions;
  d->content.width = value_to_px(width);
  d->content.height = value_to_px(height);

  d->padding.left = value_to_px(padding_left);
  d->padding.right = value_to_px(padding_right);
  d->padding.top = value_to_px(padding_top);
  d->padding.bottom = value_to_px(padding_bottom);

  d->border.left = value_to_px(border_left);
  d->border.right = value_to_px(border_right);
  d->border.top = value_to_px(border_top);
  d->border.bottom = value_to_px(border_bottom);

  d->margin.left = value_to_px(margin_left);
  d->margin.right = value_to_px(margin_right);
  d->margin.top = value_to_px(margin_top);
  d->margin.bottom = value_to_px(margin_bottom);

  rc_release(height);
  rc_release(width);

  rc_release(zero);
}

static void calculate_inline_position(LayoutBox *self, Dimensions *containing_block) {
  Dimensions *d = &self->dimensions;
  
  // Position the box left to last inline box or wrap to next line
  float nw = 0 + d->content.width + 
    d->margin.left + d->border.left + d->padding.left +
    d->margin.right + d->border.right + d->padding.right;

  // wrap : TODO
  if (nw > containing_block->content.width)
    ;

  d->content.x = containing_block->content.x + 0 +
    d->margin.left + d->border.left + d->padding.left;
  d->content.y = containing_block->content.y + 0 +
    d->margin.top + d->border.top + d->padding.top;

  // advance cursor, add box to current line for alignment : TODO
  nw += d->content.width +
    d->margin.left + d->border.left + d->padding.left +
    d->margin.right + d->border.right + d->padding.right;

  // line append : TODO
}

static void layout_inline_children(LayoutBox *self, Dimensions *containing_block) {
  Dimensions *d = &self->dimensions;
  int i;
  for (i = 0; i < al_size(self->children); i++) {
    LayoutBox *child = al_get(self->children, i);
    layout(child, d);
    Rect cmb = margin_box(&child->dimensions);
    d->content.height = d->content.height + cmb.height;
  }
}

static void layout_inline(LayoutBox *self, Dimensions *containing_block) {
  // Lay out a inline-level element and its descendants.

  // compute width and height of this box and all descendants
  calculate_inline_width_height(self);

  // Determine where the box is located within its container.
  calculate_inline_position(self, containing_block);

  // Recursively lay out the children of this box.
  layout_inline_children(self, containing_block);
}

static void layout(LayoutBox *self, Dimensions *containing_block) {
	switch(self->box_type.type) {
		case BlockNode : layout_block(self, containing_block); break;
    case InlineNode: layout_inline(self, containing_block); break;
		case AnonymousBlock : layout_block(self, containing_block); break;
	}
}

LayoutBox *layout_tree(StyledNode *node, Dimensions containing_block) {
    // Doh, layout_tree() modifies its input, so make a copy of viewport
    //Dimensions containing_block = viewport; 
	containing_block.content.height = 0.0;
	LayoutBox *root_box = build_layout_tree(node);
	layout(root_box, &containing_block);
	return root_box;
}

