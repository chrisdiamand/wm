/*
 * Copyright (c) 2012 Chris Diamand
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#include "launcher.h"
#include "list.h"
#include "menuitems.h"
#include "read_desktop_file.h"
#include "selectbox.h"
#include "wm.h"

static Window launcher_win;

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
    struct launcher_t *L = W->launcher;
    int text_h = L->font->ascent + L->font->descent;
    int y = 2 * L->height / 3 + 1, cursorpos = 10 + XTextWidth(L->font, L->str, L->len);

    /* Draw a grey background */
    XSetForeground(W->XDisplay, L->gc, W->bg_col);
    XFillRectangle(W->XDisplay, launcher_win, L->gc, 0, 0, curr_width(W), L->height);
    /* Draw the cursor */
    XSetForeground(W->XDisplay, L->gc, W->fg_col);
    XFillRectangle(W->XDisplay, launcher_win, L->gc, cursorpos, y - L->font->ascent, 2, text_h);

    XDrawString(W->XDisplay, launcher_win, L->gc, 10, y, L->str, L->len);

    if (L->sb)
        selectbox_draw(L->sb, L->selected);
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

/* Take a List * of menuitem_t * and turn all the names into
 * an array of char * */
static char **menuitem_list_to_namelist(struct List *L)
{
    struct menuitem_t *M;
    char **ret;
    int i;

    if (!L->size || !L->items)
        return NULL;

    ret = (char **) malloc(L->size * sizeof(char *));
    for (i = 0; i < L->size; i++)
    {
        M = L->items[i];
        if (M->descr)
        {
            ret[i] = malloc(strlen(M->name) + 4 + strlen(M->descr));
            sprintf(ret[i], "%s - %s", M->name, M->descr);
        }
        else
        {
            ret[i] = strdup(M->name);
        }
    }
    return ret;
}

static char *launcher_key_event(struct WM_t *W, XEvent *ev)
{
    struct launcher_t *L = W->launcher;
    int ascii = keyevent_to_ascii(W, &(ev->xkey));
    KeySym sym = XLookupKeysym(&(ev->xkey), 0);

    struct selectbox_t *new_sb = NULL;
    struct List *new_suggestions = NULL;

    if ((ascii == '\n' || ascii == '\r') && L->suggestions)
    {
        if (L->selected < L->suggestions->size)
        {
            struct menuitem_t *M = L->suggestions->items[L->selected];
            run(M->exec);
            return L->str;
        }
    }
    else if (ascii == 27) /* Escape so cancel it */
    {
        return L->str;
    }
    else if (sym == XK_Down && L->suggestions)
    {
        L->selected++;
        if (L->selected >= L->suggestions->size)
            L->selected = 0;
        return NULL;
    }
    else if (sym == XK_Up && L->suggestions)
    {
        L->selected--;
        if (L->selected < 0)
            L->selected = L->suggestions->size - 1;
        return NULL;
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

    /* If we haven't returned by now, update the suggestions menu:
     * First create a new window */
    if (L->len > 0 && L->str)
    {
        char **new_items;
        new_suggestions = menuitems_match(L->str);
        new_items = menuitem_list_to_namelist(new_suggestions);
        new_sb = selectbox_new(W, curr_head_x(W), curr_head_y(W) + L->height + 1, 350,
                               0, new_items, new_suggestions->size, L->font); 
    }

    /* Get rid of the old one */
    if (L->sb)
    {
        int i;
        for (i = 0; i < L->sb->n_items; i++)
            free(L->sb->items[i]);
        free(L->sb->items);
        selectbox_close(L->sb);
    }
    if (L->suggestions)
        List_free(L->suggestions);

    L->sb = new_sb;
    L->suggestions = new_suggestions;
    L->selected = 0;

    return NULL;
}

static void launcher_events(struct WM_t *W)
{
    XEvent ev;
    while (1)
    {
        XMaskEvent(W->XDisplay, W->launcher->inputeventmask, &ev);
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
    struct launcher_t *L = W->launcher;

    launcher_win = XCreateSimpleWindow(W->XDisplay, W->rootWindow, curr_head_x(W), curr_head_y(W),
                                       curr_width(W) - 2, L->height, 1, W->fg_col, W->bg_col);

    XSelectInput(W->XDisplay, launcher_win, L->inputeventmask);

    L->gc = XCreateGC(W->XDisplay, launcher_win, 0, NULL);

    XSetFont(W->XDisplay, L->gc, L->font->fid);

    XMapWindow(W->XDisplay, launcher_win);
    XRaiseWindow(W->XDisplay, launcher_win);
    XSetInputFocus(W->XDisplay, launcher_win, RevertToPointerRoot, CurrentTime);

    L->str[0] = '\0';
    L->len = 0;
}

static void launcher_hide(struct WM_t *W)
{
    struct launcher_t *L = W->launcher;

    if (L->sb)
    {
        free(L->sb->items);
        selectbox_close(L->sb);
        L->sb = NULL;
    }

    XUnmapWindow(W->XDisplay, launcher_win);
    XFreeGC(W->XDisplay, L->gc);
    XDestroyWindow(W->XDisplay, launcher_win);
    launcher_win = 0;
}

void launcher(struct WM_t *W)
{
    struct launcher_t *L = W->launcher;
    printf("Launching!\n");

    L->sb = NULL;
    L->suggestions = NULL;

    launcher_show(W);
    launcher_events(W);
    launcher_hide(W);
}

void launcher_init(struct WM_t *W)
{
    struct launcher_t *L;
    L = W->launcher = malloc(sizeof(struct launcher_t));

    /* Scan for .desktop files */
    msg("Loading .desktop files\n");
    menuitems_scan(W);
    msg("Done.\n");

    L->height = 20;

    L->inputeventmask = KeyPressMask | KeymapStateMask | ExposureMask;

        /* Load the font */
    L->font = XLoadQueryFont(W->XDisplay, W->prefs.launcher_font);
    if (!L->font)
    {
        msg("Couldn't load launcher font \'%s\', using \'fixed\' instead.\n", W->prefs.launcher_font);
        L->font = XLoadQueryFont(W->XDisplay, "fixed");
        assert(L->font);
    }
}

