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

#ifndef LAUNCHER_H
#define LAUNCHER_H

#define LAUNCHER_MAX_STRLEN 128

#include <X11/Xlib.h>

#include "list.h"
#include "selectbox.h"

struct WM_t;

struct launcher_t
{
    Window              win;
    int                 height;
    GC                  gc;
    unsigned long       inputeventmask;

    int                 len;
    char                str[LAUNCHER_MAX_STRLEN];

    struct List         *suggestions;
    int                 selected;

    struct selectbox_t  *sb;
    XFontStruct         *font;
};

void launcher_init(struct WM_t *);
void launcher(struct WM_t *);

#endif

