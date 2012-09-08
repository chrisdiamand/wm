
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils/selectbox.h"
#include "switcher.h"
#include "wm.h"

#define AT_BORDER 5
#define EVENT_MASK (KeyPressMask | KeyReleaseMask | KeymapStateMask | ExposureMask)

static struct selectbox_t *sb = NULL;
static XFontStruct *switcher_font = NULL;
static int currently_selected = 0;

static int switcher_key_event(struct WM_t *W, XEvent *ev)
{
    KeySym sym = XLookupKeysym(&(ev->xkey), 0);

    /* Alt released so focus the selected window and quit the switcher */
    if (!(ev->xkey.state & Mod1Mask) ||
        (ev->type == KeyRelease && sym == XK_Alt_L))
    {
        client_focus(W, W->clients[currently_selected]);
        return -1;
    }

    switch (sym)
    {
        case XK_Tab:
            if (ev->type == KeyPress)
            {
                if (ev->xkey.state & ShiftMask)
                    currently_selected--;
                else
                    currently_selected++;

                /* Wrap around */
                if (currently_selected >= W->nclients)
                    currently_selected = 0;
                if (currently_selected < 0)
                    currently_selected = W->nclients - 1;

                selectbox_draw(sb, currently_selected);
            }
            break;
        /* If escape pressed don't change the focus */
        case XK_Escape:
            return -1;
    }
    return 0;
}

static void switcher_events(struct WM_t *W)
{
    XEvent ev;

    while (1)
    {
        XMaskEvent(W->XDisplay, EVENT_MASK, &ev);
        printf("Event: %s, window %lu\n", event_name(ev.type), ev.xany.window);
        switch (ev.type)
        {
            case KeyPress:
            case KeyRelease:
                if (switcher_key_event(W, &ev) == -1)
                    return;
                break;
            case Expose:
                selectbox_draw(sb, currently_selected);
                break;
        }
    }
}

static char **make_item_list(struct WM_t *W)
{
    char **ret;
    int i;

    if (W->nclients < 0)
        return NULL;

    ret = (char **) malloc(W->nclients * sizeof(char *));
    for (i = 0; i < W->nclients; i++)
        ret[i] = W->clients[i]->name;

    return ret;
}

static void switcher_show(struct WM_t *W)
{
    char **namelist = make_item_list(W);

    if (W->nclients > 1)
        currently_selected = 1;
    else
        currently_selected = 0;

    sb = selectbox_new(W, W->rW / 2, W->rH / 2, 200, 1,
                       namelist, W->nclients, switcher_font);
    XSetInputFocus(W->XDisplay, sb->win, RevertToPointerRoot, CurrentTime);
}

static void switcher_hide(struct WM_t *W)
{
    if (W->clients[0])
        client_focus(W, W->clients[0]);

    free(sb->items);
    selectbox_close(sb);
    sb = NULL;
}

void switcher(struct WM_t *W)
{
    if (W->nclients > 1)
        currently_selected = 1;
    else
        currently_selected = 0;

    switcher_show(W);
    switcher_events(W);
    switcher_hide(W);
}

void switcher_init(struct WM_t *W)
{
    /* Load the font */
    switcher_font = XLoadQueryFont(W->XDisplay, W->prefs.switcher_font);
    if (!switcher_font)
    {
        msg("Couldn't load switcher font \'%s\', using \'fixed\' instead.\n",
            W->prefs.switcher_font);
        switcher_font = XLoadQueryFont(W->XDisplay, "fixed");
        assert(switcher_font);
    }
}

