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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "policy.h"
#include "wm.h"

static void print_clients(struct WM_t *W)
{
    int i;
    msg("%d clients:\n", W->nclients);
    for (i = 0; i < W->nclients; i++)
        msg("    -> \'%s\'\n", W->clients[i]->name);
}

static void send_ConfigureNotify(struct WM_t *W, struct wmclient *C)
{
    XConfigureEvent e;

    e.type = ConfigureNotify;
    e.display = W->XDisplay;
    e.event = C->win;
    e.window = C->win;
    e.above = None;
    e.override_redirect = 0;

    if (!C->fullscreen)
    {
        e.x = C->x;
        e.y = C->y;
        e.width = C->w;
        e.height = C->h;
        e.border_width = W->prefs.bw;
    }
    else
    {
        e.x = 0;
        e.y = 0;
        e.width = W->rW;
        e.height = W->rH;
        e.border_width = 0;
    }
    XSendEvent(W->XDisplay, C->win, 0, StructureNotifyMask, (XEvent *)(&e));
}

/* Move all clients with focus position between start end down the list,
   overwriting the one at end */
static void move_down_client_list(struct WM_t *W, int start, int end)
{
    int i;
    for (i = end - 1; i >= start; i--)
        W->clients[i + 1] = W->clients[i];
    W->clients[start] = NULL;
}

static void grabkey(struct WM_t *W, struct wmclient *C, KeySym sym, unsigned int mods)
{
    KeyCode code = XKeysymToKeycode(W->XDisplay, sym);
    XGrabKey(W->XDisplay, code, mods, C->win,
             0, GrabModeAsync, GrabModeAsync);
    XGrabKey(W->XDisplay, code, mods | LockMask, C->win,
             0, GrabModeAsync, GrabModeAsync);
}

void client_select_events(struct WM_t *W, struct wmclient *C)
{
    XSelectInput(W->XDisplay, C->win, StructureNotifyMask);

    /* Grab ALT+click events for moving windows */
    XGrabButton(W->XDisplay, Button1, Mod1Mask, C->win, 0,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync, GrabModeSync, None, None);
    /* Grab Shift+alt+click events for moving windows */
    XGrabButton(W->XDisplay, Button1, Mod1Mask | ShiftMask, C->win, 0,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync, GrabModeSync, None, None);
    /* Grab for any click so if it is clicked on it can be refocused */
    XGrabButton(W->XDisplay, Button1, 0, C->win, 0, ButtonPressMask,
                GrabModeAsync, GrabModeSync, None, None);

    /* Alt-Tab */
    grabkey(W, C, XK_Tab, Mod1Mask);
    /* Shift-alt-enter for menu */
    grabkey(W, C, XK_Return, ShiftMask | Mod1Mask);
    /* Shift-alt-F for fullscreen */
    grabkey(W, C, XK_f, ShiftMask | Mod1Mask);
    /* Shift-alt-arrows for tiling */
    grabkey(W, C, XK_Up, ShiftMask | Mod1Mask);
    grabkey(W, C, XK_Down, ShiftMask | Mod1Mask);
    grabkey(W, C, XK_Left, ShiftMask | Mod1Mask);
    grabkey(W, C, XK_Right, ShiftMask | Mod1Mask);
}

static void set_border_colour(struct WM_t *W, struct wmclient *C, int focus)
{
    if (focus)
        XSetWindowBorder(W->XDisplay, C->win, W->focus_border_col);
    else
        XSetWindowBorder(W->XDisplay, C->win, W->unfocus_border_col);
}

/* Set a 1-pixel border and position it at the size/pos in C */
static void set_size_pos_border(struct WM_t *W, struct wmclient *C)
{
    /* Add a border */
    XSetWindowBorderWidth(W->XDisplay, C->win, W->prefs.bw);

    /* Set the colour differently if it's at the top of the focus stack */
    if (W->clients[0] == C)
        set_border_colour(W, C, 1);
    else
        set_border_colour(W, C, 0);

    XMoveResizeWindow(W->XDisplay, C->win, C->x, C->y, C->w, C->h);
    
    /* XChangeProperty? */
}

static int get_client_index(struct WM_t *W, Window id)
{
    int i;
    for (i = 0; i < W->nclients; i++)
    {
        struct wmclient *C = W->clients[i];
        /* Don't test null entries */
        if (C != NULL)
        {
            if (C->win == id)
                return i;
        }
    }
    return -1;
}

void client_focus(struct WM_t *W, struct wmclient *C)
{
    int oldidx;
    struct wmclient *old;

    /* Don't do anything if there's nothing to focus */
    if (W->nclients == 0)
        return;

    oldidx = get_client_index(W, C->win);
    old = W->clients[0];

    move_down_client_list(W, 0, oldidx);
    W->clients[0] = C;

    /* Unfocus the old window */
    /* Re-enable grabbing for click events */
    XGrabButton(W->XDisplay, Button1, 0, old->win, 0, ButtonPressMask,
                GrabModeAsync, GrabModeSync, None, None);
    /* Make the border boring */
    set_border_colour(W, old, 0);

    set_border_colour(W, C, 1);
    XRaiseWindow(W->XDisplay, C->win);
    XSetInputFocus(W->XDisplay, C->win, RevertToPointerRoot, CurrentTime);
    XUngrabButton(W->XDisplay, Button1, 0, C->win);
}

