
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

void rc_add_int_option(struct rc_t *, char *, int *);
void rc_add_colour_option(struct rc_t *, char *, int *);
void rc_add_string_option(struct rc_t *, char *, char **);
void rc_add_bool_option(struct rc_t *, char *, int *);

void rc_read_file(struct rc_t *, char *);

#endif

