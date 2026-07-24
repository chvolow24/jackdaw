#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"

#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"

#include "jdaw_ffmpeg.h"
#include "log.h"
#include "session.h"

#define AV_ERR_CHECK(function_name, ...) \
    av_ret = function_name(__VA_ARGS__); \
    if (av_ret < 0) { \
        av_log_error(#function_name " failed", av_ret); \
        ret = -1;                                       \
        goto cleanup_and_ret; \
    }

static void av_log_error(const char *wrapper_msg, int av_err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(av_err, buf, sizeof(buf));
    log_tmp(LOG_ERROR, "%s: %s\n", wrapper_msg, buf);
}

/* Given an input file, retrieve:
   - an AVFormatContext (CALLER MUST FREE with avformat_close_input)
   - an AVCodec
   - an AVCodecContext (CALLER MUST FREE with avcodec_free_context)

   Return the stream index returned by av_find_best_stream, or an error code < 0
   
*/
static int av_open_codec_from_file(
    const char *filepath,
    AVFormatContext **avfmt_ctx_dst,
    const AVCodec **avcodec_dst,
    AVCodecContext **avcodec_ctx_dst)
{
    AVFormatContext *fmt = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;

    int ret = 0;
    int avret = avformat_open_input(&fmt, filepath, NULL, NULL);
    if (avret < 0) {
	av_log_error("avformat_open_input failed", avret);
	ret = -1;
	goto err_cleanup_and_return;
    }
    avret = avformat_find_stream_info(fmt, NULL);
    if (avret < 0) {
	av_log_error("avformat_find_stream_info failed", avret);
	ret = -2;
	goto err_cleanup_and_return;
    }
    int stream_index = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (stream_index < 0) {
	av_log_error("avformat_find_best_stream failed", avret);
	ret = -3;
	goto err_cleanup_and_return;
    }

    const AVStream *stream = fmt->streams[stream_index];
    codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
	log_tmp(LOG_ERROR, "Unable to find codec for id %d, %s\n", stream->codecpar->codec_id, avcodec_get_name(stream->codecpar->codec_id));
	ret = -4;
	goto err_cleanup_and_return;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
	log_tmp(LOG_ERROR, "avcodec_alloc_context3 failed\n");
	ret = -5;
	goto err_cleanup_and_return;
    }
    avret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
    if (avret < 0) {
	av_log_error("avcodec_parameters_to_context failed", avret);
	ret = -1;
	goto err_cleanup_and_return;
    }

    avret = avcodec_open2(codec_ctx, codec, NULL);
    if (avret < 0) {
	av_log_error("avcodec_open2 failed", avret);
	ret = -6;
	goto err_cleanup_and_return;
    }

    *avfmt_ctx_dst = fmt;
    *avcodec_dst = codec;
    *avcodec_ctx_dst = codec_ctx;
    
    return ret;
    
err_cleanup_and_return:
    if (fmt) {
	avformat_close_input(&fmt);
    }
    if (codec_ctx) {
	avcodec_free_context(&codec_ctx);
    }
    *avfmt_ctx_dst = NULL;
    *avcodec_dst = NULL;
    *avcodec_ctx_dst = NULL;
    return -1;        
}

