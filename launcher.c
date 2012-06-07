
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include "launcher.h"
#include "wm.h"

static int keyevent_to_ascii(struct WM_t *W, XKeyEvent *ev)
{
    char buf[8];
    int overflow;
    KeySym sym = XLookupKeysym(ev, (ev->state & ShiftMask) ? 1 : 0 );
    XkbTranslateKeySym(W->XDisplay, &sym, 0, buf, sizeof(buf), &overflow);
    return buf[0];
}

static void draw_launcher(struct WM_t *W)
{
    struct launcher_t *L = &(W->launcher);
    int text_h = L->font->ascent + L->font->descent;
    int y = 2 * L->height / 3 + 1, cursorpos = 10 + XTextWidth(L->font, L->str, L->len);

    /* Draw a grey background */
    XSetForeground(W->XDisplay, L->gc, W->bg_col);
    XFillRectangle(W->XDisplay, L->win, L->gc, 0, 0, W->rW, L->height);
    /* Draw the cursor */
    XSetForeground(W->XDisplay, L->gc, W->fg_col);
    XFillRectangle(W->XDisplay, L->win, L->gc, cursorpos, y - L->font->ascent, 2, text_h);

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
    int ascii = keyevent_to_ascii(W, &(ev->xkey));

    if (ascii == '\n' || ascii == '\r')
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
        printf("Event! %s\n", event_name(ev.type));
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

    L->win = XCreateSimpleWindow(W->XDisplay, W->rootWindow, 0, 0, W->rW, L->height,
                                 1, W->fg_col, W->bg_col);

    L->inputeventmask = KeyPressMask | KeymapStateMask | ExposureMask;
    XSelectInput(W->XDisplay, L->win, L->inputeventmask);

    L->gc = XCreateGC(W->XDisplay, L->win, 0, NULL);

    /* Load the font */
    L->font = XLoadQueryFont(W->XDisplay, W->prefs.launcher_font);
    if (!L->font)
    {
        msg("Couldn't load launcher font \'%s\', using \'fixed\' instead.\n", W->prefs.launcher_font);
        L->font = XLoadQueryFont(W->XDisplay, "fixed");
        assert(L->font);
    }

    XSetFont(W->XDisplay, L->gc, L->font->fid);

    L->str[0] = '\0';
    L->len = 0;
}

