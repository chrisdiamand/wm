
#ifndef XATOMS_H
#define XATOMS_H

#include <X11/Xlib.h>
#include "wm.h"

Atom get_atom(struct WM_t *, char *);
void change_net_wm_state(struct WM_t *, struct wmclient *, char *, int);

#endif