/* Return length of PCM buffer in sample frames */
int32_t av_open_file(const char *filepath, float **L_dst, float **R_dst)
{
    session_loading_screen_update("Finding and initializing codec...", 0.1);

    int out_sr = session_get_sample_rate();
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVFormatContext *fmt = NULL;
    int ret = av_open_codec_from_file(filepath, &fmt, &codec, &codec_ctx);
    int64_t raw_dur = fmt->duration;
    int64_t dur_sframes = ((uint64_t)(out_sr) * raw_dur) / AV_TIME_BASE;
    
    if (ret < 0) {
	return 0;
    }
    int stream_index;
    if (ret < 0) {
	fprintf(stderr, "ERROR GETTING CODEC FROM FILE\n");
	return 0;
    } else {
	stream_index = ret;
    }

    /* Setup */
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
	log_tmp(LOG_ERROR, "av_packet_alloc failed\n");
	return 0;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
	log_tmp(LOG_ERROR, "av_frame_alloc failed\n");
    }

    AVChannelLayout out_layout = {0};
    av_channel_layout_default(&out_layout, 2);

    /* Prepare for resampling inside loop */
    SwrContext *swr = NULL;
    ret = swr_alloc_set_opts2(
	&swr,
	&out_layout,
	AV_SAMPLE_FMT_FLTP,
	out_sr,
	&codec_ctx->ch_layout,
	codec_ctx->sample_fmt,
	codec_ctx->sample_rate,
	0,
	NULL);
    if (ret < 0) {
	av_log_error("swr_alloc_set_opts2 failed", ret);
	return 0;
    }
    ret = swr_init(swr);
    if (ret != 0) {
	av_log_error("swr_init failed", ret);
	return 0;
    }

    /* Prepare buffers */
    int32_t buf_alloc_len_sframes = out_sr * 10;
    int32_t buf_index = 0;
    float *L = malloc(buf_alloc_len_sframes * sizeof(float));
    float *R = malloc(buf_alloc_len_sframes * sizeof(float));

    int32_t loading_scr_sframe_mod = 0;
    const int32_t loading_scr_every_n_sframes = out_sr * 60;

    /* Main decode loop */
    while (av_read_frame(fmt, pkt) >= 0) {
	/* Only read from 'best' stream */
	if (pkt->stream_index == stream_index) {
	    avcodec_send_packet(codec_ctx, pkt);
	    while (1) {
		/* Call avcodec_receive_frame until prompted to send a new packet (EAGAIN) or file done */
		ret = avcodec_receive_frame(codec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		    break;
		}
		if (ret < 0) {
		    break;
		}
		const int max_out_samples = av_rescale_rnd(
		    swr_get_delay(swr, codec_ctx->sample_rate) + frame->nb_samples,
		    out_sr,
		    codec_ctx->sample_rate,
		    AV_ROUND_UP);
		if (buf_index + max_out_samples > buf_alloc_len_sframes) {
		    buf_alloc_len_sframes *= 2;
		    L = realloc(L, buf_alloc_len_sframes * sizeof(float));
		    R = realloc(R, buf_alloc_len_sframes * sizeof(float));
		}
		uint8_t *out[] = {
		    (uint8_t *)(L + buf_index),
		    (uint8_t *)(R + buf_index)
		};
		int out_samples = swr_convert(
		    swr,
		    out,
		    max_out_samples,
		    (const uint8_t **)frame->data,
		    frame->nb_samples);
		buf_index += out_samples;
		loading_scr_sframe_mod += out_samples;
		if (loading_scr_sframe_mod > loading_scr_every_n_sframes) {
		    if (session_loading_screen_update("Decoding file...", (float)buf_index / dur_sframes) == 1) {
			goto cleanup_and_return;
			status_set_errstr("File load aborted!");
		    }
		    loading_scr_sframe_mod = 0;
		}
	    }
	}
    }

cleanup_and_return:
    session_loading_screen_update("Creating clip buffers...", 0.7);
    *L_dst = L;
    *R_dst = R;

    /* CLEANUP */
    session_loading_screen_update("Cleaning up codec resources...", 0.75);
    avformat_close_input(&fmt);
    avcodec_free_context(&codec_ctx);
    
    return buf_index;   
}




/*------ Encode FLAC for .jdaw files ---------------------------------*/

#define ENCODE_FLAC_FRAME_SIZE 4096

