
#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_CLIENTS 64

#include "alttab.h"
#include "launcher.h"
#include "wmprefs.h"

struct wmclient
{
    char                *name;
    int                 pid;
    Window              win;
    int                 x, y, w, h;
    int                 min_w, min_h;
    int                 fullscreen;
};

/* Hold state info about the WM */
struct WM_t
{
    /* X stuff */
    Display             *XDisplay;
    int                 XScreen;
    Window              rootWindow;
    GC                  rootGC;

    Cursor              cursor_normal, cursor_move, cursor_resize;

    /* Root window width and height */
    unsigned int        rW, rH;

    /* List of open windows/programs, sorted in focus order. */
    struct wmclient     *clients[MAX_CLIENTS];
    int                 nclients;

    /* Info about the Alt-Tab switcher window */
    struct alttab_t     AT;
    struct launcher_t   launcher;

    unsigned long       black, lightgrey, white, focus_border_colour;

    struct wmprefs_t    prefs;
};

void do_alttab(struct WM_t *);
void redraw_root(struct WM_t *, XEvent *);

/* Functions from client.c */
int client_insert(struct WM_t *, struct wmclient *);
void client_register(struct WM_t *, Window);
void client_select_events(struct WM_t *, struct wmclient *);
struct wmclient *client_from_window(struct WM_t *, Window);
void client_togglefullscreen(struct WM_t *, struct wmclient *);
void client_moveresize(struct WM_t *, struct wmclient *, int, int, int, int);
void client_focus(struct WM_t *, struct wmclient *);
void client_remove(struct WM_t *, struct wmclient *);

/* Functions from event.c */
char *event_name(int);
void event_loop(struct WM_t *);

/* Debugging function */
int msg(char *, ...);

#endif

