
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "policy.h"
#include "wm.h"

struct WM_t *wm_state_for_quit = NULL;
FILE *log_file = NULL;

int msg(char *fmt, ...)
{
    va_list ap;
    int i;
    char msg[512];

    va_start(ap, fmt);
    i = vsnprintf(msg, sizeof(msg), fmt, ap);
    printf("%s", msg);
    if (log_file)
    {
        fprintf(log_file, "%s", msg);
        fflush(log_file);
    }
    return i;
}

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

static int error_handler(Display *dpy, XErrorEvent *ev)
{
    char errstr[128];
    struct wmclient *C;

    XGetErrorText(dpy, ev->error_code, errstr, sizeof(errstr));
    msg("X error: %s\n", errstr);
    msg("    serial = %lu\n", ev->serial);
    msg("    error_code = %d\n", ev->error_code);
    msg("    request_code = %d\n", ev->request_code);
    msg("    minor_code = %d\n", ev->minor_code);
    msg("    resourceid = %lu\n", ev->resourceid);

    C = client_from_window(wm_state_for_quit, ev->resourceid);
    if (C)
        msg("    from client \'%s\'\n", C->name);

    return 0;   
}

static void open_display(struct WM_t *W)
{
    Window root, tmpwin;
    int tmpx, tmpy;
    unsigned int tmpbw, tmpdepth;

    W->XDisplay = XOpenDisplay(NULL);
    if (!W->XDisplay)
    {
        msg("Cannot open X display!\n");
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

    XSelectInput(W->XDisplay, W->rootWindow, KeyPressMask               |
                                             KeyReleaseMask             |
                                             EnterWindowMask            |
                                             FocusChangeMask            |
                                             SubstructureRedirectMask   |
                                             ButtonPressMask            |
                                             ButtonReleaseMask          |
                                             ExposureMask);
    


    /* Find out the geometry of the root window */
    XGetGeometry(W->XDisplay, W->rootWindow, &tmpwin,
                 &tmpx, &tmpy,
                 &(W->rW), &(W->rH),
                 &tmpbw, &tmpdepth);

    XSetErrorHandler(error_handler);

    redraw_root(W, NULL);
}

static void key_pressed(struct WM_t *W, XEvent *ev)
{
    switch (ev->xkey.keycode)
    {
        case KEY_ALT:
            msg("Alt!\n");
            break;
        case KEY_WIN:
            msg("Win key\n");
            system("dmenu_run &");
            break;
    }
}

static void expose_event(struct WM_t *W, XEvent *ev)
{
    if (ev->xexpose.window == W->rootWindow)
        redraw_root(W, ev);
}

static void move_client_window(struct WM_t *W, struct wmclient *C, int xOff, int yOff)
{
    XEvent ev;

    msg("Moving.\n");
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
                msg("done moving. Last event was %d\n", ev.type);
                /* Select the normal events again */
                client_select_events(W, C);
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
                msg("Button pressed\n");
                /* ALT+click to move a window */
                if ((ev.xbutton.state & (Button1Mask | Mod1Mask)) && C)
                    move_client_window(W, C, ev.xbutton.x, ev.xbutton.y);
                else if (C)
                    XRaiseWindow(W->XDisplay, C->win);
                break;
            case MapRequest: /* Does not use CreateNotify */
                msg("Map request\n");
                if (!C) /* Don't register it again if it was just hiding for some reason */
                    client_register(W, ev.xmaprequest.window);
                else
                    XMapWindow(W->XDisplay, C->win);
                break;
            case UnmapNotify:
            case VisibilityNotify:
            case DestroyNotify:
                msg("Unmap \'%s\'\n", C->name);
                break;
            case KeyPress:
                msg("Keypress! %u\n", ev.xkey.keycode);
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
        client_register(W, children[i]);

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
            msg("Freeing \'%s\'\n", c->name);
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

    log_file = fopen("wm_errors.txt", "w");
    assert(log_file);

    wm_state_for_quit = &W;
    atexit(tidy_up);

    init_state(&W);
    open_display(&W);
    find_open_windows(&W);
    event_loop(&W);

    return 0;
}

