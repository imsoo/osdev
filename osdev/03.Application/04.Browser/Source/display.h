#include "OSLibrary.h"

typedef struct {
    enum {SolidColor, Text} type;
    
    union {
        struct { char *color; Rect rect; } solidColor;
        struct { char *text; Rect rect; } Text;
        // More display commands to follow...
    };
    
} DisplayCommand;

ArrayList *build_display_list(LayoutBox *layout_root);
