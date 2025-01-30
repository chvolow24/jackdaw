#include "SDL.h"
#include "SDL_events.h"
#include "input.h"
#include "test.h"

void breakfn()
{
}

extern Window *main_win;


TEST_FN_DEF(
    really_dumb_tests,
    {
	static uint64_t num_frames = 0;
	if (num_frames == 0) srand(time(NULL));

	num_frames++;
	if (num_frames >= max_num_frames) {
	    num_frames = 0;
	    *run_tests = false;
	    return 0;
	}
    

	uint16_t i_state = 0;
	int statestat = rand() % 5;
	if (statestat == 0) i_state |= I_STATE_SHIFT;
	statestat = rand() % 3;
	if (statestat == 0) i_state |= I_STATE_CMDCTRL;
	statestat = rand() % 20;
	if (statestat == 0) i_state |= I_STATE_META;
	main_win->i_state = i_state;
	
	int num_events = rand() % 10; /* Max 10 per frame */
	for (int i=0; i<num_events; i++) {

	    int key = rand() % (127 - 32) + 32;

	    if (key == '6') key++;
	    if (key >= 65 && key <= 90) {
		key += ('a' - 'A');
	    }
	    if ((key == 'q' || key == 's') && i_state == I_STATE_CMDCTRL) continue;
	    if (key == 'w' && i_state == I_STATE_SHIFT) continue;
	    if (key == 'e' && i_state == I_STATE_CMDCTRL) continue;
	    if ((key == 'm' || key == 'h') && i_state == I_STATE_CMDCTRL) continue;
	    
	    SDL_Event e;

	    if (main_win->txt_editing) {
		e.type = SDL_TEXTINPUT;
		e.text.text[0] = (char)key;
		SDL_PushEvent(&e);
	    }
	    
	    e.type = SDL_KEYDOWN;
	    SDL_Scancode sc = SDL_GetScancodeFromKey(key);
	    e.key.keysym.sym = key;
	    e.key.keysym.scancode = sc;
	    SDL_PushEvent(&e);
	
	    e.type = SDL_KEYUP;
	    SDL_PushEvent(&e);
	    
	    char *keycmd_str = input_get_keycmd_str(i_state, key);
	    fprintf(stderr, "%s\n", keycmd_str);
	    free(keycmd_str);
	}
	/* fprintf(stderr, "\n\n\n\n"); */
	return 0;
    } , bool *run_tests, uint64_t max_num_frames);
