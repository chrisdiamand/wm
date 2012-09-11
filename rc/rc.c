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

#include "parser.h"
#include "rc.h"
#include "scanner.h"

static void add_option(struct rc_t *S, char *name, pref_type type, void *ptr)
{
    struct pref_t p;

    if (S->nprefs >= MAX_PREFS)
    {
        fprintf(stderr, "Max preferences reached!\n");
        return;
    }

    p.type = type;
    p.name = name;
    p.v.i = ptr;
    S->prefs[S->nprefs++] = p;
}
 
void rc_add_int_option(struct rc_t *S, char *name, int *ptr)
    {   add_option(S, name, PREF_INT, (void *) ptr);    }
 
void rc_add_colour_option(struct rc_t *S, char *name, int *col)
    {   add_option(S, name, PREF_COL, (void *) col);    }
  
void rc_add_string_option(struct rc_t *S, char *name, char **ptr)
    {   add_option(S, name, PREF_STR, (void *) ptr);    }

void rc_add_bool_option(struct rc_t *S, char *name, int *ptr)
    {   add_option(S, name, PREF_BOOL, (void *) ptr);   }

struct rc_t *rc_init(void)
{
    struct rc_t *R = calloc(1, sizeof(struct rc_t));
    R->nprefs = 0;
    return R;
}

void rc_free(struct rc_t *R)
{
    free(R);
}

void rc_read_file(struct rc_t *R, char *fname)
{
    FILE *fp = fopen(fname, "r");
    ScannerInput *I;
    if (!fp)
    {
        perror(fname);
        return;
    }
    printf("Parsing %s\n", fname);
    I = ScannerInputFile(fp);
    rcp_parse(R, I);
}

