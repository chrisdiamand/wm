
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "alttab.h"
#include "launcher.h"
#include "policy.h"
#include "wm.h"
#include "wmprefs.h"

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

void redraw_root(struct WM_t *W, XEvent *ev)
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

static int starting_error_handler(Display *dpy, XErrorEvent *ev)
{
    /* If this function is called another WM must be running */
    msg("Error: Could not select SubstructureRedirectMask on root window.\n");
    msg("Is another window manager running?\n");
    msg("Quitting.\n");
    /* Die properly so the atexit() function isn't called */
    _Exit(1);
    return 1;
}

static void check_existing_wm(struct WM_t *W)
{
    XSetErrorHandler(starting_error_handler);
    /* This will cause an error if there is another WM running */
    XSelectInput(W->XDisplay, W->rootWindow, SubstructureRedirectMask);
    /* Wait for all events to be processed */
    XSync(W->XDisplay, 0);
    XSetErrorHandler(NULL);
}

/* Takes 8-bit R G and B values between 0 and 255 */
unsigned long colour_from_rgb(struct WM_t *W, short r, short g, short b)
{
    XColor c;
    c.red = r * 257;        /* 65535 / 255 = 257 */;
    c.green = g * 257;
    c.blue = b * 257;
    XAllocColor(W->XDisplay, DefaultColormap(W->XDisplay, DefaultScreen(W->XDisplay)), &c);
    return c.pixel;
}

static void make_colours(struct WM_t *W)
{
    struct wmprefs_t *p = &(W->prefs);

    W->black = BlackPixel(W->XDisplay, W->XScreen);

    W->focus_border_col = colour_from_rgb(W, p->focus_border_col[0],
                                             p->focus_border_col[1],
                                             p->focus_border_col[2]);

    W->unfocus_border_col = colour_from_rgb(W, p->unfocus_border_col[0],
                                               p->unfocus_border_col[1],
                                               p->unfocus_border_col[2]);

    W->fg_col = colour_from_rgb(W, p->fg_col[0],
                                   p->fg_col[1],
                                   p->fg_col[2]);

    W->bg_col = colour_from_rgb(W, p->bg_col[0],
                                   p->bg_col[1],
                                   p->bg_col[2]);

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

    check_existing_wm(W);

    XSelectInput(W->XDisplay, W->rootWindow, KeyPressMask               |
                                             KeyReleaseMask             |
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

    make_colours(W);

    XSetErrorHandler(error_handler);

    W->cursor_normal = XCreateFontCursor(W->XDisplay, XC_left_ptr);
    W->cursor_resize = XCreateFontCursor(W->XDisplay, XC_sizing);
    W->cursor_move = XCreateFontCursor(W->XDisplay, XC_fleur);
    XDefineCursor(W->XDisplay, W->rootWindow, W->cursor_normal);

    redraw_root(W, NULL);
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
    printf("Tidy up...\n");
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (W->clients[i] != NULL)
        {
            struct wmclient *c = W->clients[i];
            msg("Freeing \'%s\'\n", c->name);
        }
    }
    XCloseDisplay(W->XDisplay);
    printf("Done!\n");
}

static void init_state(struct WM_t *W)
{
    int i;
    W->nclients = 0;
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

    /* Allocate the state structure */
    init_state(&W);
    /* Make up values for the default settings */
    wmprefs_load_defaults(&(W.prefs));
    /* Read the config file */
    wmprefs_read_config_files(&(W.prefs));
    /* Open connection to X server */
    open_display(&W);
    /* See if anything is already open (e.g. a typing .xinitrc) */
    find_open_windows(&W);
    /* Allocate the alt-tab window */
    alttab_init(&W);
    printf("Alt tab done.\n");
    /* And a launcher window */
    launcher_init(&W);
    /* Go! */
    msg("GOing to event loop.\n");
    event_loop(&W);

    return 0;
}

