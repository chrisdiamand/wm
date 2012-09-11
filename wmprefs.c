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

#include "rc.h"
#include "wmprefs.h"

/* Set the default values */
void wmprefs_load_defaults(struct wmprefs_t *p)
{
    printf("Loading defaults\n");
    p->focus_border_col[0] = 0;
    p->focus_border_col[1] = 180;
    p->focus_border_col[2] = 0;

    p->unfocus_border_col[0] = 0;
    p->unfocus_border_col[1] = 0;
    p->unfocus_border_col[2] = 0;

    p->fg_col[0] = 0;
    p->fg_col[1] = 0;
    p->fg_col[2] = 0;

    p->bg_col[0] = 215;
    p->bg_col[1] = 215;
    p->bg_col[2] = 215;

    p->root_bg_col[0] = 179;
    p->root_bg_col[1] = 179;
    p->root_bg_col[2] = 204;

    p->launcher_font = "*-courier*standard*-14-*";

    p->switcher_font = "*-courier*standard*-14-*";
    p->switcher_char_width = 50;

    p->bw = 1;
    p->snap_width = 15;
}

/* Tell the config file parser what options there are */
static void init_rc_options(struct wmprefs_t *p, struct rc_t *R)
{
    rc_add_int_option(R, "border_width", &(p->bw));
    rc_add_int_option(R, "snap_width", &(p->snap_width));
    rc_add_int_option(R, "switcher_char_width", &(p->switcher_char_width));

    rc_add_colour_option(R, "root_bg_col", p->root_bg_col);
    rc_add_colour_option(R, "focus_border_col", p->focus_border_col);
    rc_add_colour_option(R, "unfocus_border_col", p->unfocus_border_col);
    rc_add_colour_option(R, "fg_col", p->fg_col);
    rc_add_colour_option(R, "bg_col", p->bg_col);

    rc_add_string_option(R, "launcher_font", &(p->launcher_font));
    rc_add_string_option(R, "switcher_font", &(p->switcher_font));

}

void wmprefs_read_config_files(struct wmprefs_t *p)
{
    struct rc_t *R = rc_init();
    init_rc_options(p, R);
    rc_read_file(R, "wmrc");

    rc_free(R);
}

