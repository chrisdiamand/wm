
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "switcher.h"
#include "wm.h"

#define AT_BORDER 5

static void draw_item_text(struct WM_t *W, char *text, int topcorner)
{
    struct switcher_t *A = &(W->AT);
    int text_w = XTextWidth(A->font, text, strlen(text));
    int text_h = A->font->ascent + A->font->descent;
    int x, y;

    if (text_w < A->w) /* Centre it if it will fit */
        x = (A->w - text_w) / 2;
    else
        x = AT_BORDER + 4; /* 4 is an arbitrary gap before the start of the text */

    y = topcorner + (A->item_height + text_h) / 2 - A->font->descent - 1;
    XDrawString(W->XDisplay, A->win, A->gc, x, y, text, strlen(text));

}

static void draw_switcher(struct WM_t *W)
{
    struct switcher_t *A = &(W->AT);
    int i;

    /* Draw a grey background */
    XSetForeground(W->XDisplay, A->gc, W->bg_col);
    XFillRectangle(W->XDisplay, A->win, A->gc, 0, 0, A->w, A->h);
    XSetForeground(W->XDisplay, A->gc, W->fg_col);

    printf("fg = %lu, bg = %lu\n", W->fg_col, W->bg_col);

    for (i = 0; i < W->nclients; i++)
    {
        if (i == A->selected)
        {
            /* Swap the background and foreground colours for the selected one */
            XSetForeground(W->XDisplay, A->gc, W->fg_col);
            XFillRectangle(W->XDisplay, A->win, A->gc,
                           AT_BORDER, A->item_height * i + AT_BORDER,
                           A->w - 2 * AT_BORDER, A->item_height);
            XSetForeground(W->XDisplay, A->gc, W->bg_col);
        }
        draw_item_text(W, W->clients[i]->name, W->AT.item_height * i + AT_BORDER);
        /* Restore fg_col text as the next one drawn will be unselected */
        if (i == A->selected)
            XSetForeground(W->XDisplay, A->gc, W->fg_col);
    }
}

static int switcher_key_event(struct WM_t *W, XEvent *ev)
{
    struct switcher_t *A = &(W->AT);
    KeySym sym = XLookupKeysym(&(ev->xkey), 0);

    /* Alt released so focus the selected window and quit the switcher */
    if (!(ev->xkey.state & Mod1Mask) ||
        (ev->type == KeyRelease && sym == XK_Alt_L))
    {
        msg("Alt released.\n");
        client_focus(W, W->clients[W->AT.selected]);
        return -1;
    }

    switch (sym)
    {
        case XK_Tab:
            msg("********* Tab!\n");
            if (ev->type == KeyPress)
            {
                if (ev->xkey.state & ShiftMask)
                    A->selected--;
                else
                    A->selected++;

                if (A->selected >= W->nclients)
                    A->selected -= W->nclients;
                if (A->selected < 0)
                    A->selected = W->nclients - 1;

                msg("Tab: client %d\n", W->AT.selected);
                draw_switcher(W);
            }
            break;
        /* If escape pressed don't change the focus */
        case XK_Escape:
            msg("Escape!\n");
            return -1;
    }
    return 0;
}

static void switcher_events(struct WM_t *W)
{
    XEvent ev;

    while (1)
    {
        XMaskEvent(W->XDisplay, W->AT.inputeventmask, &ev);
        printf("Event: %s, window %lu\n", event_name(ev.type), ev.xany.window);
        switch (ev.type)
        {
            case KeyPress:
            case KeyRelease:
                if (switcher_key_event(W, &ev) == -1)
                    return;
                break;
            case Expose:
                draw_switcher(W);
                break;
        }
    }
}

static void switcher_show(struct WM_t *W)
{
    XCharStruct max_char;
    struct switcher_t *A = &(W->AT);

    assert(A->font);

    /* Calculate the window size */
    max_char = A->font->max_bounds;
    A->item_height = (max_char.ascent + max_char.descent) * 1.5;
    A->h = A->item_height * W->nclients + 2 * AT_BORDER;
    A->w = (max_char.rbearing - max_char.lbearing) * W->prefs.switcher_char_width + 2 * AT_BORDER;

    A->x = (W->rW - A->w - 2) / 2 - AT_BORDER;
    A->y = (W->rH - A->h - 2) / 2 - AT_BORDER;

    XMoveResizeWindow(W->XDisplay, A->win, A->x, A->y, A->w, A->h);
    XMapWindow(W->XDisplay, A->win);
    XRaiseWindow(W->XDisplay, A->win);
    XSetInputFocus(W->XDisplay, A->win, RevertToPointerRoot, CurrentTime);
}

static void switcher_hide(struct WM_t *W)
{
    if (W->clients[0])
        client_focus(W, W->clients[0]);
    XUnmapWindow(W->XDisplay, W->AT.win);
}

void switcher(struct WM_t *W)
{
    if (W->nclients > 1)
        W->AT.selected = 1;
    else
        W->AT.selected = 0;

    switcher_show(W);
    switcher_events(W);
    switcher_hide(W);
}

void switcher_init(struct WM_t *W)
{
    struct switcher_t *A = &(W->AT);
    /* The window will be resized every time it is shown so these are just to
     * let it be created */
    A->x = 1;   A->y = 1;   A->w = 10;   A->h = 10;
    A->inputeventmask = KeyPressMask | KeyReleaseMask | KeymapStateMask | ExposureMask;

    msg("Creating window at %d, %d, size %dx%d\n", A->x, A->y, A->w, A->h);

    /* Create the window */
    A->win = XCreateSimpleWindow(W->XDisplay, W->rootWindow,
                                 A->x, A->y, A->w, A->h, 1,
                                 BlackPixel(W->XDisplay, W->XScreen),
                                 WhitePixel(W->XDisplay, W->XScreen));
    /* Get events */
    XSelectInput(W->XDisplay, A->win, A->inputeventmask);

    /* Load the font */
    A->font = XLoadQueryFont(W->XDisplay, W->prefs.switcher_font);
    if (!A->font)
    {
        msg("Couldn't load font \'%s\', using \'fixed\' instead.\n", W->prefs.switcher_font);
        A->font = XLoadQueryFont(W->XDisplay, "fixed");
        assert(A->font);
    }

    A->gc = XCreateGC(W->XDisplay, A->win, 0, NULL);
    XSetFont(W->XDisplay, A->gc, A->font->fid);
}