int encode_flac(float *buf, int32_t len_sframes, enum ProjectAudioBitDepth bit_depth, uint8_t **encoded_dst, size_t *size_dst)
{
    AVIOContext *mem_writer;
    int ret = 0, av_ret = 0;
    AVFormatContext *fmt = NULL;
    AVStream *stream = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;

    AVFrame *frame = NULL;
    AVPacket *packet = NULL;

    uint8_t *data_buf = NULL;

    /*------ Setup -------------------------------------------------------*/
    
    AV_ERR_CHECK(avformat_alloc_output_context2, &fmt, NULL, "flac", NULL);
    fprintf(stderr, "%s\n", fmt->oformat->name);
    fprintf(stderr, "%s\n", fmt->oformat->long_name);
    AV_ERR_CHECK(avio_open_dyn_buf, &fmt->pb);

    codec = avcodec_find_encoder(AV_CODEC_ID_FLAC);
    if (!codec) {
        log_tmp(LOG_ERROR, "avcodec_find_encoder failed\n");
        ret = -1;
        goto cleanup_and_ret;
    }
    
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        log_tmp(LOG_ERROR, "avcodec_alloc_context3 failed\n");
        ret = -1;
        goto cleanup_and_ret;
    }
    codec_ctx->frame_size = ENCODE_FLAC_FRAME_SIZE;

    stream = avformat_new_stream(fmt, codec);
    if (!stream) {
        log_tmp(LOG_ERROR, "avcodec_new_stream failed");
        ret = -1;
        goto cleanup_and_ret;
    }


    codec_ctx->sample_rate = session_get_sample_rate();
    codec_ctx->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
    switch (bit_depth) {
    case PROJ_AUDIO_16:
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
        break;
    case PROJ_AUDIO_32:
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_S32;
        break;
    }
    codec_ctx->time_base = (AVRational){1, codec_ctx->sample_rate};

    
    AV_ERR_CHECK(avcodec_open2, codec_ctx, codec, NULL);
    AV_ERR_CHECK(avcodec_parameters_from_context, stream->codecpar, codec_ctx);
    stream->time_base = codec_ctx->time_base;
   
    
    frame = av_frame_alloc();
    if (!frame) {
        log_tmp(LOG_ERROR, "av_frame_alloc failed\n");
        ret = -1;
        goto cleanup_and_ret;
    }
    
    packet = av_packet_alloc();
    if (!packet) {
        log_tmp(LOG_ERROR, "av_packet_alloc failed\n");
        ret = -1;
        goto cleanup_and_ret;
    }
    
    /*------ Write data --------------------------------------------------*/
    
    AV_ERR_CHECK(avformat_write_header, fmt, NULL);
    int32_t encode_index = 0;
    bool flushed = false;
    while (!flushed) {
        int iter_len = len_sframes - encode_index < ENCODE_FLAC_FRAME_SIZE ? len_sframes - encode_index : ENCODE_FLAC_FRAME_SIZE;
        av_frame_unref(frame);
        frame->format = codec_ctx->sample_fmt;
        frame->sample_rate = codec_ctx->sample_rate;
        av_channel_layout_copy(&frame->ch_layout, &codec_ctx->ch_layout);
        frame->nb_samples = iter_len;
        frame->pts = encode_index;
        fprintf(stderr, "Sending %d samples...\n", frame->nb_samples);
        AV_ERR_CHECK(av_frame_get_buffer, frame, 0);
        AV_ERR_CHECK(av_frame_make_writable, frame);
        int16_t *d16 = (int16_t *)frame->data[0];
        int32_t *d32 = (int32_t *)frame->data[0];

        for (int i=0; i<iter_len; i++) {
            switch (bit_depth) {
            case PROJ_AUDIO_16:
                d16[i] = buf[encode_index] * INT16_MAX;
                break;
            case PROJ_AUDIO_32:
                d32[i] = buf[encode_index] * INT32_MAX;
                break;
            }
            encode_index++;
        }
        AV_ERR_CHECK(avcodec_send_frame, codec_ctx, frame);
    handle_packets:
        /* Read packets until exhausted */
        while (av_ret >= 0) {
            av_ret = avcodec_receive_packet(codec_ctx, packet);
            if (av_ret == AVERROR(EAGAIN) || av_ret == AVERROR_EOF) {
                break;
            } else if (av_ret < 0) {
                av_log_error("avcodec_receive_packet failed", av_ret);
                ret = -1;
                goto cleanup_and_ret;
            }
            AV_ERR_CHECK(av_write_frame, fmt, packet);
            av_packet_unref(packet);
            /* Packet data is automatically writen to the buf */
            
        }
        if (encode_index == len_sframes && !flushed) {
            /* Flush */
            AV_ERR_CHECK(avcodec_send_frame, codec_ctx, NULL);
            flushed = true;
            goto handle_packets;
        }
    }
    AV_ERR_CHECK(av_write_trailer, fmt);

    /*------ Denoument ---------------------------------------------------*/
    
    int size = avio_close_dyn_buf(fmt->pb, &data_buf);
    *size_dst = size;
    fprintf(stderr, "ENCODED %d bytes (%fx)\n", size, (float)size / (len_sframes * sizeof(float)));
cleanup_and_ret:
    if (fmt) avformat_free_context(fmt);
    if (data_buf && ret < 0) av_free(data_buf);
    if (codec_ctx) avcodec_free_context(&codec_ctx);
    if (frame) av_frame_free(&frame);
    if (packet) av_packet_free(&packet);
    return ret;
}


/*--------------------------------------------------------------------*/




/*------ Decode FLAC for .jdaw files ---------------------------------*/

#define DECODE_FLAC_DATA_SIZE 20480

/* buf should be allocated to len_sframes */
int decode_flac(void *data, size_t data_size, float *buf, int32_t len_sframes, enum ProjectAudioBitDepth bit_depth)
{

}


/*--------------------------------------------------------------------*/
