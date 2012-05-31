
#include <stdio.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xutil.h>

#include "alttab.h"
#include "launcher.h"
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

static void event_expose(struct WM_t *W, XEvent *ev)
{
    if (ev->xexpose.window == W->rootWindow)
        redraw_root(W, ev);
}

static void move_snap(struct WM_t *W, struct wmclient *C, int resize)
{
    /* Work out the bottom right corner position */
    int rx = C->x + C->w + 2 * W->prefs.bw;
    int ry = C->y + C->h + 2 * W->prefs.bw;
    /* Snap to borders if near edges */
    if (ABS(C->x) < W->prefs.snap_width)
        C->x = 0;
    if (ABS(C->y) < W->prefs.snap_width)
        C->y = 0;
    /* Right border */
    if (ABS(W->rW - rx) < W->prefs.snap_width)
    {
        if (resize)
            C->w = W->rW - (C->x + 2*W->prefs.bw);
        else
            C->x = W->rW - (C->w + 2*W->prefs.bw);
    }
    if (ABS(W->rH - ry) < W->prefs.snap_width)
    {
        if (resize)
            C->h = W->rH - (C->y + 2*W->prefs.bw);
        else
            C->y = W->rH - (C->h + 2*W->prefs.bw);
    }

    if (C->w < C->min_w)
        C->w = C->min_w;
    if (C->h < C->min_h)
        C->h = C->min_h;

    XMoveResizeWindow(W->XDisplay, C->win, C->x, C->y, C->w, C->h);
}

/* Move a window. It has been clicked at coordinates (xOff, yOff) relative to
 * the top-left corner of the client. */
static void event_move_window(struct WM_t *W, struct wmclient *C, int xOff, int yOff)
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
        C->w = W->rW - 2 * W->prefs.bw;
        C->h = W->rH - 2 * W->prefs.bw;
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

static void event_resize_window(struct WM_t *W, struct wmclient *C, int init_x, int init_y)
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
        startW = C->w = W->rW - 2 * W->prefs.bw;
        startH = C->h = W->rH - 2 * W->prefs.bw;
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

/* XK_Super_L is win key */
static void event_key_pressed(struct WM_t *W, struct wmclient *C, XEvent *ev)
{
    /* Border size */
    int B = W->prefs.bw;
    KeySym sym = XLookupKeysym(&(ev->xkey), 0);
    if (sym == XK_Tab && (ev->xkey.state & Mod1Mask))
        alttab(W);
    
    if (ev->xkey.state & (Mod1Mask | ShiftMask))
    {
        switch (sym)
        {
            case XK_f:
                client_togglefullscreen(W, C);
                break;
            /* Tiling. Passing -1 to moveresize is for maximising in that dimension */
            case XK_Up:
                client_moveresize(W, C, 0, 0, -1, W->rH / 2 - 2 * B);
                break;
            case XK_Down:
                client_moveresize(W, C, 0, W->rH / 2 - B, -1, W->rH / 2 - B);
                break;
            case XK_Left:
                client_moveresize(W, C, 0, 0, W->rW / 2 - 2 * B, -1);
                break;
            case XK_Right:
                client_moveresize(W, C, W->rW / 2 - B, 0, W->rW / 2 - B, -1);
                break;
            case XK_Return:
                launcher(W);
                break;
        }
    }
}

static void event_configure_request(struct WM_t *W, struct wmclient *C, XEvent *ev)
{
    XConfigureRequestEvent *conf = &(ev->xconfigurerequest);
    printf("ConfigureRequest: border = %d\n", conf->border_width);
    printf("    x, y = %d, %d\n", conf->x, conf->y);
    printf("    w, h = %d, %d\n", conf->width, conf->height);

    if (C)
    {
        printf("Resizing client \'%s\'\n", C->name);
        client_moveresize(W, C, conf->x, conf->y, conf->width, conf->height);
    }
    else
    {
        printf("Resizing anon.\n");
        XMoveResizeWindow(W->XDisplay, conf->window,
                          conf->x, conf->y, conf->width, conf->height);
    }
}


#define NETWM_MAX_STATE "_NET_WM_STATE_MAXIMISED"
static void client_message(struct WM_t *W, struct wmclient *C, XEvent *ev)
{
    char *name;
    XClientMessageEvent cm = ev->xclient;
    name = XGetAtomName(W->XDisplay, cm.message_type);
    if (strcmp(name, "_NET_WM_STATE") == 0)
    {
        char *a = XGetAtomName(W->XDisplay, cm.data.l[1]);
        msg("Maximise \'%s\'\n", C->name);
        if (strncmp(a, NETWM_MAX_STATE, sizeof(NETWM_MAX_STATE)))
            client_togglefullscreen(W, C);
    }
    /*
    msg("Atom name %s. Format %d\n", XGetAtomName(W->XDisplay, cm.message_type), cm.format);
    for (i = 0; i < 5; i++)
        msg("%d : %s\n", i, XGetAtomName(W->XDisplay, cm.data.l[i]));
        */
}

void event_loop(struct WM_t *W)
{
    XEvent ev;
    struct wmclient *C = NULL;
    unsigned int state;
    while (1)
    {
        XNextEvent(W->XDisplay, &ev);
        C = client_from_window(W, ev.xany.window);

        switch (ev.type)
        {
            /* Mouse button */
            case ButtonPress:
                state = ev.xbutton.state;
                /* Alt+click to move a window, resize if shift held as well */
                /* Shift+alt+click to resize a window */
                if (C && (state & (Button1Mask | Mod1Mask)))
                {
                    if (state & ShiftMask)
                        event_resize_window(W, C, ev.xbutton.x_root, ev.xbutton.y_root);
                    else
                        event_move_window(W, C, ev.xbutton.x, ev.xbutton.y);
                }
                /* A normal click to focus a window */
                else if (C && state == 0)
                {
                    client_focus(W, C);
                    XSendEvent(W->XDisplay, C->win, 0, ButtonPressMask, &ev);
                }
                break;
            case ConfigureNotify:
                break;
            case ConfigureRequest:
                event_configure_request(W, C, &ev);
                break;
            case MapRequest: /* Does not use CreateNotify */
                msg("Map request\n");
                /* Don't register it again if it was just hiding for some reason
                   or if it's the Alt-Tab switcher window */
                if (!C && ev.xany.window != W->AT.win)
                    client_register(W, ev.xmaprequest.window);
                else
                    XMapWindow(W->XDisplay, C->win);
                break;
            case UnmapNotify:
                if (C)
                    msg("%s unmapped\n", C->name);
                break;
            case DestroyNotify:
                if (C)
                    client_remove(W, C);
                break;
            case KeyPress:
                event_key_pressed(W, C, &ev);
                break;
            case Expose:
                event_expose(W, &ev);
                break;
            case ClientMessage:
                if (C)
                    client_message(W, C, &ev);
                else
                    msg("Client message from unknown client!\n");
                break;
            default:
                msg("- %s -", event_name(ev.type));
                if (C)
                    msg(" %s -\n", C->name);
                else
                    msg("\n");
                break;
        }
    }
}
