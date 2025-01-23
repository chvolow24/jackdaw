#include "page.h"
#include "project.h"
#include "tempo.h"

TabView *settings_track_tabview_create(Track *track);
void settings_track_tabview_set_track(TabView *tv, Track *track);

void tempo_track_populate_settings_tabview(TempoTrack *tt, TabView *tv);
