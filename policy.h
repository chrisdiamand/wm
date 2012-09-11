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

#ifndef POLICY_H
#define POLICY_H

#include <X11/Xlib.h>
#include "wm.h"

void decide_new_window_size_pos(struct WM_t *, struct wmclient *);
int which_head(struct WM_t *, int, int);
void refresh_current_head(struct WM_t *);

#endif

