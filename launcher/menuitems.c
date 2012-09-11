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

#define _GNU_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launcher.h"
#include "list.h"
#include "menuitems.h"
#include "read_desktop_file.h"
#include "wm.h"

/* A list of struct menuitem_t * of all the menu items */
static struct List *all_menu_items = NULL;

/* Recursively scan through the given directory
 * looking for .desktop files */
static void scan_applications_dir(char *path)
{
    DIR *dp = opendir(path);
    struct dirent *entry;
    struct menuitem_t *M;

    if (!dp)
    {
        perror(path);
        return;
    }

    while ( (entry = readdir(dp)) )
    {
        char *n = entry->d_name, *fullpath;
        if (!strcmp(n, "..") || !strcmp(n, "."))
            continue;
        fullpath = malloc( sizeof(char) * (strlen(n) + strlen(path) + 2) );
        sprintf(fullpath, "%s/%s", path, n);
        switch (entry->d_type)
        {
            case DT_DIR: /* Recursively scan directories */
                scan_applications_dir(fullpath);
                break;
            case DT_REG: /* A regular file */
                M = calloc(1, sizeof(struct menuitem_t));
                read_desktop_file( fullpath, &(M->name), &(M->descr), &(M->exec) );

                if ( (!M->name && !M->descr) || (!M->exec) )
                {
                    free(M);
                    M = NULL;
                }
                else if (!M->name && M->descr)
                {
                    M->name = M->descr;
                    M->descr = NULL;
                }
                if (M)
                    List_push_back(all_menu_items, M);
                break;
        }
        free(fullpath);
    }
    closedir(dp);
}

/* Find menu items for which the name, description or exec contains str.
 * Return a list of struct menuitem_t * */
struct List *menuitems_match(char *str)
{
    unsigned int i;
    struct List *L = List_new();
    for (i = 0; i < all_menu_items->size; i++)
    {
        struct menuitem_t *M = all_menu_items->items[i];
        if (!M->name)
            msg("BLANK NAME!\n");
        if (strcasestr(M->name, str))
            List_push_back(L, M);
        else if (M->descr)
        {
            if (strcasestr(M->descr, str))
                List_push_back(L, M);
        }
    }
    return L;
}

/* FIXME: Get .desktop directories from .wmrc */
void menuitems_scan(struct WM_t *W)
{
    all_menu_items = List_new();
    scan_applications_dir("/usr/share/applications");
}

