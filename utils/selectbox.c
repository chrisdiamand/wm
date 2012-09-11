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
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "selectbox.h"

#define BORDER_SIZE 5

#define EVENT_MASK (KeyPressMask    |   \
                    KeyReleaseMask  |   \
                    KeymapStateMask |   \
                    ExposureMask)

static void draw_item_text(struct selectbox_t *sb, char *text, int topcorner)
{
    int text_w = XTextWidth(sb->font, text, strlen(text));
    int text_h = sb->font->ascent + sb->font->descent;
    int x, y, text_len = strlen(text);

    if (text_w < sb->width && sb->centre) /* Centre it if it will fit */
        x = (sb->width - text_w) / 2;
    else
        x = BORDER_SIZE + 4; /* 4 is an arbitrary gap before the start of the text */

    y = topcorner + (sb->item_height + text_h) / 2 - sb->font->descent - 1;

    XDrawString(sb->wm->XDisplay, sb->win, sb->gc, x, y, text, text_len);
}

/* Use the font size and number of items to calculate
 * the size of the window */
static void calculate_height(struct selectbox_t *sb)
{
    XCharStruct max_char;
    if (!sb->font)
    {
        fprintf(stderr, "Error: No font loaded!\n");
        sb->height = 50;
        return;
    }

    max_char = sb->font->max_bounds;
    sb->item_height = (max_char.ascent + max_char.descent) * 1.5;
    sb->height = sb->item_height * sb->n_items + 2 * BORDER_SIZE;

    /* Adjust x and y if it's centered */
    if (sb->centre)
    {
        sb->x -= sb->width / 2;
        sb->y -= sb->height / 2;
    }
}

static void open_window(struct selectbox_t *sb)
{
    struct WM_t *W = sb->wm;
    /* Create the window */
    sb->win = XCreateSimpleWindow(W->XDisplay, W->rootWindow,
                                  sb->x, sb->y, sb->width, sb->height, 1,
                                  BlackPixel(W->XDisplay, W->XScreen),
                                  WhitePixel(W->XDisplay, W->XScreen));

    XSelectInput(W->XDisplay, sb->win, EVENT_MASK);

    sb->gc = XCreateGC(W->XDisplay, sb->win, 0, NULL);
    XSetFont(W->XDisplay, sb->gc, sb->font->fid);

    XMapWindow(W->XDisplay, sb->win);
    XRaiseWindow(W->XDisplay, sb->win);
}

struct selectbox_t *selectbox_new(struct WM_t *W, int x, int y, int width, int centre,
                                  char **items, int n_items, XFontStruct *font)
{
    struct selectbox_t *sb = malloc(sizeof(struct selectbox_t));
    sb->wm = W;
    sb->x = x;
    sb->y = y;
    sb->width = width;
    sb->centre = centre;
    sb->items = items;
    sb->n_items = n_items;
    sb->font = font;

    calculate_height(sb);
    open_window(sb);

    return sb;
}

void selectbox_draw(struct selectbox_t *sb, int selected)
{
    int i;
    struct WM_t *W = sb->wm;

    /* Draw a grey background */
    XSetForeground(W->XDisplay, sb->gc, W->bg_col);
    XFillRectangle(W->XDisplay, sb->win, sb->gc, 0, 0, sb->width, sb->height);
    XSetForeground(W->XDisplay, sb->gc, W->fg_col);

    /* Draw the items */
    for (i = 0; i < sb->n_items; i++)
    {
        if (i == selected) /* If it's selected swap FG and BG colours */
        {
            XSetForeground(W->XDisplay, sb->gc, W->fg_col);
            XFillRectangle(W->XDisplay, sb->win, sb->gc,
                           BORDER_SIZE, sb->item_height * i + BORDER_SIZE,
                           sb->width - 2 * BORDER_SIZE, sb->item_height);
            XSetForeground(W->XDisplay, sb->gc, W->bg_col);
        }
        draw_item_text(sb, sb->items[i], sb->item_height * i + BORDER_SIZE);
        /* Restore fg_col text as the next one drawn will be unselected */
        if (i == selected)
            XSetForeground(W->XDisplay, sb->gc, W->fg_col);
    }
}

void selectbox_close(struct selectbox_t *sb)
{
    Display *xd = sb->wm->XDisplay;
    XFreeGC(xd, sb->gc);
    XUnmapWindow(xd, sb->win);
    XDestroyWindow(xd, sb->win);
    free(sb);
}

