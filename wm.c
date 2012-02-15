
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "policy.h"
#include "wm.h"

struct WM_t *wm_state_for_quit;

int makeColourPixel(struct WM_t *W, double r, double g, double b)
{
    XColor c;
    c.red = r * 65535.0;
    c.green = g * 65535.0;
    c.blue = b * 65535.0;
    XAllocColor(W->XDisplay, DefaultColormap(W->XDisplay, DefaultScreen(W->XDisplay)), &c);
    return c.pixel;
}

static void redraw_root(struct WM_t *W, XEvent *ev)
{
    int bgcol;
    int x, y, w, h;
    GC gc = XCreateGC(W->XDisplay, W->rootWindow, 0, NULL);

    bgcol = makeColourPixel(W, 0.7, 0.7, 0.8);

    XSetForeground(W->XDisplay, gc, bgcol);

    if (ev)
    {
        x = ev->xexpose.x;
        y = ev->xexpose.y;
        w = ev->xexpose.width;
        h = ev->xexpose.height;
    }
    else
    {
        x = 0;
        y = 0;
        w = W->rW;
        h = W->rH;
    }

    XFillRectangle(W->XDisplay, W->rootWindow, gc, x, y, w, h);
}

static void open_display(struct WM_t *W)
{
    Window root, tmpwin;
    int tmpx, tmpy;
    unsigned int tmpbw, tmpdepth;

    W->XDisplay = XOpenDisplay(NULL);
    if (!W->XDisplay)
    {
        fprintf(stderr, "Cannot open X display!\n");
        exit(1);
    }

    W->XScreen = DefaultScreen(W->XDisplay);

    root = RootWindow(W->XDisplay, DefaultScreen(W->XDisplay));
    

    W->rootWindow = root;
#if 0
    /* Make a pretend root window to avoid lots of logging in and out */
    W->rootWindow = XCreateSimpleWindow(W->XDisplay, root,
                                        100, 100, 640, 240, 1,
                                        BlackPixel(W->XDisplay, W->XScreen),
                                        WhitePixel(W->XDisplay, W->XScreen));

    XMapWindow(W->XDisplay, W->rootWindow);
#endif

    printf("Open...\n");

    XSelectInput(W->XDisplay, W->rootWindow, KeyPressMask             |
                                             KeyReleaseMask           |
                                             EnterWindowMask          |
                                             FocusChangeMask          |
                                             SubstructureRedirectMask |
                                             ButtonPressMask          |
                                             ButtonReleaseMask        |
                                             ExposureMask);
    


    printf("did select input\n");
    /* Find out the geometry of the root window */
    XGetGeometry(W->XDisplay, W->rootWindow, &tmpwin,
                 &tmpx, &tmpy,
                 &(W->rW), &(W->rH),
                 &tmpbw, &tmpdepth);
    printf("got geometry\n");

    redraw_root(W, NULL);
}

static void key_pressed(struct WM_t *W, XEvent *ev)
{
    switch (ev->xkey.keycode)
    {
        case KEY_ALT:
            printf("Alt!\n");
            break;
        case KEY_WIN:
            printf("Win key\n");
            system("dmenu_run &");
            break;
    }
}

static int insert_client(struct WM_t *W, struct wmclient *C)
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

static void register_client(struct WM_t *W, Window xwindow_id)
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

    XResizeWindow(W->XDisplay, C->win, C->w, C->h);

    /* Add a border */
    XSetWindowBorderWidth(W->XDisplay, C->win, W->bsize);
    /* Set the colour */
    XSetWindowBorder(W->XDisplay, C->win, BlackPixel(W->XDisplay, W->XScreen));


    /* Grab ALT+click events for moving windows */
    XGrabButton(W->XDisplay, Button1, Mod1Mask, C->win, 0,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
                GrabModeAsync, GrabModeSync, None, None);

    XMapWindow(W->XDisplay, C->win);

    XFlush(W->XDisplay);

    insert_client(W, C);
}

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

static void expose_event(struct WM_t *W, XEvent *ev)
{
    if (ev->xexpose.window == W->rootWindow)
        redraw_root(W, ev);
}

static void move_client_window(struct WM_t *W, struct wmclient *C, int xOff, int yOff)
{
    XEvent ev;

    printf("Moving.\n");
    XSelectInput(W->XDisplay, C->win, ButtonReleaseMask  |
                                      ButtonMotionMask   |
                                      KeyReleaseMask     |
                                      PointerMotionMask);

    XRaiseWindow(W->XDisplay, C->win);

    while (1)
    {
        XNextEvent(W->XDisplay, &ev);
        switch (ev.type)
        {
            case MotionNotify:
                XMoveWindow(W->XDisplay, C->win,
                            ev.xmotion.x_root - xOff, ev.xmotion.y_root - yOff);
                break;
            case Expose:
                expose_event(W, &ev);
                break;
            case KeyRelease:
            case ButtonRelease:
            default:
                printf("done moving. Last event was %d\n", ev.type);
                XSelectInput(W->XDisplay, C->win, 0);
                return;
        }
    }
}

static void event_loop(struct WM_t *W)
{
    XEvent ev;
    struct wmclient *C = NULL;
    while (1)
    {
        XNextEvent(W->XDisplay, &ev);

        C = client_from_window(W, ev.xany.window);

        switch (ev.type)
        {
            case ButtonPress:
                /* ALT+click to move a window */
                if ((ev.xbutton.state & (Button1Mask | Mod1Mask)) && C)
                    move_client_window(W, C, ev.xbutton.x, ev.xbutton.y);
                break;
            case MapRequest: /* Does not use CreateNotify */
                printf("Map request\n");
                if (!C) /* Don't register it again if it was just hiding for some reason */
                    register_client(W, ev.xmaprequest.window);
                else
                    XMapWindow(W->XDisplay, C->win);
                break;
            case KeyPress:
                printf("Keypress! %u\n", ev.xkey.keycode);
                key_pressed(W, &ev);
                break;
            case Expose:
                expose_event(W, &ev);
                break;
        }
    }
}

static void find_open_windows(struct WM_t *W)
{
    Window *children, root_ret, par_ret;
    unsigned int n, i;
    XQueryTree(W->XDisplay, W->rootWindow, &root_ret, &par_ret, &children, &n);

    for (i = 0; i < n; i++)
        register_client(W, children[i]);

    XFree(children);
}

/* Clean up nicely and unreparent all the windows */
void tidy_up(void)
{
    struct WM_t *W = wm_state_for_quit;
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (W->clients[i] != NULL)
        {
            struct wmclient *c = W->clients[i];
            printf("Freeing \'%s\'\n", c->name);
        }
    }
    XCloseDisplay(W->XDisplay);
}

static void init_state(struct WM_t *W)
{
    int i;
    W->bsize = 1;
    for (i = 0; i < MAX_CLIENTS; i++)
        W->clients[i] = NULL;
}

int main(void)
{
    struct WM_t W;

    wm_state_for_quit = &W;
    atexit(tidy_up);

    init_state(&W);
    open_display(&W);
    find_open_windows(&W);
    event_loop(&W);

    return 0;
}

