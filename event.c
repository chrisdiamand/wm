
#include <X11/X.h>
#include <X11/Xutil.h>

#include "wm.h"

static int ABS(int n)
{
    return n >= 0 ? n : -n;
}

char *event_name(int type)
{
    switch (type)
    {
        case KeyPress:          return "KeyPress";
        case KeyRelease:        return "KeyRelease";
        case ButtonPress:       return "ButtonPress";
        case ButtonRelease:     return "ButtonRelease";
        case MotionNotify:      return "MotionNotify";
        case EnterNotify:       return "EnterNotify";
        case LeaveNotify:       return "LeaveNotify";
        case FocusIn:           return "FocusIn";
        case FocusOut:          return "FocusOut";
        case KeymapNotify:      return "KeymapNotify";
        case Expose:            return "Expose";
        case GraphicsExpose:    return "GraphicsExpose";
        case NoExpose:          return "NoExpose";
        case VisibilityNotify:  return "VisibilityNotify";
        case CreateNotify:      return "CreateNotify";
        case DestroyNotify:     return "DestroyNotify";
        case UnmapNotify:       return "UnmapNotify";
        case MapNotify:         return "MapNotify";
        case MapRequest:        return "MapRequest";
        case ReparentNotify:    return "ReparentNotify";
        case ConfigureNotify:   return "ConfigureNotify";
        case ConfigureRequest:  return "ConfigureRequest";
        case GravityNotify:     return "GravityNotify";
        case ResizeRequest:     return "ResizeRequest";
        case CirculateNotify:   return "CirculateNotify";
        case CirculateRequest:  return "CirculateRequest";
        case PropertyNotify:    return "PropertyNotify";
        case SelectionClear:    return "SelectionClear";
        case SelectionRequest:  return "SelectionRequest";
        case SelectionNotify:   return "SelectionNotify";
        case ColormapNotify:    return "ColormapNotify";
        case ClientMessage:     return "ClientMessage";
        case MappingNotify:     return "MappingNotify";
        case GenericEvent:      return "GenericEvent";
        case LASTEvent:         return "LASTEvent";
        default:                return "<UnknownEvent>";
    }
    return "Switch didn't work???";
}

void event_expose(struct WM_t *W, XEvent *ev)
{
    if (ev->xexpose.window == W->rootWindow)
        redraw_root(W, ev);
}

static void move_snap(struct WM_t *W, struct wmclient *C, int resize)
{
    /* Work out the bottom right corner position */
    int rx = C->x + C->w + 2 * W->bsize;
    int ry = C->y + C->h + 2 * W->bsize;
    /* Snap to borders if near edges */
    if (ABS(C->x) < W->snapwidth)
        C->x = 0;
    if (ABS(C->y) < W->snapwidth)
        C->y = 0;
    /* Right border */
    if (ABS(W->rW - rx) < W->snapwidth)
    {
        if (resize)
            C->w = W->rW - (C->x + 2*W->bsize);
        else
            C->x = W->rW - (C->w + 2*W->bsize);
    }
    if (ABS(W->rH - ry) < W->snapwidth)
    {
        if (resize)
            C->h = W->rH - (C->y + 2*W->bsize);
        else
            C->y = W->rH - (C->h + 2*W->bsize);
    }

    if (C->w < C->min_w)
        C->w = C->min_w;
    if (C->h < C->min_h)
        C->h = C->min_h;

    XMoveResizeWindow(W->XDisplay, C->win, C->x, C->y, C->w, C->h);
}

/* Move a window. It has been clicked at coordinates (xOff, yOff) relative to
 * the top-left corner of the client. */
void event_move_window(struct WM_t *W, struct wmclient *C, int xOff, int yOff)
{
    XEvent ev;
    long mask = ButtonReleaseMask   |
                ButtonMotionMask    |
                PointerMotionMask;

    /* If it's fullscreen, add a border and move it as the size of the whole screen. */
    if (C->fullscreen)
    {
        C->x = 0;
        C->y = 0;
        C->w = W->rW - 2 * W->bsize;
        C->h = W->rH - 2 * W->bsize;
        client_togglefullscreen(W, C);
    }

    msg("Moving.\n");

    /* Grab the pointer to ensure the ButtonRelease event is received
     * FIXME: Change the cursor */
    XGrabPointer(W->XDisplay, W->rootWindow, 0, mask, GrabModeAsync,
                 GrabModeAsync, None, W->cursor_move, CurrentTime);

    while (1)
    {
        /* Don't use XNextEvent as moving could be stopped if another window closes,
         * for example. This could also mean such events are missed completely.
         * Add ExposeMask for root window draw updates */
        XMaskEvent(W->XDisplay, mask | ExposureMask, &ev);
        switch (ev.type)
        {
            case MotionNotify:
                C->x = ev.xmotion.x_root - xOff;;
                C->y = ev.xmotion.y_root - yOff;;
                move_snap(W, C, 0);
                break;
            case Expose: /* Redraw background during window moves */
                event_expose(W, &ev);
                break;
            case ButtonRelease:
            default:
                msg("done moving. Last event was %s\n", event_name(ev.type));
                /* Select the normal events again */
                XUngrabPointer(W->XDisplay, CurrentTime);
                return;
        }
    }
}

void event_resize_window(struct WM_t *W, struct wmclient *C, int init_x, int init_y)
{
    XEvent ev;
    long mask = ButtonReleaseMask   |
                ButtonMotionMask    |
                PointerMotionMask;
    int startW = C->w, startH = C->h;

    if (C->fullscreen)
    {
        msg("Resizing fullscreen window!\n");
        C->x = 0;
        C->y = 0;
        startW = C->w = W->rW - 2 * W->bsize;
        startH = C->h = W->rH - 2 * W->bsize;
        client_togglefullscreen(W, C);
    }
    /* Grab the pointer to ensure the ButtonRelease event is received
     * FIXME: Change the cursor */
    XGrabPointer(W->XDisplay, W->rootWindow, 0, mask, GrabModeAsync,
                 GrabModeAsync, None, W->cursor_move, CurrentTime);

    /* Make the offsets be the distance from the bottom/right of the window, not top/left */

    msg("Clicked at %d, %d\n", init_x, init_y);
    while (1)
    {
        /* Don't use XNextEvent as moving could be stopped if another window closes,
         * for example. This could also mean such events are missed completely.
         * Add ExposeMask for root window draw updates */
        XMaskEvent(W->XDisplay, mask | ExposureMask, &ev);
        switch (ev.type)
        {
            case MotionNotify:
                C->w = startW + ev.xmotion.x_root - init_x;
                C->h = startH + ev.xmotion.y_root - init_y;
                move_snap(W, C, 1);
                break;
            case Expose: /* Redraw background during window moves */
                event_expose(W, &ev);
                break;
            case ButtonRelease:
            default:
                msg("done resization. Last event was %s\n", event_name(ev.type));
                /* Select the normal events again */
                XUngrabPointer(W->XDisplay, CurrentTime);
                return;
        }
    }
}

