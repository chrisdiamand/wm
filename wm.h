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

#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_CLIENTS 64

#include "launcher.h"
#include "switcher.h"
#include "wmprefs.h"

struct wmclient
{
    char                *name;
    int                 pid;
    Window              win;
    int                 x, y, w, h;
    int                 min_w, min_h;
    int                 fullscreen;
};

/* Hold state info about the WM */
struct WM_t
{
    /* X stuff */
    Display             *XDisplay;
    int                 XScreen;
    Window              rootWindow;
    GC                  rootGC;

    Cursor              cursor_normal, cursor_move, cursor_resize;

    /* Root window width and height */
    unsigned int        rW, rH;

    /* List of open windows/programs, sorted in focus order. */
    struct wmclient     *clients[MAX_CLIENTS];
    int                 nclients;

    /* Info about the Alt-Tab switcher window */
    struct launcher_t   *launcher;

    /* Colours */
    unsigned long       focus_border_col, unfocus_border_col;
    unsigned long       fg_col, bg_col, black, root_bg_col;

    struct wmprefs_t    prefs;
};

void do_alttab(struct WM_t *);
void redraw_root(struct WM_t *, XEvent *);

/* Functions from client.c */
int client_insert(struct WM_t *, struct wmclient *);
void client_register(struct WM_t *, Window);
void client_select_events(struct WM_t *, struct wmclient *);
struct wmclient *client_from_window(struct WM_t *, Window);
void client_togglefullscreen(struct WM_t *, struct wmclient *);
void client_moveresize(struct WM_t *, struct wmclient *, int, int, int, int);
void client_focus(struct WM_t *, struct wmclient *);
void client_remove(struct WM_t *, struct wmclient *);

/* Functions from event.c */
char *event_name(int);
void event_loop(struct WM_t *);

/* Debugging function */
int msg(char *, ...);

#endif

