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

#ifndef SELECTBOX_H
#define SELECTBOX_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "wm.h"

struct selectbox_t
{
    struct          WM_t *wm;
    Window          win;
    XFontStruct     *font;
    GC              gc;

    int             x, y;
    int             width, height;
    int             item_height;

    int             centre;

    char            **items;
    int             n_items;
};

/* X, Y, width, is_centered, items, n_items */
struct selectbox_t *selectbox_new(struct WM_t *, int, int, int, int, char **, int, XFontStruct *);

/* Select box instance, item to draw selected (-1 for none) */
void selectbox_draw(struct selectbox_t *, int);

void selectbox_close(struct selectbox_t *);

#endif

