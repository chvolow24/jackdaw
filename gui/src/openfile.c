/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

#include "draw.h"
#include "layout.h"
#include "openfile.h"
#include "parse_xml.h"
#include "layout_xml.h"
#include "window.h"

extern Layout *openfile_lt;
extern OpenFile *openfile;
extern bool show_openfile;
//extern TTF_Font *open_sans;
extern Window *main_win;

extern Layout *main_lt;
extern SDL_Color color_global_white;

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
        openfile->label = txt_create_from_str("Open file at:", 14, &(layout_get_child_by_name_recursive(openfile_lt, "label")->rect), main_win->std_font, 12, color_global_white, CENTER_LEFT, true, main_win);
        openfile->filepath = txt_create_from_str(filepath_buffer, 254, &(layout_get_child_by_name_recursive(openfile_lt, "filepath")->rect), main_win->std_font, 12, color_global_white, CENTER_LEFT, true, main_win);
    }

    txt_edit (openfile->filepath, layout_draw_main);

    Layout *ret = layout_read_xml_to_lt(lt, openfile->filepath->display_value);


    if (ret != main_lt && ret->type == PRGRM_INTERNAL) make_editable(ret);

    // delete_layout(openfile_lt);
    show_openfile = false;
    layout_reset(ret);
    return ret;
}
