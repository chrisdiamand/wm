
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/cursorfont.h>
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

unsigned long colour_from_rgb(struct WM_t *W, double r, double g, double b)
{
    XColor c;
    c.red = r * 65535.0;
    c.green = g * 65535.0;
    c.blue = b * 65535.0;
    XAllocColor(W->XDisplay, DefaultColormap(W->XDisplay, DefaultScreen(W->XDisplay)), &c);
    return c.pixel;
}

static void make_colours(struct WM_t *W)
{
    W->black = BlackPixel(W->XDisplay, W->XScreen);
    W->white = WhitePixel(W->XDisplay, W->XScreen);
    W->lightgrey = colour_from_rgb(W, 0.8, 0.8, 0.8);
    W->focus_border_colour = colour_from_rgb(W, 0.0, 0.7, 0.0);
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
    W->bsize = 1;
    W->snapwidth = 15;
    W->nclients = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
        W->clients[i] = NULL;
}

static void load_font(struct WM_t *W)
{
    char **font_names;
    int count, i;
    font_names = XListFonts(W->XDisplay, "*fixed*", 16, &count);
    for (i = 0; i < count; i++)
        msg("    %s\n", font_names[i]);

    W->font = XLoadQueryFont(W->XDisplay, WM_FONTNAME);
    if (!W->font)
    {
        msg("Couldn't load font \'%s\', using \'fixed\' instead.\n", WM_FONTNAME);
        W->font = XLoadQueryFont(W->XDisplay, "fixed");
        assert(W->font);
    }
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
    load_font(&W);
    find_open_windows(&W);
    alttab_init(&W);
    launcher_init(&W);
    event_loop(&W);

    return 0;
}

