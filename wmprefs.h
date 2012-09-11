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

#ifndef WMPREFS_H
#define WMPREFS_H

struct wmprefs_t
{
    /* Border width. Short name because it's referenced so much */
    int         bw;
    /* Maximum distance for snapping to borders during window moves */
    int         snap_width;
    /* Number of characters in the alt+tab switcher window */
    int         switcher_char_width;

    int         focus_border_col[3], unfocus_border_col[3];
    int         fg_col[3], bg_col[3], root_bg_col[3];
    
    char        *launcher_font, *switcher_font;

};

void wmprefs_load_defaults(struct wmprefs_t *);
void wmprefs_read_config_files(struct wmprefs_t *);

#endif

