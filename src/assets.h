#ifndef JDAW_ASSETS_H
#define JDAW_ASSETS_H

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define LAYOUT_PATH INSTALL_DIR "/assets/layouts"
#define MAIN_LT_PATH LAYOUT_PATH "/jackdaw_main_layout.xml"
#define TRACK_LT_PATH LAYOUT_PATH "/track_template.xml"
#define QUICKREF_LT_PATH LAYOUT_PATH "/quickref.xml"
#define SOURCE_AREA_LT_PATH LAYOUT_PATH "/source_area.xml"
#define OUTPUT_PANEL_LT_PATH LAYOUT_PATH "/output_panel.xml"
#define OUTPUT_SPECTRUM_LT_PATH LAYOUT_PATH "/output_spectrum.xml"
#define QWERTY_PANEL_LT_PATH LAYOUT_PATH "/qwerty_panel.xml"
#define CLICK_TRACK_LT_PATH LAYOUT_PATH "/click_track_template.xml"
#define FIR_FILTER_LT_PATH LAYOUT_PATH "/track_settings_fir_filter.xml"
#define DELAY_LINE_LT_PATH LAYOUT_PATH "/track_settings_delay_line.xml"
#define EQ_LT_PATH LAYOUT_PATH "/track_settings_eq.xml"
#define SATURATION_LT_PATH LAYOUT_PATH "/track_settings_saturation.xml"
#define COMPRESSOR_LT_PATH LAYOUT_PATH "/track_settings_compressor.xml"
#define CLICK_TRACK_SETTINGS_LT_PATH LAYOUT_PATH "/click_track_settings.xml"
#define LOADING_SCREEN_LT_PATH LAYOUT_PATH "/loading_screen.xml"

#define SYNTH_OSCS_LT_PATH LAYOUT_PATH "/synth_oscs.xml"
#define SYNTH_AMP_ENV_LT_PATH LAYOUT_PATH "/synth_amp_env.xml"
#define SYNTH_FILTER_LT_PATH LAYOUT_PATH "/synth_filter.xml"
#define SYNTH_NOISE_LT_PATH LAYOUT_PATH "/synth_noise.xml"
#define SYNTH_PRESETS_LT_PATH LAYOUT_PATH "/synth_presets.xml"

#define PIANO_LT_PATH LAYOUT_PATH "/piano.xml"


#define TTF_PATH INSTALL_DIR "/assets/ttf/"
#define OPEN_SANS_PATH TTF_PATH "OpenSans-Regular.ttf"
#define OPEN_SANS_BOLD_PATH TTF_PATH "OpenSans-Bold.ttf"

/* #define OPEN_SANS_PATH TTF_PATH "Overpass-VariableFont_wght.ttf" */
/* #define OPEN_SANS_BOLD_PATH TTF_PATH "Overpass-VariableFont_wght.ttf" */

#define LTSUPERIOR_PATH TTF_PATH "LTSuperiorMono-Regular.otf"
#define LTSUPERIOR_BOLD_PATH TTF_PATH "LTSuperiorMono-Bold.otf"
#define NOTO_SANS_SYMBOLS2_PATH TTF_PATH "NotoSansSymbols2-Regular.ttf"
#define NOTO_SANS_MATH_PATH TTF_PATH "NotoSansMath-Regular.ttf"

#define METRONOME_PATH INSTALL_DIR "/assets/metronome/"
#define METRONOME_STD_HIGH_PATH METRONOME_PATH "std_high.wav"
#define METRONOME_STD_LOW_PATH METRONOME_PATH "std_low.wav"

/* #define DEFAULT_KEYB_PATH INSTALL_DIR "/assets/key_bindings/default.yaml" */
#define DEFAULT_KEYBIND_CFG_PATH INSTALL_DIR "/assets/key_bindings/default.yaml"

#define PARAM_LT_PATH INSTALL_DIR "/gui/template_lts/param_lt.xml"
#define OPENFILE_LT_PATH INSTALL_DIR "/gui/template_lts/openfile.xml"



#endif
