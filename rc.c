
#include <stdio.h>
#include <stdlib.h>

#include "rc.h"

void rc_add_int_option(struct rc_t *S, char *name, int *ptr)
{
    struct pref_t p;

    if (S->nprefs >= MAX_PREFS)
    {
        fprintf(stderr, "Max preferences reached!\n");
        return;
    }

    p.type = PREF_INT;
    p.name = name;
    p.v.i = ptr;
    S->prefs[S->nprefs++] = p;
}

void rc_add_string_option(struct rc_t *S, char *name, char **ptr)
{
    struct pref_t p;

    if (S->nprefs >= MAX_PREFS)
    {
        fprintf(stderr, "Max preferences reached!\n");
        return;
    }

    p.type = PREF_INT;
    p.name = name;
    p.v.str = ptr;
    S->prefs[S->nprefs++] = p;
}

void rc_add_bool_option(struct rc_t *S, char *name, int *ptr)
{
    struct pref_t p;

    if (S->nprefs >= MAX_PREFS)
    {
        fprintf(stderr, "Max preferences reached!\n");
        return;
    }

    p.type = PREF_INT;
    p.name = name;
    p.v.onoff = ptr;
    S->prefs[S->nprefs++] = p;
}

struct rc_t *rc_init(void)
{
    struct rc_t *S = calloc(1, sizeof(*S));
    S->nprefs = 0;
    return S;
}

