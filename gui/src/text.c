#include "draw.h"
#include "text.h"
#include "layout.h"

#define CURSOR_WIDTH 4

extern Layout *main_layout;
extern Window *main_win;
// extern TTF_Font *open_sans;

static Text *create_empty_text(
    SDL_Rect *container, 
    TTF_Font *font, 
    SDL_Color txt_clr, 
    TextAlign align,
    bool truncate,
    SDL_Renderer *rend
)
{
    Text *txt = malloc(sizeof(Text));
    txt->value_handle = NULL;
    txt->len = 0;
    txt->max_len = 0;
    txt->container = container;

    txt->font = font;
    txt->color = txt_clr;
    txt->truncate = false;

    txt->cursor_countdown = 0;
    txt->cursor_end_pos = 0;
    txt->cursor_start_pos = 0;
    txt->show_cursor = false;

    txt->rend = rend;
    txt->surface = NULL;
    txt->texture = NULL;
    return txt;
}

void reset_text_drawable(Text *txt) 
{
    if (txt->surface) {
        SDL_FreeSurface(txt->surface);
    }
    if (txt->texture) {
        SDL_DestroyTexture(txt->texture);
    }
    fprintf(stderr, "\tCreating surface from disp val %s\n", txt->display_value);
    txt->surface = TTF_RenderUTF8_Blended(txt->font, txt->display_value, txt->color);
    if (!txt->surface) {
        fprintf(stderr, "\nError: TTF_RenderText_Blended failed: %s", TTF_GetError());
        return;
    }
    txt->texture = SDL_CreateTextureFromSurface(txt->rend, txt->surface);
    if (!txt->texture) {
        fprintf(stderr, "\nError: SDL_CreateTextureFromSurface failed: %s", TTF_GetError());
        return;
    }
    fprintf(stderr, "txt rect before: %d, %d, %d, %d\n", txt->text_rect.x, txt->text_rect.y, txt->text_rect.w, txt->text_rect.h);
    fprintf(stderr, "txt container: %d %d %d %d\n", txt->container->x, txt->container->y, txt->container->w, txt->container->h);
    SDL_QueryTexture(txt->texture, NULL, NULL, &(txt->text_rect.w), &(txt->text_rect.h));

    switch (txt->align) {
        case CENTER:
            txt->text_rect.x = (int)((float) txt->container->w / 2.0 - (float) txt->text_rect.w / 2.0) + txt->container->x;
            txt->text_rect.y = (int)((float) txt->container->h / 2.0 - (float) txt->text_rect.w / 2.0) + txt->container->y;
            break;
        case TOP_LEFT:
            txt->text_rect.x = txt->container->x;
            txt->text_rect.y = txt->container->y;
            break;
        case TOP_RIGHT:
            txt->text_rect.x = txt->container->x + txt->container->w - txt->text_rect.w;
            txt->text_rect.y = txt->container->y;
            break;
        case BOTTOM_LEFT:
            txt->text_rect.x = txt->container->x;
            txt->text_rect.y = txt->container->y + txt->container->h- txt->text_rect.h;
            break;
        case BOTTOM_RIGHT:
            txt->text_rect.x = txt->container->x + txt->container->w - txt->text_rect.w;
            txt->text_rect.y = txt->container->y + txt->container->h- txt->text_rect.h;
            break;
        case CENTER_LEFT:
            txt->text_rect.x = txt->container->x;
            txt->text_rect.y = (int)((float) txt->container->h / 2.0 - (float) txt->text_rect.w / 2.0) + txt->container->y;
            break;
    }
    fprintf(stderr, "txt rect after: %d, %d, %d, %d\n", txt->text_rect.x, txt->text_rect.y, txt->text_rect.w, txt->text_rect.h);

}
// Text *create_text_from_str(char *value, uint8_t max_len, int x, int y, SDL_Color txt_clr);
void set_text_value(Text *txt, char *set_str)
{
    fprintf(stderr, "Set text value from %s to %s\n", txt->display_value, set_str);
    txt->value_handle = set_str;
    txt->len = strlen(set_str);

    int txtw, txth;

    TTF_SizeUTF8(txt->font, txt->value_handle, &txtw, &txth);

    if (txt->truncate && txt->text_rect.w > txt->container->w) {

        int approx_allowable_chars = (int) ((float) txt->len * txt->container->w / txtw);
        // int ellipsisw;
        // TTF_SizeUTF8(txt->font, "...", &ellipsisw, NULL);
        for (int i=0; i<approx_allowable_chars - 3; i++) {
            txt->display_value[i] = txt->value_handle[i];
        }
        for (int i=approx_allowable_chars - 3; i<approx_allowable_chars; i++) {
            txt->display_value[i] = '.';
        }
        txt->display_value[approx_allowable_chars] = '\0';
    } else {
        strcpy(txt->display_value, txt->value_handle);
    }

    reset_text_drawable(txt);
}

