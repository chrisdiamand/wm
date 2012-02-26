
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "policy.h"
#include "wm.h"

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

/* Add a new client to the list */
int client_insert(struct WM_t *W, struct wmclient *C)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (W->clients[i] == NULL)
        {
            W->clients[i] = C;
            msg("Client \'%s\' inserted at %d\n", C->name, i);
            return i;
        }
    }
    return -1;
}

void client_select_events(struct WM_t *W, struct wmclient *C)
{
    XSelectInput(W->XDisplay, C->win, StructureNotifyMask);

    /* Grab ALT+click events for moving windows */
    XGrabButton(W->XDisplay, Button1, Mod1Mask, C->win, 0,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync, GrabModeSync, None, None);
    /* Alt-Tab */
    XGrabKey(W->XDisplay, XKeysymToKeycode(W->XDisplay, XK_Tab),
             Mod1Mask, C->win, 0, GrabModeAsync, GrabModeAsync);
    /* Shift-alt-F for fullscreen */
    XGrabKey(W->XDisplay, XKeysymToKeycode(W->XDisplay, XK_f),
             ShiftMask | Mod1Mask, C->win, 0, GrabModeAsync, GrabModeAsync);
    /* Grab for any click so if it is clicked on it can be refocused */
    XGrabButton(W->XDisplay, Button1, 0, C->win, 1, ButtonPressMask,
                GrabModeAsync, GrabModeSync, None, None);
}

/* Set a 1-pixel border and position it at the size/pos in C */
static void set_size_pos_border(struct WM_t *W, struct wmclient *C)
{
    /* Add a border */
    XSetWindowBorderWidth(W->XDisplay, C->win, W->bsize);
    msg("Border added\n");
    /* Set the colour */
    XSetWindowBorder(W->XDisplay, C->win, BlackPixel(W->XDisplay, W->XScreen));
    msg("colour changed\n");

    XMoveResizeWindow(W->XDisplay, C->win, C->x, C->y, C->w, C->h);
    msg("Resized and moved\n");
}

void client_focus(struct WM_t *W, struct wmclient *C)
{
    int i;
    printf("Focus %s!\n", C->name);
    /* Go through all the clients and decrease their focus number.
       0 is the window in focus */
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        struct wmclient *D = W->clients[i];
        if (D)
        {
            /* If it was the old focus it has its click-grab turned off; reenable it */
            if (D->focus == 0)
                XGrabButton(W->XDisplay, Button1, 0, D->win, 1, ButtonPressMask,
                            GrabModeAsync, GrabModeSync, None, None);
            if (D->focus > C->focus)
                D->focus--;
        }
    }
    C->focus = 0;
    XRaiseWindow(W->XDisplay, C->win);
    XSetInputFocus(W->XDisplay, C->win, RevertToPointerRoot, CurrentTime);
    XUngrabButton(W->XDisplay, Button1, 0, C->win);
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

    client_focus(W, C);

    XFlush(W->XDisplay);

    client_insert(W, C);
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
            {
                return i;
            }
        }
    }
    return -1;
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
    int i = get_client_index(W, C->win);

    assert(W->clients[i] == C);

    msg("Removing client \'%s\'\n", C->name);
    free(C->name);
    free(C);
    W->clients[i] = NULL;
}

