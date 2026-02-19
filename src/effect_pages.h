/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    effect_pages.h

    * horrible procedural construction of GUI pages for effects
    * this is why they invented HTML
 *****************************************************************************************************************/


#ifndef EFFECT_PAGES_H
#define EFFECT_PAGES_H

#include "effect.h"
#include "page.h"

TabView *effect_chain_tabview_create(EffectChain *ec);
void effect_chain_open_tabview(EffectChain *ec);


#endif
