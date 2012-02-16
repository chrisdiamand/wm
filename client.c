
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
    XSelectInput(W->XDisplay, C->win, StructureNotifyMask | LeaveWindowMask);
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

    msg("decided size\n");

    XResizeWindow(W->XDisplay, C->win, C->w, C->h);
    msg("Resized\n");

    /* Add a border */
    XSetWindowBorderWidth(W->XDisplay, C->win, W->bsize);
    msg("Border added\n");
    /* Set the colour */
    XSetWindowBorder(W->XDisplay, C->win, BlackPixel(W->XDisplay, W->XScreen));
    msg("colour changed\n");


    /* Grab ALT+click events for moving windows */
    XGrabButton(W->XDisplay, Button1, Mod1Mask, C->win, 0,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync, GrabModeSync, None, None);

    client_select_events(W, C);
    msg("Buttons grabbed\n");

    send_ConfigureNotify(W, C);
    XMapWindow(W->XDisplay, C->win);
    msg("Mapped\n");

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

