#include "gui.h"
/*
  function txt_create_from_str

→ Text *
Parameters:
- char * set_str
- int max_len
- SDL_Rect * container
- TTF_Font * font
- SDL_Color txt_clr
- TextAlign align
- bool truncate
- SDL_Renderer * rend
Create a Text from an existing string. If the string is a pointer to a const char *, Text cannot be edited.

Text *txt_create_from_str(char *set_str, int max_len, SDL_Rect *container,
                          TTF_Font *font, SDL_Color txt_clr, TextAlign align,
                          bool truncate, SDL_Renderer *rend)
*/

/* Auto-sizing */
Textbox *textbox_create_from_str(
    char *set_str,
    int max_len,
    Layout *lt,
    TTF_Font *font,
    SDL_Color *bckgrnd_clr,
    SDL_Color *border_clr,
    SDL_Color *txt_clr,
    bool truncate,
    SDL_Renderer *rend
    
    )
{
    Textbox *tb = malloc(sizeof(Textbox));
    tb->layout = lt;
    Layout *text_layout = layout_add_child(lt);
    layout_set_name(text_layout, "text");
    
    tb->text = txt_create_from_str(
	set_str,
	max_len,
	&(text_layout->rect),
        font,
	*txt_clr,
	TextAlign CENTER,
	
	
	
	

}

    
