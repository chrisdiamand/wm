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

#include <stdio.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xutil.h>

#include "launcher.h"
#include "policy.h"
#include "switcher.h"
#include "wm.h"

#if 0
static int ABS(int n)
{
    return n >= 0 ? n : -n;
}
#endif

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

/* Move a window to the position in the wmclient struct, but
 * first see if it can be snapped to any edges */
static void move_snap(struct WM_t *W, struct wmclient *C, int resize)
{
    #if 0
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

    #endif
    client_moveresize(W, C, C->x, C->y, C->w, C->h);
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
        C->x = curr_head_x(W);
        C->y = curr_head_y(W);
        C->w = curr_width(W) - 2 * W->prefs.bw;
        C->h = curr_height(W) - 2 * W->prefs.bw;
        client_togglefullscreen(W, C);
    }

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
                /* Select the normal events again */
                XUngrabPointer(W->XDisplay, CurrentTime);
                refresh_current_head(W);
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
        C->x = curr_head_x(W);
        C->y = curr_head_y(W);
        startW = C->w = curr_width(W) - 2 * W->prefs.bw;
        startH = C->h = curr_height(W) - 2 * W->prefs.bw;
        client_togglefullscreen(W, C);
    }
    /* Grab the pointer to ensure the ButtonRelease event is received
     * FIXME: Change the cursor */
    XGrabPointer(W->XDisplay, W->rootWindow, 0, mask, GrabModeAsync,
                 GrabModeAsync, None, W->cursor_move, CurrentTime);

    /* Make the offsets be the distance from the bottom/right of the window, not top/left */

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
        switcher(W);
    
    if (ev->xkey.state & (Mod1Mask | ShiftMask))
    {
        switch (sym)
        {
            case XK_f:
                client_togglefullscreen(W, C);
                break;
            /* Tiling. Passing -1 to moveresize is for maximising in that dimension */
            case XK_Up:
                client_moveresize(W, C, curr_head_x(W), curr_head_y(W), -1, curr_height(W) / 2 - 2 * B);
                break;
            case XK_Down:
                client_moveresize(W, C, curr_head_x(W), curr_head_y(W) + curr_height(W) / 2 - B,
                                  -1, curr_height(W) / 2 - B);
                break;
            case XK_Left:
                client_moveresize(W, C, curr_head_x(W), curr_head_y(W), curr_width(W) / 2 - 2 * B, -1);
                break;
            case XK_Right:
                client_moveresize(W, C, curr_head_x(W) + curr_width(W) / 2 - B,
                                  curr_head_y(W), curr_width(W) / 2 - B, -1);
                break;
            case XK_Return:
                launcher(W);
                redraw_root(W, NULL);
                break;
        }
    }
}

static void event_configure_request(struct WM_t *W, struct wmclient *C, XEvent *ev)
{
    XConfigureRequestEvent *conf = &(ev->xconfigurerequest);
    /*
    msg("ConfigureRequest: border = %d\n", conf->border_width);
    msg("    x, y = %d, %d\n", conf->x, conf->y);
    msg("    w, h = %d, %d\n", conf->width, conf->height);
    */

    if (C)
    {
        msg("ConfigureRequest from client \'%s\'\n", C->name);
        client_moveresize(W, C, conf->x, conf->y, conf->width, conf->height);
        refresh_current_head(W);
    }
    else
    {
        msg("ConfigureRequest from unknown window.\n");
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
        msg("Maximise request from \'%s\'\n", C->name);
        if (strncmp(a, NETWM_MAX_STATE, sizeof(NETWM_MAX_STATE)))
            client_togglefullscreen(W, C);
    }
    else
    {
        int i;
        msg("Unknown client message: ");
        msg("Atom name %s. Format %d\n", XGetAtomName(W->XDisplay, cm.message_type), cm.format);
        for (i = 0; i < 5; i++)
            msg("   %d : %s\n", i, XGetAtomName(W->XDisplay, cm.data.l[i]));
    }
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
                else if (ev.xany.window == W->rootWindow)
                {
                    W->curr_head = which_head(W, ev.xbutton.x, ev.xbutton.y);
                }
                break;
            case ConfigureNotify:
                break;
            case ConfigureRequest:
                event_configure_request(W, C, &ev);
                break;
            case MapRequest: /* Does not use CreateNotify */
                /* Don't register it again if it was just hiding for some reason */
                if (!C)
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