Text *create_text_from_str(
    char *set_str,
    int max_len,
    SDL_Rect *container,
    TTF_Font *font,
    SDL_Color txt_clr,
    TextAlign align,
    bool truncate,
    SDL_Renderer *rend
)
{
    fprintf(stderr, "Create text from string\n");
    Text *txt = create_empty_text(container, font, txt_clr, align, truncate, rend);
    fprintf(stderr, "Created empty text\n");

    txt->max_len = max_len;
    if (set_str) {
        set_text_value(txt, set_str);
    }
    fprintf(stderr, "\t-> done Create text from string\n");

    return txt;

}

static void cursor_left(Text *txt)
{
    if (txt->cursor_start_pos > 0) {
        txt->cursor_start_pos--;
    }
    txt->cursor_end_pos = txt->cursor_start_pos;  
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;

}

static void cursor_right(Text *txt)
{
    if (txt->cursor_start_pos < txt->len) {
        txt->cursor_start_pos++;
    }
    txt->cursor_end_pos = txt->cursor_start_pos;  
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;


}

static int handle_char(Text *txt, char input)
{
    fprintf(stderr, "Handle char %c; cursor %d-%d, txt before: %s, ", input, txt->cursor_start_pos, txt->cursor_end_pos, txt->display_value);
    char *val = txt->display_value;
    int displace;
    // fprintf(stderr, "cursor_end: %d, txt len: %d, max len: %d, last char: %c\n", txt->cursor_end_pos, txt->len, txt->max_len, txt->value[txt->len]);
    if ((displace = 1 - (txt->cursor_end_pos - txt->cursor_start_pos)) + txt->len < txt->max_len) {
        if (displace == 1) {
            for (int i = txt->len + 1; i > txt->cursor_end_pos; i--) {
                val[i] = val[i-1];
            }
            val[txt->cursor_end_pos] = input;
            txt->cursor_start_pos++;
            txt->cursor_end_pos = txt->cursor_start_pos;
            txt->len++;
        } else if (displace < 0) {
            for (int i = txt->cursor_end_pos; i > txt->len + 1; i++) {
                val[i - displace] = val[i];
            }
            val[txt->cursor_start_pos] = input;
            txt->cursor_start_pos++;
            txt->cursor_end_pos = txt->cursor_start_pos;
            txt->len+=displace;
            //TODO
        } else {
            val[txt->cursor_start_pos] = input;
            txt->cursor_start_pos++;
        }
        // print_tb(tb);
    } else return -1;
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;
    fprintf(stderr, "txt after: %s\n", txt->display_value);
    return 1;
}

