
#ifndef LAUNCHER_H
#define LAUNCHER_H

#define LAUNCHER_MAX_STRLEN 128

#include <X11/Xlib.h>

#include "list.h"
#include "selectbox.h"

struct WM_t;

struct launcher_t
{
    Window              win;
    int                 height;
    GC                  gc;
    unsigned long       inputeventmask;

    int                 len;
    char                str[LAUNCHER_MAX_STRLEN];

    struct List         *suggestions;
    int                 selected;

    struct selectbox_t  *sb;
    XFontStruct         *font;
};

void launcher_init(struct WM_t *);
void launcher(struct WM_t *);

#endif

