
#ifndef SELECTBOX_H
#define SELECTBOX_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "wm.h"

struct selectbox_t
{
    struct          WM_t *wm;
    Window          win;
    XFontStruct     *font;
    GC              gc;

    int             x, y;
    int             width, height;
    int             item_height;

    int             centre;

    char            **items;
    int             n_items;
};

/* X, Y, width, is_centered, items, n_items */
struct selectbox_t *selectbox_new(struct WM_t *, int, int, int, int, char **, int, XFontStruct *);

/* Select box instance, item to draw selected (-1 for none) */
void selectbox_draw(struct selectbox_t *, int);

void selectbox_close(struct selectbox_t *);

#endif

