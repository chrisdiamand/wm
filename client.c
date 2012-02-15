
#include <stdio.h>
#include <stdlib.h>

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
    printf("configurenotify sent\n");
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
            printf("Client %s inserted at %d\n", C->name, i);
            return i;
        }
    }
    return -1;
}

/* Register a client, steal its border, grap events, etc */
void client_register(struct WM_t *W, Window xwindow_id)
{
    char *name;
    struct wmclient *C = malloc(sizeof(*C));

    XFetchName(W->XDisplay, xwindow_id, &name);
    if (!name)
        name = "--";
    printf("Registering \'%s\':\n", name);

    C->name = name;
    C->win = xwindow_id;

    decide_new_window_size_pos(W, C->win, &(C->x), &(C->y), &(C->w), &(C->h));

    printf("decided size\n");

    XResizeWindow(W->XDisplay, C->win, C->w, C->h);
    printf("Resized\n");

    /* Add a border */
    XSetWindowBorderWidth(W->XDisplay, C->win, W->bsize);
    printf("Border added\n");
    /* Set the colour */
    XSetWindowBorder(W->XDisplay, C->win, BlackPixel(W->XDisplay, W->XScreen));
    printf("colour changed\n");


    /* Grab ALT+click events for moving windows */
    XGrabButton(W->XDisplay, Button1, Mod1Mask, C->win, 0,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync, GrabModeSync, None, None);

    printf("Buttons grabbed\n");

    send_ConfigureNotify(W, C);
    XMapWindow(W->XDisplay, C->win);
    printf("Mapped\n");

    XFlush(W->XDisplay);

    client_insert(W, C);
}

/* Find a wmclient structure from its window ID */
struct wmclient *client_from_window(struct WM_t *W, Window id)
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
                return C;
            }
        }
    }
    return NULL;
}
