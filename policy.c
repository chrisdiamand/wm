
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "policy.h"
#include "wm.h"

void decide_new_window_size_pos(struct WM_t *W, struct wmclient *C)
{
    XWindowAttributes attr;
    XSizeHints hints;
    long user_hints;

    /* Get actual position/size of window */
    XGetWindowAttributes(W->XDisplay, C->win, &attr);

    C->min_w = C->min_h = 30;

    C->x = attr.x;
    C->y = attr.y;
    C->w = attr.width;
    C->h = attr.height;
    printf("New window: Size %dx%d\n", C->w, C->h);

    /* Get size hints */
    if (XGetWMNormalHints(W->XDisplay, C->win, &hints, &user_hints))
    {
        if (hints.flags & PMinSize)
        {
            if (C->min_w < hints.min_width)
                C->min_w = hints.min_width;
            if (C->min_h < hints.min_height)
                C->min_h = hints.min_height;
        }
    }

    /* The minimum size could be larger than the screen? */
    if (C->w < C->min_w)
        C->w = C->min_w;
    if (C->h < C->min_h)
        C->h = C->min_h;

    /* Don't let windows be larger than the root window */
    if (C->w > W->rW)
    {
        C->w = W->rW - 2 * W->bsize;
        C->x = 0;
    }

    if (C->h > W->rH)
    {
        C->h = W->rH - 2 * W->bsize;
        C->y = 0;
    }

}

