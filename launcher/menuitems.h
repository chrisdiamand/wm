
#ifndef MENUITEMS_H
#define MENUITEMS_H

#include "../wm.h"

struct menuitem_t
{
    char    *name;
    char    *descr;
    char    *exec;
};

void menuitems_scan(struct WM_t *);
struct List *menuitems_match(char *);

#endif

