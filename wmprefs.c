
#include <stdio.h>
#include <stdlib.h>

#include "rc.h"
#include "wmprefs.h"

void wmprefs_load_defaults(struct wmprefs_t *p)
{
    p->focus_border_r = 0;
    p->focus_border_g = 180;
    p->focus_border_b = 0;

    p->launcher_bg_r = 205;
    p->launcher_bg_r = 205;
    p->launcher_bg_r = 205;
    p->launcher_font = "*-courier*standard*-14-*";

    p->alttab_font = "*-courier*standard*-14-*";
    p->alttab_char_width = 50;

    p->bw = 1;
    p->snapwidth = 15;
}

void wmprefs_read_config_files(struct wmprefs_t *p)
{
    ;
}

