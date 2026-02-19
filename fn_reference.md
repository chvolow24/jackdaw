
### global mode

- Summon menu : <kbd>C-m</kbd>, <kbd>C-h</kbd>
- Escape : <kbd>\<esc\></kbd>
- Quit : <kbd>C-q</kbd>
- Undo : <kbd>C-z</kbd>, <kbd>C-/</kbd>
- Redo : <kbd>C-y</kbd>, <kbd>C-S-z</kbd>
- Show output spectrum : <kbd></kbd>
- Save Project : <kbd>C-s</kbd>
- Open file (.wav or .jdaw) : <kbd>C-o</kbd>
- Start API server : <kbd>C-S-p</kbd>
- Command lookup : <kbd>/</kbd>
- Chaotic user test (debug only) : <kbd>A-S-\<del\></kbd>
- Toggle tranpsort performance logging (debug only) : <kbd></kbd>
- Print all API routes : <kbd></kbd>
- Dump exec logs to stderr : <kbd></kbd>
- Disable synth parallelism : <kbd></kbd>
- Enable synth parallelism : <kbd></kbd>

### menu_nav mode

- Next item : <kbd>n</kbd>, <kbd>\<tab\></kbd>, <kbd>f</kbd>
- Previous item : <kbd>p</kbd>, <kbd>S-\<tab\></kbd>, <kbd>d</kbd>
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
- Pause : <kbd>k</kbd>, <kbd>w</kbd>
- Rewind : <kbd>j</kbd>, <kbd>q</kbd>
- Play/pause : <kbd>\<spc\></kbd>
- Play slow : <kbd>k-l</kbd>
- Rewind slow : <kbd>k-j</kbd>
- Halve playspeed : <kbd>A-k</kbd>
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

- Previous track (move selector up) : <kbd>p</kbd>, <kbd>\<up\></kbd>, <kbd>d</kbd>
- Next track (move selector down) : <kbd>n</kbd>, <kbd>\<down\></kbd>, <kbd>f</kbd>
- Toggle automation read : <kbd>S-r</kbd>
- Move selected track down : <kbd>S-n</kbd>, <kbd>S-f</kbd>, <kbd>S-\<down\></kbd>
- Move selected track up : <kbd>S-p</kbd>, <kbd>S-d</kbd>, <kbd>S-\<up\></kbd>
- Minimize selected track(s) : <kbd>-</kbd>
- Move view right : <kbd>;</kbd>
- Move view left : <kbd>h</kbd>
- Zoom out : <kbd>,</kbd>
- Zoom in : <kbd>.</kbd>
- Lock view to playhead : <kbd>S-\</kbd>

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

#### Click tracks

- Add click track : <kbd>C-S-t</kbd>
- Set tempo at cursor : <kbd>t</kbd>

#### Track settings

- Add effect to track : <kbd>S-e</kbd>
- Open track effects (or click track settings) : <kbd>S-t</kbd>
- Open synth : <kbd>S-s</kbd>
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
- Grab left edge of clips : <kbd>g-j</kbd>
- Grab right edge of clips : <kbd>g-l</kbd>
- Ungrab clip edge : <kbd>g-k</kbd>
- Grab clips in marked range : <kbd>S-g</kbd>
- Grab left edge of clips in marked range : <kbd>S-g-j</kbd>
- Grab right edge of clips in marked range : <kbd>S-g-l</kbd>
- Copy grabbed clips : <kbd>C-c</kbd>
- Paste grabbed clips : <kbd>C-v</kbd>
- Start or stop dragging clips : <kbd>C-k</kbd>, <kbd>S-k</kbd>
- Cut at cursor (clip or click track) : <kbd>S-c</kbd>
- Cut clip at cursor and grab edges : <kbd>g-c</kbd>
- Split stereo clip at cursor to mono : <kbd></kbd>
- Rename clip at cursor : <kbd>C-S-r</kbd>
- Delete : <kbd>\<del\></kbd>

#### MIDI / piano roll

