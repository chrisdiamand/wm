
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
    e.x = C->x;
    e.y = C->y;
    e.width = C->w;
    e.height = C->h;
    e.border_width = W->bsize;
    e.above = None;
    e.override_redirect = 0;
    XSendEvent(W->XDisplay, C->win, 0, StructureNotifyMask, (XEvent *)(&e));
    msg("configurenotify sent\n");
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
    XGrabKey(W->XDisplay, XKeysymToKeycode(W->XDisplay, sym),
             mods, C->win, 0, GrabModeAsync, GrabModeAsync);
}

void client_select_events(struct WM_t *W, struct wmclient *C)
{
    XSelectInput(W->XDisplay, C->win, StructureNotifyMask);

    /* Grab ALT+click events for moving windows */
    XGrabButton(W->XDisplay, Button1, Mod1Mask, C->win, 0,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync, GrabModeSync, None, None);
    /* Grab for any click so if it is clicked on it can be refocused */
    XGrabButton(W->XDisplay, Button1, 0, C->win, 1, ButtonPressMask,
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
        XSetWindowBorder(W->XDisplay, C->win, W->focus_border_colour);
    else
        XSetWindowBorder(W->XDisplay, C->win, W->black);
}

/* Set a 1-pixel border and position it at the size/pos in C */
static void set_size_pos_border(struct WM_t *W, struct wmclient *C)
{
    /* Add a border */
    XSetWindowBorderWidth(W->XDisplay, C->win, W->bsize);
    msg("Border added\n");
    /* Set the colour */

    if (W->clients[0] == C)
        set_border_colour(W, C, 1);
    else
        set_border_colour(W, C, 0);

    XMoveResizeWindow(W->XDisplay, C->win, C->x, C->y, C->w, C->h);
    msg("Resized and moved\n");
}

static int get_client_index(struct WM_t *W, Window id)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
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
    int oldidx = get_client_index(W, C->win);
    struct wmclient *old = W->clients[0];
    printf("Focus %s!\n", C->name);

    move_down_client_list(W, 0, oldidx);
    W->clients[0] = C;

    /* Unfocus the old window */
    /* Re-enable grabbing for click events */
    XGrabButton(W->XDisplay, Button1, 0, old->win, 1, ButtonPressMask,
                GrabModeAsync, GrabModeSync, None, None);
    /* Make the border boring */
    set_border_colour(W, old, 0);

    set_border_colour(W, C, 1);
    XRaiseWindow(W->XDisplay, C->win);
    XSetInputFocus(W->XDisplay, C->win, RevertToPointerRoot, CurrentTime);
    XUngrabButton(W->XDisplay, Button1, 0, C->win);

    print_clients(W);
}

/* Move and resize a window, saving the new dimensions. Negative sizes mean maximise */
void client_moveresize(struct WM_t *W, struct wmclient *C, int x, int y, int w, int h)
{
    if (C->fullscreen)
        return;

    if (w > W->rW || w < 0)
        w = W->rW - 2 * W->bsize;
    if (h > W->rH || h < 0)
        h = W->rH - 2 * W->bsize;

    C->x = x;
    C->y = y;
    C->w = w;
    C->h = h;

    XMoveResizeWindow(W->XDisplay, C->win, x, y, w, h);
}

static void maximise(struct WM_t *W, struct wmclient *C)
{
    msg("MAX: %s\n", C->name);
    C->fullscreen = 1;
    XSetWindowBorderWidth(W->XDisplay, C->win, 0);
    XMoveResizeWindow(W->XDisplay, C->win, 0, 0, W->rW, W->rH);
}

static void unmaximise(struct WM_t *W, struct wmclient *C)
{
    msg("MIN: %s\n", C->name);
    C->fullscreen = 0;
    set_size_pos_border(W, C);
}

void client_togglefullscreen(struct WM_t *W, struct wmclient *C)
{
    if (!C->fullscreen)
        maximise(W, C);
    else
        unmaximise(W, C);
}

/* Register a client, steal its border, grap events, etc */
void client_register(struct WM_t *W, Window xwindow_id)
{
    char *name;
    struct wmclient *C = malloc(sizeof(*C));

    XFetchName(W->XDisplay, xwindow_id, &name);
    if (!name)
        name = "<notitle>";
    msg("Registering \'%s\':\n", name);

    /* Duplicate the string in case the window closes but the string is still required */
    C->name = strdup(name);
    printf("name = %p, from x = %p\n", C->name, name);
    C->win = xwindow_id;

    decide_new_window_size_pos(W, C->win, &(C->x), &(C->y), &(C->w), &(C->h));

    C->fullscreen = 0;

    set_size_pos_border(W, C);

    client_select_events(W, C);
    msg("Buttons grabbed\n");

    send_ConfigureNotify(W, C);
    XMapWindow(W->XDisplay, C->win);
    msg("Mapped\n");

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

    assert(W->clients[idx] == C);

    /* Update all the focus numbers, i.e. decrease (bring forward) all the windows
       with bigger focus numbers. */
    for (i = idx; i < W->nclients; i++)
        W->clients[i] = W->clients[i + 1];
    W->nclients--;

    msg("Removing client \'%s\'\n", C->name);
    free(C->name);
    free(C);

    print_clients(W);

    client_focus(W, W->clients[0]);
}

