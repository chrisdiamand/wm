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

#ifndef RC_H
#define RC_H

#define MAX_PREFS 16

typedef enum
{
    PREF_INT,
    PREF_COL,
    PREF_STR,
    PREF_BOOL
} pref_type;

struct pref_t
{
    pref_type       type;
    char            *name;
    union
    {
        int         *i;
        int         *col;
        char        **str;
        int         *onoff;
    } v;
};

struct rc_t
{
    struct pref_t   prefs[MAX_PREFS];
    int             nprefs;
};

struct rc_t *rc_init(void);
void rc_free(struct rc_t *);

void rc_add_int_option(struct rc_t *, char *, int *);
void rc_add_colour_option(struct rc_t *, char *, int *);
void rc_add_string_option(struct rc_t *, char *, char **);
void rc_add_bool_option(struct rc_t *, char *, int *);

void rc_read_file(struct rc_t *, char *);

#endif

