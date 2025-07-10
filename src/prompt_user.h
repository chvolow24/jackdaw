/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    prompt_user.h

    * prompt user with multiple selections
    * interrupts flow of program anywhere
    * contains its own draw and event handling loop
    * standard modals (window_push_modal) should be used instead
      where possible
*****************************************************************************************************************/


#ifndef JDAW_PROMPT_USER_H
#define JDAW_PROMPT_USER_H

int prompt_user(const char *header, const char *description, int num_options, char **option_titles);

#endif
