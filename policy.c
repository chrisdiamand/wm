
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

    /* Get hints on size limits, but not the actual sizes. If a client
     * wants a window a certain size, they will just create it that size,
     * not put it as a 'hint'. */
    if (XGetWMNormalHints(W->XDisplay, C->win, &hints, &user_hints))
    {
        if (hints.flags & PMinSize)
        {
            if (C->min_w < hints.min_width)
                C->min_w = hints.min_width;
            if (C->min_h < hints.min_height)
                C->min_h = hints.min_height;
        }

        /* If size hints are provided and are larger than
         * the minimum, use them instead */
        if (hints.flags & PBaseSize)
        {
            if (hints.base_width > C->min_w)
                C->w = hints.base_width;
            if (hints.base_height > C->min_h)
                C->h = hints.base_height;
        }
        else if (hints.flags & PSize)
        {
            if (hints.width > C->min_w)
                C->w = hints.width;
            if (hints.height > C->min_h)
                C->h = hints.height;
        }
    }

    if (C->w < C->min_w)
        C->w = C->min_w;
    if (C->h < C->min_h)
        C->h = C->min_h;

    /* Don't let windows be larger than the root window */
    if (C->w > W->rW)
    {
        C->w = W->rW - 2 * W->prefs.bw;
        C->x = 0;
    }

    if (C->h > W->rH)
    {
        C->h = W->rH - 2 * W->prefs.bw;
        C->y = 0;
    }
}

