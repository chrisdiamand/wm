
#define _GNU_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launcher.h"
#include "menuitems.h"
#include "read_desktop_file.h"
#include "../wm.h"
#include "../utils/list.h"

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
                M = malloc(sizeof(struct menuitem_t));
                read_desktop_file( fullpath, &(M->name), &(M->descr), &(M->exec) );
                List_push_back(all_menu_items, M);
                break;
        }
        free(fullpath);
    }
    closedir(dp);
}

/* Find menu items for which the name, description or exec contains str */
struct List *menuitems_match(char *str)
{
    unsigned int i;
    struct List *L = List_new();
    for (i = 0; i < all_menu_items->size; i++)
    {
        struct menuitem_t *M = all_menu_items->items[i];
        if (strcasestr(M->name, str))
            List_push_back(L, M);
        else if (strcasestr(M->descr, str))
            List_push_back(L, M);
        else if (strcasestr(M->exec, str))
            List_push_back(L, M);
    }
    return L;
}

/* FIXME: Get .desktop directories from .wmrc */
void menuitems_scan(struct WM_t *W)
{
    all_menu_items = List_new();
    scan_applications_dir("/usr/share/applications");
}

