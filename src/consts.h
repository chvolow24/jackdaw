#ifndef JDAW_CONSTS_H
#define JDAW_CONSTS_H


#define TAU (6.283185307179586476925286766559)
#define PI (3.1415926535897932384626433832795)

#define DEFAULT_SAMPLE_RATE 96000
#define DEFAULT_FOURIER_LEN_SFRAMES 1024
#define DEFAULT_AUDIO_CHUNK_LEN_SFRAMES 64
#define DEFAULT_SAMPLE_FORMAT AUDIO_S16SYS

#define DEFAULT_FILTER_LEN 128
#define DELAY_LINE_MAX_LEN_S 1

#define HAMMING_SCALAR 1.851852

#define VOL_EXP 2.0

#define CMP_EPSILON_FLOAT 1e-9f
#define CMP_EPSILON_DOUBLE 1e-9

#define TL_MIN_SFRAMES (INT32_MIN + 9600 * 60 * 10)
#define TL_MAX_SFRAMES (INT32_MAX - 96000 * 60 * 10)

#define AUDIO_FILE_EXTENSIONS \
"wav", "WAV", \
"aiff", "AIFF", "aif", "AIF", "aifc", "AIFC", \
"mp3", "MP3", \
"flac", "FLAC", \
"ogg", "OGG", \
"oga", "OGA", \
"opus", "OPUS", \
"m4a", "M4A", \
"aac", "AAC", \
"ac3", "AC3", \
"eac3", "EAC3", \
"dts", "DTS", \
"amr", "AMR", \
"awb", "AWB", \
"wma", "WMA", \
"asf", "ASF", \
"ape", "APE", \
"mpc", "MPC", \
"wv", "WV", \
"tta", "TTA", \
"tak", "TAK", \
"ra", "RA", \
"rm", "RM", \
"au", "AU", \
"snd", "SND", \
"caf", "CAF", \
"voc", "VOC", \
"8svx", "8SVX", \
"iff", "IFF" \


#endif
