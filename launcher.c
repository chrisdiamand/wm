
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include "wm.h"

static int keysym_to_ascii(struct WM_t *W, KeySym sym)
{
    char buf[8];
    int overflow;
    XkbTranslateKeySym(W->XDisplay, &sym, 0, buf, sizeof(buf), &overflow);
    return buf[0];
}

static void draw_launcher(struct WM_t *W)
{
    struct launcher_t *L = &(W->launcher);
    int text_h = W->font->ascent + W->font->descent;
    int y = 2 * L->height / 3 + 1, cursorpos = 10 + XTextWidth(W->font, L->str, L->len);

    /* Draw a grey background */
    XSetForeground(W->XDisplay, L->gc, W->lightgrey);
    XFillRectangle(W->XDisplay, L->win, L->gc, 0, 0, W->rW, L->height);
    /* Draw the cursor */
    XSetForeground(W->XDisplay, L->gc, W->black);
    XFillRectangle(W->XDisplay, L->win, L->gc, cursorpos, y - W->font->ascent, 2, text_h);
    /*
    XSetForeground(W->XDisplay, L->gc, W->black);
    */

    XDrawString(W->XDisplay, L->win, L->gc, 10, y, L->str, L->len);
}

static void run(char *cmd)
{
    char exec[LAUNCHER_MAX_STRLEN + 3];
    if (strcmp(cmd, "logout") == 0)
    {
        msg("\"logout\" typed, exiting.\n");
        exit(0);
    }
    snprintf(exec, sizeof(exec), "%s &", cmd);
    system(exec);
}

static char *launcher_key_event(struct WM_t *W, XEvent *ev)
{
    struct launcher_t *L = &(W->launcher);
    KeySym sym = XkbKeycodeToKeysym(W->XDisplay, ev->xkey.keycode, 0, ev->xkey.state & ShiftMask);
    int ascii = keysym_to_ascii(W, sym);

    if (sym == XK_Return || ascii == '\n' || ascii == '\r')
    {
        run(L->str);
        return L->str;
    }
    else if (ascii == 27) /* Escape so cancel it */
    {
        return L->str;
    }
    else if (ascii == '\b')
    {
        if (L->len > 0)
        {
            L->str[--L->len] = '\0';
        }
    }
    else if (ascii && L->len < LAUNCHER_MAX_STRLEN - 1 && L->len >= 0)
    {
        L->str[L->len++] = ascii;
        L->str[L->len] = '\0';
    }
    return NULL;
}

static void launcher_events(struct WM_t *W)
{
    XEvent ev;
    while (1)
    {
        XMaskEvent(W->XDisplay, W->launcher.inputeventmask, &ev);
        switch (ev.type)
        {
            case KeyPress:
                if (launcher_key_event(W, &ev))
                    return;
                draw_launcher(W);
                break;
            case Expose:
                draw_launcher(W);
                break;
        }
    }
}

static void launcher_show(struct WM_t *W)
{
    struct launcher_t *L = &(W->launcher);
    XMapWindow(W->XDisplay, L->win);
    XRaiseWindow(W->XDisplay, L->win);
    XSetInputFocus(W->XDisplay, L->win, RevertToPointerRoot, CurrentTime);
}

static void launcher_hide(struct WM_t *W)
{
    struct launcher_t *L = &(W->launcher);
    XUnmapWindow(W->XDisplay, W->launcher.win);
    L->str[0] = '\0';
    L->len = 0;
}

void launcher(struct WM_t *W)
{
    printf("Launching!\n");
    launcher_show(W);
    launcher_events(W);
    launcher_hide(W);
}

void launcher_init(struct WM_t *W)
{
    struct launcher_t *L = &(W->launcher);
    L->height = 20;

    L->win = XCreateSimpleWindow(W->XDisplay, W->rootWindow, 0, 0, W->rW, L->height, 1, W->black, W->white);

    L->inputeventmask = KeyPressMask | KeymapStateMask | ExposureMask;
    XSelectInput(W->XDisplay, L->win, L->inputeventmask);

    L->gc = XCreateGC(W->XDisplay, L->win, 0, NULL);
    XSetFont(W->XDisplay, L->gc, W->font->fid);

    L->str[0] = '\0';
    L->len = 0;
}

