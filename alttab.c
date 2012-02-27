
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "wm.h"

struct alttab
{
    struct WM_t         *W;
    Window              win;
    int                 w, h, x, y;
    GC                  gc;
    unsigned long       inputeventmask;
    struct wmclient     *list[MAX_CLIENTS];
    XFontStruct         *font;
};

#if 0
static void draw_item_in_menu(struct WM_t *W, int xpos, int ypos, struct menu_item *I)
{
    int txtW, txtH, tY;
    char *txt = I->name;
    GC gc = XCreateGC(W->XDisplay, W->menu_win, 0, NULL);

    XSetFont(W->XDisplay, gc, W->font->fid);

    txtW = XTextWidth(W->font, txt, strlen(txt));
    txtH = W->font->ascent;

    tY = ypos + (W->item_height / 2) + (txtH / 2) - 1;

    XDrawString(W->XDisplay, W->menu_win, gc, xpos, tY, txt, strlen(txt));
    XFreeGC(W->XDisplay, gc);
}

static void draw_menu(struct WM_t *W)
{
    unsigned int bgcol;
    GC gc;
    Window win = W->menu_win;
    int i;

    bgcol = makeColourPixel(W, 0.8, 0.8, 0.8);

    printf("Root is %d x %d\n", W->rW, W->rH);

    gc = XCreateGC(W->XDisplay, win, 0, NULL);
    XSetForeground(W->XDisplay, gc, bgcol);
    XFillRectangle(W->XDisplay, win, gc, 0, 0, W->menuW, W->menuH);

    XSetForeground(W->XDisplay, gc, makeColourPixel(W, 0, 0, 0));
    XDrawLine(W->XDisplay, W->menu_win, gc, 300, 0, 300, W->menuH);

    for (i = 0; i < W->currMenu->n; i++)
    {
        draw_item_in_menu(W, 0, i * W->item_height, &(W->currMenu->items[i]));
    }
    XFlush(W->XDisplay);
}

static int menu_key_pressed(struct WM_t *W, XEvent *ev)
{
    switch (ev->xkey.keycode)
    {
        case KEY_WIN:
        case KEY_Q:
            /* Return 1 if we quit the menu */
            return 1;
    }
    return 0;
}

/* Hide the menu window */
static void hide_menu(struct WM_t *W)
{
    XUnmapWindow(W->XDisplay, W->menu_win);
}

/* Show the menu window */
static void show_menu(struct WM_t *W)
{
    XMapWindow(W->XDisplay, W->menu_win);
}

void do_menu(struct WM_t *W)
{
    XEvent ev;

    show_menu(W);

    while (1)
    {
        XNextEvent(W->XDisplay, &ev);
        switch (ev.type)
        {
            case KeyPress:
                /* Break the loop if the menu is exited */
                if (menu_key_pressed(W, &ev));
                    goto outside_loop;
                break;
            case Expose:
                draw_menu(W);
                break;
        }
    }
outside_loop:
    
    hide_menu(W);
}

static void open_AT_window(struct WM_t *W)
{
    Window win;

    load_menu_items(W);
    W->currMenu = &(W->rootMenu);

    /* Set the menu widths and heights */
    W->menuW = W->rW - 80;
    W->menuH = W->rH - 50;

    W->item_height = 20;

       W->menu_win = win;

    W->font = XLoadQueryFont(W->XDisplay, HELVETICA_12);
    assert(W->font);
}

#endif

static void draw_alttab(struct alttab *A)
{
    printf("Drawing alttab!\n");
}

static void alttab_events(struct alttab *A)
{
    XEvent ev;

    while (1)
    {
        XWindowEvent(A->W->XDisplay, A->win, A->inputeventmask, &ev);
        switch (ev.type)
        {
            case KeyPress:
                return;
                break;
            case Expose:
                draw_alttab(A);
                break;
        }
    }
}

static void load_font_open_window(struct alttab *A)
{
    struct WM_t *W = A->W;
    XCharStruct max_char;

    if (!W->font)
        W->font = XLoadQueryFont(W->XDisplay, ALT_TAB_FONTNAME);

    max_char = W->font->max_bounds;
    A->h = (max_char.ascent + max_char.descent + 2) * W->nclients;
    A->w = (max_char.rbearing - max_char.lbearing) * ALT_TAB_CHARACTERS;

    A->x = (W->rW - A->w - 2) / 2;
    A->y = (W->rH - A->h - 2) / 2;

    A->inputeventmask = KeyPressMask | ExposureMask;

    msg("Creating window at %d, %d, size %dx%d\n", A->x, A->y, A->w, A->h);

    /* Create the window */
    A->win = XCreateSimpleWindow(W->XDisplay, W->rootWindow,
                                 A->x, A->y, A->w, A->h, 1,
                                 BlackPixel(W->XDisplay, W->XScreen),
                                 WhitePixel(W->XDisplay, W->XScreen));
    /* Get events */
    XSelectInput(W->XDisplay, A->win, A->inputeventmask);

    A->gc = XCreateGC(W->XDisplay, A->win, 0, NULL);
 
    XMapWindow(W->XDisplay, A->win);
    XRaiseWindow(W->XDisplay, A->win);
    XSetInputFocus(W->XDisplay, A->win, RevertToPointerRoot, CurrentTime);
}

static struct wmclient *get_client_from_focus(struct WM_t *W, int f)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        struct wmclient *C = W->clients[i];
        if (C)
        {
            if (C->focus == f)
                return C;
        }
    }
    return NULL;
}

static void sort_by_focus_order(struct alttab *A)
{
    int f;
    for (f = 0; f < A->W->nclients; f++)
    {
        struct wmclient *C = get_client_from_focus(A->W, f);
        if (!C)
        {
            msg("???: Could not get window with focus %d - this is weird.\n", f);
            continue;
        }
        printf("    -> %s\n", C->name);
        A->list[f] = C;
    }
}

static void clean_up_alttab(struct alttab *A)
{
    XFreeGC(A->W->XDisplay, A->gc);
    XDestroyWindow(A->W->XDisplay, A->win);
    if (A->list[0])
        client_focus(A->W, A->list[0]);
    msg("Cleaned up.\n");
}

void do_alttab(struct WM_t *W)
{
    struct alttab A;
    A.W = W;

    sort_by_focus_order(&A);
    load_font_open_window(&A);
    alttab_events(&A);
    clean_up_alttab(&A);
}

