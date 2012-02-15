
#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_CLIENTS 128

struct wmclient
{
    char                *name;
    Window              win;
    int                 x, y, w, h;
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

    /* List of open windows/programs. Too lazy to do a linked list so
     * bodged fixed array thingy for now. */
    struct wmclient     *clients[MAX_CLIENTS];

    /* Font to use */
    XFontStruct         *font;
};

void do_alttab(struct WM_t *);

/* Functions from client.c */
int client_insert(struct WM_t *, struct wmclient *);
void client_register(struct WM_t *, Window);
struct wmclient *client_from_window(struct WM_t *, Window);

#define KEY_Q 24
#define KEY_ALT 64
#define KEY_WIN 133

#define HELVETICA_11 "*helvetica*11*"
#define HELVETICA_12 "*helvetica*12*"
#define COURIER_11 "*courier*11*"
#define COURIER_12 "*courier*12*"

#endif