/* Move and resize a window, saving the new dimensions. Negative sizes mean
 * maximise but still with a border */
void client_moveresize(struct WM_t *W, struct wmclient *C, int x, int y, int w, int h)
{
    if (C->fullscreen)
    {
        C->fullscreen = 0;
        set_size_pos_border(W, C);
    }

    if (w > W->rW || w < 0)
        w = W->rW - 2 * W->prefs.bw;
    if (h > W->rH || h < 0)
        h = W->rH - 2 * W->prefs.bw;

    C->x = x;
    C->y = y;
    C->w = w;
    C->h = h;

    XMoveResizeWindow(W->XDisplay, C->win, x, y, w, h);
}

static void maximise(struct WM_t *W, struct wmclient *C)
{
    msg("client_MAX: %s\n", C->name);
    C->fullscreen = 1;
    XSetWindowBorderWidth(W->XDisplay, C->win, 0);
    XMoveResizeWindow(W->XDisplay, C->win, 0, 0, W->rW, W->rH);

    send_ConfigureNotify(W, C);
}

static void unmaximise(struct WM_t *W, struct wmclient *C)
{
    msg("client_unMAX: %s\n", C->name);
    C->fullscreen = 0;
    set_size_pos_border(W, C);

    send_ConfigureNotify(W, C);
}

void client_togglefullscreen(struct WM_t *W, struct wmclient *C)
{
    if (!C->fullscreen)
        maximise(W, C);
    else
        unmaximise(W, C);
}

static void get_pid(struct WM_t *W, struct wmclient *C)
{
    Atom NetWM_PID = XInternAtom(W->XDisplay, "_NET_WM_PID", 0);
    Atom actual_type_return;
    int actual_format_return;
    unsigned long nitems_return, bytes_after_return;
    unsigned char *prop_return;

    if (NetWM_PID == None)
    {
        msg("get_pid(): Couldn't get _NET_WM_PID atom\n");
        C->pid = 0;
        return;
    }
    XGetWindowProperty(W->XDisplay, C->win, NetWM_PID, /* Get the _NET_WM_PID property */
                       0,               /* Data offset 0 */
                       1,               /* Length 1x32 bits */
                       0,               /* Don't delete it afterwards */
                       AnyPropertyType, /* Atom reg_type, whatever that is */
                       &actual_type_return, &actual_format_return,
                       &nitems_return, &bytes_after_return,
                       &prop_return);   /* The actual data */

    if (nitems_return > 0 && prop_return)
    {
        C->pid = *( (int *) prop_return );
        XFree(prop_return);
    }
    else /* It doesn't have a PID property */
        C->pid = 0;
}

/* Register a client, steal its border, grap events, etc */
void client_register(struct WM_t *W, Window xwindow_id)
{
    struct wmclient *C = malloc(sizeof(*C));

    XFetchName(W->XDisplay, xwindow_id, &(C->name));
    if (!(C->name))
        C->name = "<notitle>";
    msg("Registering \'%s\':\n", C->name);

    C->win = xwindow_id;

    decide_new_window_size_pos(W, C);

    C->fullscreen = 0;

    set_size_pos_border(W, C);

    client_select_events(W, C);

    send_ConfigureNotify(W, C);
    XMapWindow(W->XDisplay, C->win);

    get_pid(W, C);

    W->clients[W->nclients++] = C;

    client_focus(W, C);

    XFlush(W->XDisplay);

    print_clients(W);
}

/* Find a wmclient structure from its window ID */
struct wmclient *client_from_window(struct WM_t *W, Window id)
{
    int idx = get_client_index(W, id);
    if (idx >= 0)
        return W->clients[idx];
    return NULL;
}

void client_remove(struct WM_t *W, struct wmclient *C)
{
    int idx = get_client_index(W, C->win), i;

    if (W->clients[idx] != C)
    {
        msg("client_remove: somehow get_client_index failed! :o\n");
        return;
    }

    W->clients[idx] = NULL;
    /* Update all the focus numbers, i.e. decrease (bring forward) all the windows
       with bigger focus numbers. */
    for (i = idx; i < W->nclients; i++)
        W->clients[i] = W->clients[i + 1];
    W->nclients--;

    msg("Removing client \'%s\'\n", C->name);
    free(C->name);
    free(C);

    msg("About to print\n");
    print_clients(W);
    msg("Printed, about to focus\n");

    client_focus(W, W->clients[0]);
    msg("Focused\n");
}

