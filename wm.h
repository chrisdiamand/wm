
#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_CLIENTS 64

#define ALT_TAB_CHARACTERS 32
#define ALT_TAB_FONTNAME "fixed"
#define ALT_TAB_FONTNAME__ "-*-helvetica-*-22-*"

#define HELVETICA_11 "*helvetica*11*"
#define HELVETICA_12 "*helvetica*12*"
#define COURIER_11 "*courier*11*"
#define COURIER_12 "*courier*12*"

struct alttab_t
{
    Window              win;
    int                 w, h, x, y;
    int                 item_height;
    GC                  gc;
    unsigned long       inputeventmask;
    XFontStruct         *font;
    /* Index of currently selected window */
    int                 selected;
};

struct wmclient
{
    char                *name;
    Window              win;
    int                 x, y, w, h;
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

    /* Root window width and height */
    unsigned int        rW, rH;

    /* Default window border size */
    int                 bsize;
    /* Maximum distance for snapping to borders during window moves */
    int                 snapwidth;

    /* List of open windows/programs, sorted in focus order. */
    struct wmclient     *clients[MAX_CLIENTS];
    int                 nclients;

    /* Info about the Alt-Tab switcher window */
    struct alttab_t     AT;

    unsigned long       black, lightgrey, white, focus_border_colour;
};

void do_alttab(struct WM_t *);

/* Functions from client.c */
int client_insert(struct WM_t *, struct wmclient *);
void client_register(struct WM_t *, Window);
void client_select_events(struct WM_t *, struct wmclient *);
struct wmclient *client_from_window(struct WM_t *, Window);
void client_togglefullscreen(struct WM_t *W, struct wmclient *C);
void client_focus(struct WM_t *W, struct wmclient *C);
void client_remove(struct WM_t *, struct wmclient *);

/* Functions from alttab.c */
void alttab(struct WM_t *);
void alttab_init(struct WM_t *);

/* Functions from event.c */
char *event_name(int);

/* Debugging function */
int msg(char *, ...);

#endif

