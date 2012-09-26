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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "launcher.h"
#include "policy.h"
#include "switcher.h"
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
    int x, y, w, h;

    XSetForeground(W->XDisplay, W->rootGC, W->root_bg_col);

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
        w = W->root_max_w;
        h = W->root_max_h;
    }

    XFillRectangle(W->XDisplay, W->rootWindow, W->rootGC, x, y, w, h);
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

    W->root_bg_col = colour_from_rgb(W, p->root_bg_col[0],
                                        p->root_bg_col[1],
                                        p->root_bg_col[2]);

}

static void open_display(struct WM_t *W)
{
    Window root;

    W->XDisplay = XOpenDisplay(NULL);
    if (!W->XDisplay)
    {
        msg("Cannot open X display!\n");
        exit(1);
    }

    W->XScreen = DefaultScreen(W->XDisplay);

    W->rootWindow = root = RootWindow(W->XDisplay, W->XScreen);
    
    check_existing_wm(W);

    XSelectInput(W->XDisplay, W->rootWindow, KeyPressMask               |
                                             KeyReleaseMask             |
                                             FocusChangeMask            |
                                             SubstructureRedirectMask   |
                                             ButtonPressMask            |
                                             ButtonReleaseMask          |
                                             ExposureMask);
    make_colours(W);

    XSetErrorHandler(error_handler);

    W->cursor_normal = XCreateFontCursor(W->XDisplay, XC_left_ptr);
    W->cursor_resize = XCreateFontCursor(W->XDisplay, XC_sizing);
    W->cursor_move = XCreateFontCursor(W->XDisplay, XC_fleur);
    XDefineCursor(W->XDisplay, W->rootWindow, W->cursor_normal);

    W->rootGC = XCreateGC(W->XDisplay, W->rootWindow, 0, NULL);
    redraw_root(W, NULL);
}

static void refresh_X_info(struct WM_t *W)
{
    int evbase, errbase;
    Window tmpwin;
    int tmpx, tmpy;
    unsigned int tmpbw, tmpdepth;

    XGetGeometry(W->XDisplay, W->rootWindow, &tmpwin,
                 &tmpx, &tmpy,
                 &(W->root_max_w), &(W->root_max_h),
                 &tmpbw, &tmpdepth);


    if (XineramaQueryExtension(W->XDisplay, &evbase, &errbase))
    {
        W->heads = XineramaQueryScreens(W->XDisplay, &(W->n_heads));
        printf("Querying screens, got %p, n = %u\n", W->heads, W->n_heads);
    }

    if (W->heads == NULL)
    {
        msg("Error: Xinerama either not present or not active!\n");

        W->heads = malloc(sizeof(XineramaScreenInfo));
        W->n_heads = 1;

        /* Load the root window dimensions and put it into a single
         * XineramaScreenInfo, using the root window dimensions */
        W->heads->screen_number = 0;
        W->heads->x_org = W->heads->y_org = 0;
        W->heads->width = W->root_max_w;
        W->heads->height = W->root_max_h;
    }
    
    W->curr_head = 0;
}

/* Clean up nicely and unreparent all the windows */
void tidy_up(void)
{
    struct WM_t *W = wm_state_for_quit;
    int i;
    msg("Tidying up.\n");
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        struct wmclient *c = W->clients[i];
        if (c)
            free(c);
    }
    XCloseDisplay(W->XDisplay);
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

    /* Allocate the state structure */
    init_state(&W);
    /* Make up values for the default settings */
    wmprefs_load_defaults(&(W.prefs));
    /* Read the config file */
    wmprefs_read_config_files(&(W.prefs));
    /* Open connection to X server */
    open_display(&W);
    /* Get information about connected monitors */
    refresh_X_info(&W);
    /* Draw root background */
    redraw_root(&W, NULL);
    /* See if anything is already open (e.g. a typing .xinitrc) */
    client_find_open_windows(&W);
    /* Allocate the alt-tab window */
    switcher_init(&W);
    /* And a launcher window */
    launcher_init(&W);
    /* Go! */
    atexit(tidy_up);
    event_loop(&W);

    return 0;
}

