
#ifndef RC_H
#define RC_H

#define MAX_PREFS 4

typedef enum
{
    PREF_INT,
    PREF_STRING,
    PREF_BOOL
} pref_type;

struct pref_t
{
    pref_type       type;
    char            *name;
    union
    {
        int         *i;
        char        **str;
        int         *onoff;
    } v;
};

struct rc_t
{
    struct pref_t   prefs[MAX_PREFS];
    int             nprefs;
};

#endif