- Edit clip at cursor / open piano roll : <kbd>C-S-e</kbd>
- Quantize notes (clip at cursor) : <kbd>A-q</kbd>
- Adjust quantize amount : <kbd>A-S-q</kbd>

#### Source mode

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

#### MIDI I/O

- Set track MIDI out : <kbd>C-S-m</kbd>
- Activate QWERTY piano : <kbd>S-q</kbd>
- Insert Jlily (LilyPond) notes : <kbd></kbd>

### source mode

- Play (source) : <kbd>l</kbd>
- Pause (source) : <kbd>k</kbd>, <kbd>S-k</kbd>
- Rewind (source) : <kbd>j</kbd>
- Play slow (source) : <kbd>S-l</kbd>, <kbd>k-l</kbd>
- Rewind slow (source : <kbd>S-j</kbd>, <kbd>k-j</kbd>
- Set In Mark (source) : <kbd>i</kbd>, <kbd>S-i</kbd>
- Set Out Mark (source) : <kbd>o</kbd>, <kbd>S-o</kbd>
- Zoom in (source) : <kbd>.</kbd>
- Zoom out (source) : <kbd>,</kbd>
- Move left (source) : <kbd>h</kbd>
- Move right (source) : <kbd>;</kbd>

### modal mode

- Go to next item : <kbd>n</kbd>, <kbd>f</kbd>, <kbd>\<down\></kbd>
- Go to previous item : <kbd>p</kbd>, <kbd>d</kbd>, <kbd>\<up\></kbd>
- Go to next item (escape DirNav) : <kbd>\<tab\></kbd>, <kbd>S-n</kbd>, <kbd>S-f</kbd>, <kbd>S-\<down\></kbd>
- Go to previous item (escape DirNav) : <kbd>S-\<tab\></kbd>, <kbd>S-p</kbd>, <kbd>S-d</kbd>, <kbd>S-\<up\></kbd>
- Select item : <kbd>\<ret\></kbd>
- Modal left (nudge slider) : <kbd>h</kbd>
- Modal right (nudge slider) : <kbd>;</kbd>
- Dismiss modal window : <kbd>m</kbd>, <kbd>g</kbd>, <kbd>\<esc\></kbd>, <kbd>\<del\></kbd>, <kbd>C-\<del\></kbd>, <kbd>S-\<del\></kbd>
- Submit form : <kbd>C-\<ret\></kbd>

### text_edit mode

- Escape text edit : <kbd>\<ret\></kbd>, <kbd>\<tab\></kbd>, <kbd>S-\<tab\></kbd>
- Escape text edit : <kbd>\<esc\></kbd>
- Backspace : <kbd>\<del\></kbd>
- Move cursor right : <kbd>\<right\></kbd>, <kbd>C-f</kbd>
- Move cursor left : <kbd>\<left\></kbd>, <kbd>C-d</kbd>, <kbd>C-b</kbd>
- Select all : <kbd>C-a</kbd>

### tabview mode

- Next element : <kbd>S-n</kbd>, <kbd>S-\<down\></kbd>, <kbd>S-f</kbd>, <kbd>\<tab\></kbd>
- Previous element : <kbd>S-p</kbd>, <kbd>S-\<up\></kbd>, <kbd>S-d</kbd>, <kbd>S-\<tab\></kbd>
- Select : <kbd>\<ret\></kbd>
- Move left : <kbd>h</kbd>
- Move right : <kbd>;</kbd>
- Next tab : <kbd>S-l</kbd>, <kbd>S-;</kbd>
- Previous tab : <kbd>S-j</kbd>, <kbd>S-h</kbd>
- Move current tab left : <kbd>C-S-j</kbd>, <kbd>C-S-h</kbd>
- Move current tab right : <kbd>C-S-l</kbd>, <kbd>C-S-;</kbd>
- Close tab view : <kbd>\<esc\></kbd>, <kbd>C-\<del\></kbd>, <kbd>S-\<del\></kbd>

### autocomplete_list mode

- Next item : <kbd>\<tab\></kbd>, <kbd>\<down\></kbd>, <kbd>C-n</kbd>
- Previous item : <kbd>S-\<tab\></kbd>, <kbd>\<up\></kbd>, <kbd>C-p</kbd>
- Select item : <kbd>\<ret\></kbd>
- Escape autocomplete list : <kbd>\<esc\></kbd>, <kbd>/</kbd>, <kbd>C-\<del\></kbd>, <kbd>S-\<del\></kbd>

### midi_qwerty mode

- Escape MIDI QWERTY : <kbd>\<esc\></kbd>, <kbd>S-q</kbd>
- Octave up : <kbd>x</kbd>
- Octave down : <kbd>z</kbd>
- Transpose up : <kbd>.</kbd>
- Transpose down : <kbd>,</kbd>
- Velocity up : <kbd>v</kbd>
- Velocity down : <kbd>c</kbd>
- midi qwerty c1 : <kbd>a</kbd>
- midi qwerty cis1 : <kbd>w</kbd>
- midi qwerty d1 : <kbd>s</kbd>
- midi qwerty dis : <kbd>e</kbd>
- midi qwerty e : <kbd>d</kbd>
- midi qwerty f : <kbd>f</kbd>
- midi qwerty fis : <kbd>t</kbd>
- midi qwerty g : <kbd>g</kbd>
- midi qwerty gis : <kbd>y</kbd>
- midi qwerty a : <kbd>h</kbd>
- midi qwerty ais : <kbd>u</kbd>
- midi qwerty b : <kbd>j</kbd>
- midi qwerty c2 : <kbd>k</kbd>
- midi qwerty cis2 : <kbd>o</kbd>
- midi qwerty d2 : <kbd>l</kbd>
- midi qwerty dis2 : <kbd>p</kbd>
- midi qwerty e2 : <kbd>;</kbd>
- midi qwerty f2 : <kbd>'</kbd>

### piano_roll mode

- Escape / exit piano roll : <kbd>\<esc\></kbd>, <kbd>C-S-e</kbd>
- Zoom in (piano roll) : <kbd>.</kbd>
- Zoom out (piano roll) : <kbd>,</kbd>
- Move view left (piano roll) : <kbd>h</kbd>
- Move view right (piano roll) : <kbd>;</kbd>
- Note selector down : <kbd>n</kbd>, <kbd>f</kbd>
- Note selector up : <kbd>p</kbd>, <kbd>d</kbd>
- Note selector down octave : <kbd>C-n</kbd>, <kbd>C-f</kbd>
- Note selector up octave : <kbd>C-p</kbd>, <kbd>C-d</kbd>
- Velocity down : <kbd>c</kbd>
- Velocity down : <kbd>v</kbd>
- Move forward by current dur : <kbd>\<right\></kbd>
- Move back by current dur : <kbd>\<left\></kbd>
- Go to next note boundary : <kbd>S-l</kbd>
- Go to previous note boundary : <kbd>S-j</kbd>
- Go to next note up : <kbd>S-p</kbd>, <kbd>S-d</kbd>
- Go to next note down : <kbd>S-n</kbd>, <kbd>S-f</kbd>
- Longer note duration : <kbd>\<down\></kbd>, <kbd>1</kbd>
- Shorter note duration : <kbd>\<up\></kbd>, <kbd>2</kbd>
- Insert note at current position : <kbd>\<ret\></kbd>
- Insert rest at current position : <kbd>\<spc\></kbd>
- Grab/ungrab notes : <kbd>g</kbd>
- Grab left edge of note : <kbd>g-j</kbd>
- Grab right edge of note : <kbd>g-l</kbd>
- Grab notes in marked range : <kbd>S-g</kbd>
- Backspace / delete grabbed notes : <kbd>\<del\></kbd>, <kbd>C-\<del\></kbd>
- Play grabbed notes : <kbd>S-\<spc\></kbd>
- Toggle tie mode : <kbd>S-t</kbd>
- Toggle chord mode : <kbd>S-h</kbd>
- Quantize notes : <kbd>A-q</kbd>
- Adjust quantize amount : <kbd>A-S-q</kbd>
