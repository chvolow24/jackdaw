/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "color.h"
#include "draw.h"
#include "layout.h"
#include "openfile.h"
#include "layout_xml.h"
#include "window.h"

extern Layout *openfile_lt;
extern OpenFile *openfile;
extern bool show_openfile;
//extern TTF_Font *open_sans;
extern Window *main_win;

extern Layout *main_lt;
extern struct colors colors;


static void make_editable(Layout *lt) 
{
    lt->type = NORMAL;
    for (int i=0; i<lt->num_children; i++) {
        make_editable(lt->children[i]);
    }
}

Layout *openfile_loop(Layout *lt) 
{
    /* TTF_Font *open_sans_12 = ttf_get_font_at_size(main_win->std_font, 12); */
    static char filepath_buffer[255];
    if (!openfile) {
        openfile = malloc(sizeof(OpenFile));
        /* Layout *label_lt = layout_get_child_by_name_recursive(openfile_lt, "label"); */
        openfile->label = txt_create_from_str("Open file at:", 14, layout_get_child_by_name_recursive(openfile_lt, "label"), main_win->std_font, 12, colors.white, CENTER_LEFT, true, main_win);
        openfile->filepath = txt_create_from_str(filepath_buffer, 254, layout_get_child_by_name_recursive(openfile_lt, "filepath"), main_win->std_font, 12, colors.white, CENTER_LEFT, true, main_win);
    }

    txt_edit (openfile->filepath, layout_draw_main);

    Layout *ret = layout_read_xml_to_lt(lt, openfile->filepath->display_value);


    if (ret != main_lt && ret->type == PRGRM_INTERNAL) make_editable(ret);

    // delete_layout(openfile_lt);
    show_openfile = false;
    layout_reset(ret);
    return ret;
}
