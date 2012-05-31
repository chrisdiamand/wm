
#ifndef WMPREFS_H
#define WMPREFS_H

struct wmprefs_t
{
    /* Border width. Short name because it's referenced so much */
    int         bw;
    /* Maximum distance for snapping to borders during window moves */
    int         snapwidth;

    int         focus_border_r, focus_border_g, focus_border_b;
    int         launcher_bg_r, launcher_bg_g, launcher_bg_b;
    
    char        *launcher_font, *alttab_font;
    int         alttab_char_width;

};

void wmprefs_load_defaults(struct wmprefs_t *);
void wmprefs_read_config_files(struct wmprefs_t *);

#endif

