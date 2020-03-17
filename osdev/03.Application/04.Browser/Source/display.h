#include "OSLibrary.h"

typedef struct {
    enum { SolidColor, Text, Img } type;
    
    union {
        struct { char *color; Rect rect; } solidColor;
        struct { char *text; Rect rect; } Text;
        struct { char *src; char* alt; Rect rect; } Img;
        // More display commands to follow...
    };
    
} DisplayCommand;

ArrayList *build_display_list(LayoutBox *layout_root);
