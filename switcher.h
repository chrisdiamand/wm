
#ifndef ALTTAB_H
#define ALTTAB_H

struct WM_t;

struct switcher_t 
{
    Window              win;
    int                 w, h, x, y;
    int                 item_height;
    GC                  gc;
    unsigned long       inputeventmask;
    /* Index of currently selected window */
    int                 selected;

    XFontStruct         *font;
};

void switcher(struct WM_t *);
void switcher_init(struct WM_t *);

#endif

