#include "geometry.h"
#include "textbox.h"
#include "layout.h"

#define TXTBX_DEFAULT_RAD 0
#define TXTBX_DEFAULT_MAXLEN 255


SDL_Color textbox_default_bckgrnd_clr = (SDL_Color) {240, 240, 240, 255};
SDL_Color textbox_default_txt_clr = (SDL_Color) {0, 0, 0, 255};
SDL_Color textbox_default_border_clr = (SDL_Color) {100, 100, 100, 255};
int textbox_default_radius = 0;

/*txt_create_from_str(char *set_str, int max_len, SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend) -> Text **/
Textbox *textbox_create_from_str(
    char *set_str,
    Layout *lt,
    TTF_Font *font,
    SDL_Renderer *rend
    )
{
    Textbox *tb = malloc(sizeof(Textbox));
    tb->layout = lt;
    tb->bckgrnd_clr = &textbox_default_bckgrnd_clr;
    tb->border_clr = &textbox_default_border_clr;
    tb->corner_radius = textbox_default_radius;
    tb->text = txt_create_from_str(
	set_str,
	TXTBX_DEFAULT_MAXLEN,
	&(lt->rect),
	font,
	textbox_default_txt_clr,
	CENTER,
	true,
	rend
	);

	
    return tb;
}

/* Pad a textbox on all sides. Modifies the dimensions of the layout */
void textbox_pad(Textbox *tb, int pad)
{
    SDL_Rect *lt_rect = &tb->layout->rect;
    SDL_Rect *txt_rect = &tb->text->text_rect;
    lt_rect->w = txt_rect->w + (2 * pad);
    lt_rect->h = txt_rect->h + (2 * pad);
    layout_set_values_from_rect(tb->layout);
}

void textbox_destroy(Textbox *tb)
{
    txt_destroy(tb->text);
    free(tb);
}

void textbox_draw(Textbox *tb)
{
    /* TODO: consider whether this is bad and if rend needs to be on textbox as well */
    SDL_Renderer *rend = tb->text->rend;
    SDL_Color *bckgrnd = tb->bckgrnd_clr;
    SDL_Color *txtclr = &tb->text->color;
    SDL_Color *brdrclr = tb->border_clr;
    SDL_Rect tb_rect = tb->layout->rect;
    SDL_SetRenderDrawColor(rend, bckgrnd->r, bckgrnd->g, bckgrnd->b, bckgrnd->a);
    if (tb->corner_radius > 0) {
	int halfdim = tb_rect.w < tb_rect.h ? tb_rect.w >> 1 : tb_rect.h >>1;
	int rad = halfdim < tb->corner_radius ? halfdim : tb->corner_radius;
	geom_fill_rounded_rect(rend, &tb_rect, rad);
	SDL_SetRenderDrawColor(rend, brdrclr->r, brdrclr->g, brdrclr->b, brdrclr->a);
	for (int i=0; i<tb->border_thickness; i++) {	    
	    geom_draw_rounded_rect(rend, &tb_rect, rad);
	    tb_rect.x += 1;
	    tb_rect.y += 1;
	    tb_rect.w -= 2;
	    tb_rect.h -= 2;
	    rad -= 1;
	}
	    
    } else {
	SDL_RenderFillRect(rend, &tb_rect);
	SDL_SetRenderDrawColor(rend, brdrclr->r, brdrclr->g, brdrclr->b, brdrclr->a);
	for (int i=0; i<tb->border_thickness; i++) {
	    SDL_RenderDrawRect(rend, &tb_rect);
	    tb_rect.x += 1;
	    tb_rect.y += 1;
	    tb_rect.w -= 2;
	    tb_rect.h -= 2;
	}
    }
    SDL_SetRenderDrawColor(rend, txtclr->r, txtclr->g, txtclr->b, txtclr->a);
    txt_draw(tb->text);
}




