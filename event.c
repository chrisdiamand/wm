
#include <X11/X.h>
#include <X11/Xutil.h>

#include "wm.h"

/* For some reason this won't work as a macro... */
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

/* Move a window. It has been clicked at coordinates (xOff, yOff) relative to
 * the top-left corner of the client. */
void event_move_window(struct WM_t *W, struct wmclient *C, int xOff, int yOff)
{
    XEvent ev;
    long mask = ButtonReleaseMask   |
                ButtonMotionMask    |
                PointerMotionMask;
    /* Top left corner (x, y), bottom right corner (rx, ry) */
    int x, y, rx, ry;

    /* Can't move fullscreen windows */
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
                x = ev.xmotion.x_root - xOff;
                y = ev.xmotion.y_root - yOff;
                rx = x + C->w + 2 * W->bsize;
                ry = y + C->h + 2 * W->bsize;
                /* Snap to borders if near edges */
                if (ABS(x) < W->snapwidth)
                    x = 0;
                if (ABS(y) < W->snapwidth)
                    y = 0;
                if (ABS(W->rW - rx) < W->snapwidth)
                    x = W->rW - C->w - 2*W->bsize;
                if (ABS(W->rH - ry) < W->snapwidth)
                    y = W->rH - C->h - 2*W->bsize;
                XMoveWindow(W->XDisplay, C->win, x, y);
                C->x = x;
                C->y = y;
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


