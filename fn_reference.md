### global mode
- Summon menu : <kbd>C-m</kbd>, <kbd>C-h</kbd>
- Escape : <kbd>\<esc\></kbd>
- Quit : <kbd>C-q</kbd>
- Undo : <kbd>C-z</kbd>
- Redo : <kbd>C-y</kbd>, <kbd>C-S-z</kbd>
- Show output spectrum : <kbd></kbd>
- Save Project : <kbd>C-s</kbd>
- Open file (.wav or .jdaw) : <kbd>C-o</kbd>
- Start API server : <kbd>C-S-p</kbd>
- Function lookup : <kbd>\<spc\></kbd>
- Chaotic user test (debug only) : <kbd>A-S-\<del\></kbd>
### menu_nav mode
- Next item : <kbd>n</kbd>, <kbd>f</kbd>
- Previous item : <kbd>p</kbd>, <kbd>d</kbd>
- Next section : <kbd>C-n</kbd>, <kbd>C-f</kbd>, <kbd>C-\<up\></kbd>
- Previous section : <kbd>C-p</kbd>, <kbd>C-d</kbd>, <kbd>C-\<down\></kbd>
- Choose item : <kbd>\<ret\></kbd>, <kbd>k</kbd>
- Column right : <kbd>l</kbd>
- Column left : <kbd>j</kbd>
- Move menu up : <kbd>\<up\></kbd>
- Move menu down : <kbd>\<down\></kbd>
- Move menu right : <kbd>\<right\></kbd>
- Move menu left : <kbd>\<left\></kbd>
- go back (dismiss) : <kbd>\<del\></kbd>, <kbd>m</kbd>, <kbd>h</kbd>, <kbd>\<esc\></kbd>, <kbd>g</kbd>
### timeline mode
#### Playback / Record
- Play : <kbd>l</kbd>, <kbd>e</kbd>
- Pause : <kbd>k</kbd>, <kbd>w</kbd>, <kbd>S-k</kbd>
- Rewind : <kbd>j</kbd>, <kbd>q</kbd>
- Play slow : <kbd>K-l</kbd>
- Rewind slow : <kbd>K-j</kbd>
- Move playhead left : <kbd>[</kbd>
- Move playhead right : <kbd>]</kbd>
- Move playhead left (slow) : <kbd>S-[</kbd>
- Move playhead right (slow) : <kbd>S-]</kbd>
- Nudge play position left (500 samples) : <kbd>\<left\></kbd>
- Nudge play position right (500 samples) : <kbd>\<right\></kbd>
- Nudge play position left (100 samples) : <kbd>S-\<left\></kbd>
- Nudge play position right (100 samples) : <kbd>S-\<right\></kbd>
- Move one sample left : <kbd>C-S-\<left\></kbd>
- Move one sample right : <kbd>C-S-\<right\></kbd>
- Record (start or stop) : <kbd>r</kbd>
#### Timeline navigation
- Previous track (move selector up) : <kbd>p</kbd>, <kbd>d</kbd>
- Next track (move selector down) : <kbd>n</kbd>, <kbd>f</kbd>
- Toggle automation read : <kbd>S-r</kbd>
- Move selected track down : <kbd>S-n</kbd>, <kbd>S-f</kbd>
- Move selected track up : <kbd>S-p</kbd>, <kbd>S-d</kbd>
- Minimize selected track(s) : <kbd>-</kbd>
- Move view right : <kbd>;</kbd>
- Move view left : <kbd>h</kbd>
- Zoom out : <kbd>,</kbd>
- Zoom in : <kbd>.</kbd>
#### Marks
- Set In : <kbd>i</kbd>
- Set Out : <kbd>o</kbd>
- Go to In : <kbd>S-i</kbd>
- Go to Out : <kbd>S-o</kbd>
- Go to t=0 : <kbd>S-u</kbd>
- Go to previous clip boundary : <kbd>S-j</kbd>
- Go to next clip boundary : <kbd>S-l</kbd>
- Go to next beat : <kbd>C-l</kbd>
- Go to previous beat : <kbd>C-j</kbd>
- Go to next subdiv : <kbd>C-S-l</kbd>
- Go to previous subdiv : <kbd>C-S-j</kbd>
- Go to next measure : <kbd>A-S-l</kbd>
- Go to previous measure : <kbd>A-S-j</kbd>
- Bring rear clip at cursor to front : <kbd>S-z</kbd>
- Toggle loop playback : <kbd>C-8</kbd>
#### Output
- Set default audio output : <kbd>C-S-o</kbd>
#### Tracks
- Add Track : <kbd>C-t</kbd>
- Activate/deactivate selected track : <kbd>\<ret\></kbd>
- Activate/deactivate all tracks : <kbd>`</kbd>
- Delete selected track or automation : <kbd>C-\<del\></kbd>
- Activate track 1 : <kbd>1</kbd>
- Activate track 2 : <kbd>2</kbd>
- Activate track 3 : <kbd>3</kbd>
- Activate track 4 : <kbd>4</kbd>
- Activate track 5 : <kbd>5</kbd>
- Activate track 6 : <kbd>6</kbd>
- Activate track 7 : <kbd>7</kbd>
- Activate track 8 : <kbd>8</kbd>
- Activate track 9 : <kbd>9</kbd>
#### Tempo tracks
- Add click track : <kbd>C-S-t</kbd>
- Set tempo at cursor : <kbd>t</kbd>
#### Track settings
- Open track settings : <kbd>S-t</kbd>
- Mute or unmute selected track(s) : <kbd>m</kbd>
- Solo or unsolo selected track(s) : <kbd>s</kbd>
- Track volume up : <kbd>S-=</kbd>
- Track volume down : <kbd>S--</kbd>
- Track pan left : <kbd>S-9</kbd>
- Track pan right : <kbd>S-0</kbd>
- Rename selected track : <kbd>C-r</kbd>
- Set track input : <kbd>C-S-i</kbd>
- Show or hide all track automations : <kbd>a</kbd>
- Add automation to track : <kbd>C-a</kbd>
#### Clips
- Grab clip at cursor : <kbd>g</kbd>
- Grab clips in marked range : <kbd>S-g</kbd>
- Copy grabbed clips : <kbd>C-c</kbd>
- Paste grabbed clips : <kbd>C-v</kbd>
- Start or stop dragging clips : <kbd>C-k</kbd>
- Cut : <kbd>S-c</kbd>
- Split stereo clip at cursor to mono : <kbd></kbd>
- Rename clip at cursor : <kbd>C-S-r</kbd>
- Delete : <kbd>\<del\></kbd>
#### Sample mode
- Load clip at cursor to source : <kbd>C-1</kbd>
- Activate Source Mode : <kbd>S-1</kbd>
- Drop clip from source : <kbd>b</kbd>
- Drop previously dropped clip (1) : <kbd>v</kbd>
- Drop previously dropped clip (2) : <kbd>c</kbd>
- Drop previously dropped clip (3) : <kbd>x</kbd>
- Drop previously dropped clip (4) : <kbd>z</kbd>
#### Multiple timelines
- Add new timeline : <kbd>A-t</kbd>
- Previous timeline : <kbd>A-j</kbd>
- Next timeline : <kbd>A-l</kbd>
- Delete timeline : <kbd>A-\<del\></kbd>
#### Export
- Write mixdown to .wav file : <kbd>C-e</kbd>, <kbd>S-w</kbd>
### source mode
- Play (source) : <kbd>l</kbd>
- Pause (source) : <kbd>k</kbd>, <kbd>S-k</kbd>
- Rewind (source) : <kbd>j</kbd>
- Play slow (source) : <kbd>S-l</kbd>, <kbd>K-l</kbd>
- Rewind slow (source : <kbd>S-j</kbd>, <kbd>K-j</kbd>
- Set In Mark (source) : <kbd>i</kbd>, <kbd>S-i</kbd>
- Set Out Mark (source) : <kbd>o</kbd>, <kbd>S-o</kbd>
### modal mode
- Go to next item : <kbd>n</kbd>, <kbd>f</kbd>
- Go to previous item : <kbd>p</kbd>, <kbd>d</kbd>
- Go to next item (escape DirNav) : <kbd>\<tab\></kbd>, <kbd>S-n</kbd>, <kbd>S-f</kbd>
- Go to previous item (escape DirNav) : <kbd>S-\<tab\></kbd>, <kbd>S-p</kbd>, <kbd>S-d</kbd>
- Select item : <kbd>\<ret\></kbd>
- Dismiss modal window : <kbd>m</kbd>, <kbd>h</kbd>, <kbd>g</kbd>, <kbd>\<esc\></kbd>
- Submit form : <kbd>C-\<ret\></kbd>
### text_edit mode
- Escape text edit : <kbd>\<ret\></kbd>, <kbd>\<tab\></kbd>, <kbd>S-\<tab\></kbd>
- Escape text edit : <kbd>\<esc\></kbd>
- Backspace : <kbd>\<del\></kbd>
- Move cursor right : <kbd>\<right\></kbd>, <kbd>C-f</kbd>
- Move cursor left : <kbd>\<left\></kbd>, <kbd>C-d</kbd>, <kbd>C-b</kbd>
- Select all : <kbd>C-a</kbd>
### tabview mode
- Next element : <kbd>S-n</kbd>, <kbd>S-f</kbd>, <kbd>\<tab\></kbd>
- Previous element : <kbd>S-p</kbd>, <kbd>S-d</kbd>, <kbd>S-\<tab\></kbd>
- Select : <kbd>\<ret\></kbd>
- Move left : <kbd>h</kbd>
- Move right : <kbd>;</kbd>
- Next tab : <kbd>S-l</kbd>, <kbd>S-;</kbd>
- Previous tab : <kbd>S-j</kbd>, <kbd>S-h</kbd>
- Close tab view : <kbd>g</kbd>, <kbd>\<esc\></kbd>
### autocomplete_list mode
- Next item : <kbd>\<tab\></kbd>
- Previous item : <kbd>S-\<tab\></kbd>
- Select item : <kbd>\<ret\></kbd>
- Escape autocomplete list : <kbd>\<esc\></kbd>
