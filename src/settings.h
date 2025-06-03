/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    settings.h

    * mostly replaced by effect_pages.h
    * click track "settings" page
*****************************************************************************************************************/


#include "page.h"
#include "project.h"
#include "tempo.h"

TabView *settings_track_tabview_create(Track *track);
void settings_track_tabview_set_track(TabView *tv, Track *track);

void click_track_populate_settings_tabview(ClickTrack *tt, TabView *tv);
