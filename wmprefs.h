
#ifndef WMPREFS_H
#define WMPREFS_H

struct wmprefs_t
{
    /* Border width. Short name because it's referenced so much */
    int         bw;
    /* Maximum distance for snapping to borders during window moves */
    int         snap_width;

    int         focus_border_col[3], unfocus_border_col[3];
    int         fg_col[3], bg_col[3];
    
    char        *launcher_font, *switcher_font;
    int         alttab_char_width;

};

void wmprefs_load_defaults(struct wmprefs_t *);
void wmprefs_read_config_files(struct wmprefs_t *);

#endif

