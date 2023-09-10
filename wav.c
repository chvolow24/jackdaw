/****************************** WAV File Specification ******************************

Positions	Sample Value	    Description
1 - 4	“RIFF”                  Marks the file as a riff file. Characters are each 1 byte long.
5 - 8	File size (integer)	    Size of the overall file - 8 bytes, in bytes (32-bit integer). Typically, you’d fill this in after creation.
9 -12	“WAVE”	                File Type Header. For our purposes, it always equals “WAVE”.
13-16	“fmt "	                Format chunk marker. Includes trailing null
17-20	16	                    Length of format data as listed above
21-22	1	                    Type of format (1 is PCM) - 2 byte integer
23-24	2	                    Number of Channels - 2 byte integer
25-28	44100	                Sample Rate - 32 byte integer. Common values are 44100 (CD), 48000 (DAT). Sample Rate = Number of Samples per second, or Hertz.
29-32	176400	                (Sample Rate * BitsPerSample * Channels) / 8.
33-34	4	                    (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
35-36	16	                    Bits per sample
37-40	“data”	                “data” chunk header. Marks the beginning of the data section.
41-44	File size (data)	    Size of the data section.
*************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
void write_wav_header(FILE *f, uint32_t samples, uint16_t bits_per_sample, uint8_t channels)
{
    // int bps = 16;

    /* RIFF rhunk descriptor */
    const static char spec[] = {'R','I','F','F'};
    uint32_t file_size = 44 + samples * (bits_per_sample / 8);
    const static char ftype[] = {'W','A','V','E'};

    /* fmt sub-chunk */
    const static char fmt_chunk_mk[] = {'f', 'm', 't', ' '};
    uint32_t fmt_data_len = 16;
    const static uint16_t fmt_type = 1;
    uint16_t num_channels = channels;
    uint32_t sample_rate = 44100;
    uint32_t bytes_per_sec = (sample_rate * bits_per_sample * num_channels) / 8;
    uint16_t block_align = num_channels * fmt_data_len / 8;
    /* bits per sample goes here */

    /* data sub-chunk */
    char chunk_id[] = {'d','a','t','a'};
    uint32_t chunk_size = samples * (bits_per_sample / 8);

    fwrite(spec, 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite(ftype, 1, 4, f);
    fwrite(fmt_chunk_mk, 1, 4, f);
    fwrite(&fmt_data_len, 4, 1, f);
    fwrite(&fmt_type, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&bytes_per_sec, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite(chunk_id, 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
}

void write_wav(const char *fname)
{
    FILE* f = fopen(fname, "wb");
    int16_t samples[220500];
    memset(samples, 0, 220500 * 2);

    int up = 0;
    int16_t amplitude = 2000;
    for (int i=0; i<220500; i++) {
        if (up) {
            samples[i] = amplitude;
        } else {
            samples[i] = amplitude * -1;
        }
        if (i%735 == 0) {
            up = up == 0 ? 1 : 0;
        }
    }
    write_wav_header(f, 220500, 16, 1);
    fwrite(samples, 2, 220500, f);
}