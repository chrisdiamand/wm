
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>

#include "xatoms.h"

struct X_Atom
{
    char    *name;
    Atom    val;
};

static struct X_Atom *all_atoms = NULL;
static size_t n_atoms = 0;

struct X_Atom *find_atom(struct WM_t *W, char *name)
{
    int i;
    for (i = 0; i < n_atoms; i++)
    {
        struct X_Atom *a = all_atoms + i;
        if (!strcmp(a->name, name))
            return a;
    }
    return NULL;
}

Atom get_atom(struct WM_t *W, char *name)
{
    struct X_Atom *a = find_atom(W, name);
    if (a)
        return a->val;
    /* It doesn't exist, so get it with XInternAtom and cache it */
    all_atoms = realloc(all_atoms, ++n_atoms * sizeof(struct X_Atom));
    a = all_atoms + n_atoms - 1;
    a->name = name;
    a->val = XInternAtom(W->XDisplay, name, False);
    return a->val;
}

/* Doesn't quite work... */
#if 0
void change_net_wm_state(struct WM_t *W, struct wmclient *C, char *name, int enable)
{
    Atom netwmstate = get_atom(W, "_NET_WM_STATE");
    Atom prop = get_atom(W, name);
    Atom actual_type_return;
    int actual_format_return, i, j;
    unsigned long nitems_return, bytes_after_return;
    Atom *prop_return, *actual_props;
    printf("Getting props\n");
    XGetWindowProperty(W->XDisplay, C->win, netwmstate, 0, 1, False, AnyPropertyType,
                       &actual_type_return, &actual_format_return,
                       &nitems_return, &bytes_after_return, (unsigned char **) &prop_return);
    printf("type=%s, format=%d, n=%lu, bytes=%lu, prop=%p\n",
           XGetAtomName(W->XDisplay, actual_type_return),
           actual_format_return, nitems_return, bytes_after_return, prop_return);
    actual_props = malloc(sizeof(Atom) * (nitems_return + 1));

    for (i = 0, j = 0; i < nitems_return; i++)
    {
        Atom a = prop_return[i];
        printf(" - %d: %s\n", i, XGetAtomName(W->XDisplay, a));
        if (a == prop && enable) /* It's already there so return */
            return;
        if (a == prop && !enable)
            ;
        else
            actual_props[j++] = a;
    }
    if (enable)
        actual_props[j++] = prop;
    XChangeProperty(W->XDisplay, C->win, get_atom(W, "_NET_WM_STATE"),
                    XA_ATOM, 32, PropModeReplace, (unsigned char *) actual_props, j);
}
#endif

