#include "layout.h"
#include "openfile.h"
#include "parse_xml.h"
#include "layout_xml.h"
#include "window.h"

extern Layout *openfile_lt;
extern OpenFile *openfile;
extern bool show_openfile;
extern TTF_Font *open_sans;
extern Window *main_win;

extern Layout *main_lt;

static void make_editable(Layout *lt) 
{
    lt->type = NORMAL;
    for (int i=0; i<lt->num_children; i++) {
        make_editable(lt->children[i]);
    }
}

extern SDL_Color white;
Layout *openfile_loop(Layout *lt) 
{
    static char filepath_buffer[255];
    if (!openfile) {
        openfile = malloc(sizeof(OpenFile));
        /* Layout *label_lt = get_child_recursive(openfile_lt, "label"); */
        openfile->label = create_text_from_str("Open file at:", 14, &(get_child_recursive(openfile_lt, "label")->rect), open_sans, white, CENTER_LEFT, true, main_win->rend);
        openfile->filepath = create_text_from_str(filepath_buffer, 254, &(get_child_recursive(openfile_lt, "filepath")->rect), open_sans, white, CENTER_LEFT, true, main_win->rend);
    }

    edit_text(openfile->filepath);

    Layout *ret = read_xml_to_lt(lt, openfile->filepath->display_value);


    if (ret != main_lt && ret->type == PRGRM_INTERNAL) make_editable(ret);

    // delete_layout(openfile_lt);
    show_openfile = false;
    reset_layout(ret);
    return ret;
}