static void handle_backspace(Text *txt) 
{
    char *val = txt->display_value;
    int displace = -1 * (txt->cursor_end_pos - txt->cursor_start_pos);
    if (displace == 0) {
        displace = -1;
    }
    fprintf(stderr, "Handle backspace, orig value \"%s\", displace = %d", txt->display_value, displace);

    for (int i=txt->cursor_start_pos; i<txt->len + displace + 1; i++) {
        val[i] = val[i - displace];
    }
    txt->len += displace;
    if (displace == -1) {
        txt->cursor_start_pos--;
    }
    txt->cursor_end_pos = txt->cursor_start_pos;
    fprintf(stderr, " new value \"%s\"\n", txt->display_value);

    //  if (txt->cursor_end_pos > txt->cursor_start_pos) {
    //     txt->display_value[txt->cursor_start_pos] = '\0';
    //     txt->len -= (txt->cursor_end_pos - txt->cursor_start_pos);
    //     txt->cursor_end_pos = txt->len;
    //     txt->cursor_start_pos = txt->len;
    // } else if (txt->cursor_end_pos > 0) {
    //     while (txt->cursor_end_pos < txt->len + 1) {
    //         txt->value_handle[txt->cursor_end_pos - 1] = txt->value_handle[txt->cursor_end_pos];
    //         txt->cursor_end_pos++;

    //     }

    //     txt->cursor_start_pos--;
    //     txt->cursor_end_pos = txt->cursor_start_pos;
    //     txt->len--;
    // }
    txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;
    // print_tb(tb);

}

void edit_text(Text *txt)
{
    fprintf(stderr, "Edit txt %p\n", txt);
    bool save_truncate = txt->truncate;
    txt->truncate = false;
    fprintf(stderr, "EDIT Setting value from %s to %s\n", txt->display_value, txt->value_handle);
    set_text_value(txt, txt->value_handle);
    txt->show_cursor = true;
    txt->cursor_start_pos = 0;
    txt->cursor_end_pos = txt->len;
    bool exit = false;
    // bool mousedown = false;
    // bool cmdctrldown = false;
    bool shiftdown = false;
    SDL_StartTextInput();
    while (!exit) {
        // get_mousep(main_win, &mousep);

        SDL_Event e;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT 
                || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) 
                || e.type == SDL_MOUSEBUTTONDOWN) {
                exit = true;
                /* Push the event back to the main event stack, so it can be handled in main.c */
                SDL_PushEvent(&e);
            } else if (e.type == SDL_TEXTINPUT) {
                handle_char(txt, e.text.text[0]);
                reset_text_drawable(txt);
                fprintf(stderr, "Text value: %s\n", txt->display_value);            
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_KP_ENTER:
                        exit = true;
                        break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shiftdown = true;
                        break;
                    case SDL_SCANCODE_LEFT:
                        cursor_left(txt);
                        break;
                    case SDL_SCANCODE_RIGHT:
                        cursor_right(txt);
                        break;
                    case SDL_SCANCODE_BACKSPACE:
                        handle_backspace(txt);
                        reset_text_drawable(txt);
                        // set_text_value(txt, txt->display_value);
                        break;
                    case SDL_SCANCODE_SPACE:
                        handle_char(txt, ' ');
                        reset_text_drawable(txt);
                        // set_text_value(txt, txt->display_value);
                        break;
                    default: {
                        break;
                    }

                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shiftdown = false;
                        break;
                    default: 
                        break;
                }
            }
        }
        // fprintf(stderr, "About to call draw %p %p\n", main_win, txt);
        draw_main();
        if (txt->cursor_countdown == 0) {
            txt->cursor_countdown = CURSOR_COUNTDOWN_MAX;
        } else {
            txt->cursor_countdown--;
        }
        // draw_textxtox(main_win, txt);
        SDL_Delay(1);

    }
    SDL_StopTextInput();
    txt->show_cursor = false;
    txt->truncate = save_truncate;
    set_text_value(txt, txt->display_value);
    // draw_main();
}

void destroy_text(Text *txt)
{
    if (txt->surface) {
        free(txt->surface);
    }
    if (txt->texture) {
        free(txt->texture);
    }
    free(txt);
}

TTF_Font *open_font(const char* path, int size, Window *win)
{
    size *= win->dpi_scale_factor;
    TTF_Font *font = TTF_OpenFont(path, size);
    if (!font) {
        fprintf(stderr, "\nError: failed to open font: %s", TTF_GetError());
        exit(1);
    }
    return font;
}

void destroy_font(TTF_Font *font)
{
    free(font);
}