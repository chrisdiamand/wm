
#include <stdio.h>
#include <stdlib.h>

#include "rc.h"

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
    struct rc_t *S = calloc(1, sizeof(*S));
    S->nprefs = 0;
    return S;
}

void rc_parse(struct rc_t *R, char *fname)
{

}

