
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
    GC                  gc;
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

static struct menu_item alloc_item(char *name, char *cmd)
{
    struct menu_item i;
    i.name = name;
    i.command = cmd;
    i.sub = NULL;
    return i;
}

static void load_menu_items(struct WM_t *W)
{
    W->rootMenu.n = 3;
    W->rootMenu.prev = NULL;
    W->rootMenu.items = malloc(sizeof(struct menu_item) * W->rootMenu.n);
    W->rootMenu.items[0] = alloc_item("xterm", "xterm");
    W->rootMenu.items[1] = alloc_item("konsole", "konsole");
    W->rootMenu.items[2] = alloc_item("Chrome", "google-chrome");
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

    /* Create the window */
    win = XCreateSimpleWindow(W->XDisplay, W->rootWindow,
                              0, 0, W->menuW, W->menuH, 0,
                              BlackPixel(W->XDisplay, W->XScreen),
                              WhitePixel(W->XDisplay, W->XScreen));
    /* Get events */
    XSelectInput(W->XDisplay, win, KeyPressMask | ExposureMask);

    W->menu_win = win;

    W->font = XLoadQueryFont(W->XDisplay, HELVETICA_12);
    assert(W->font);
}

#endif

static void load_font_open_window(struct alttab *A)
{
    ;
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

void do_alttab(struct WM_t *W)
{
    struct alttab A;
    A.W = W;

    sort_by_focus_order(&A);
    load_font_open_window(&A);
}

