
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "policy.h"
#include "wm.h"

void decide_new_window_size_pos(struct WM_t *W, Window xwinid, int *x, int *y, int *w, int *h)
{
    XWindowAttributes attr;
    XSizeHints hints;
    long user_hints;

    /* Get actual position/size of window */
    XGetWindowAttributes(W->XDisplay, xwinid, &attr);

    *w = attr.width;
    *h = attr.height;

    /* Get size hints */
    if (XGetWMNormalHints(W->XDisplay, xwinid, &hints, &user_hints))
    {
        printf("Hints: flags = %ld\n", hints.flags);
        printf("      w = %d,     h = %d\n", hints.width, hints.height);
        printf("  min_w = %d, min_h = %d\n", hints.min_width, hints.min_height);
        printf("  max_w = %d, max_h = %d\n", hints.max_width, hints.max_height);
        
        /* Check if a hint size has been provided */
        if ((hints.flags & PSize) || (hints.flags & USSize))
        {
            printf("Using hints\n");
            *w = hints.width;
            *h = hints.height;
        }
        else if (hints.flags & PMinSize)
        {
            if (attr.width < hints.min_width)
                *w = hints.min_width;
            if (attr.height < hints.min_height)
                *h = hints.min_height;
        }

        /* Check if there is a hint position */
        if ((hints.flags & PPosition) || (hints.flags & USPosition))
        {
            *x = hints.x;
            *y = hints.y;
        }
        else
        {
            *x = attr.x;
            *y = attr.y;
        }
    }

    /* Don't let windows be larger than the root window */
    if (*w > W->rW)
    {
        *w = W->rW - 2 * W->bsize;
        *x = 0;
    }

    if (*h > W->rH)
    {
        *h = W->rH - 2 * W->bsize;
        *y = 0;
    }

}

