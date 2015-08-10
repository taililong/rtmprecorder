#include <string.h>
#include <time.h>
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavutil/avutil.h"
#include "include/libswresample/swresample.h"
#include "com_wzzn_myrenzheng_Taiplayer.h"
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "myrenzheng", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "rzerror", __VA_ARGS__)

int isready = 0;
int debug = 0;
AVFormatContext *oc;
AVStream *audio_st = NULL;
AVCodecContext *audio_c;
struct AVFrame *samples_frame;
AVPacket pkt, pkt2;
struct SwrContext *swr_ctx;
void my_log_callback(void *ptr, int level, const char *fmt, va_list vargs) {
	LOGE(fmt, vargs);
}
JNIEXPORT jint JNICALL Java_com_wzzn_myrenzheng_Taiplayer_release(JNIEnv *env,
		jobject thiz) {
	if(isready == 0){
		return 0;
	}
//	avcodec_close(audio_c);
//	audio_c = NULL;
//	av_free_packet(&pkt);
//	av_frame_free(&samples_frame);
	avformat_close_input(&oc);
	oc = NULL;
//	swr_free(&swr_ctx);
	isready = 0;
	return 0;
}
JNIEXPORT jint JNICALL Java_com_wzzn_myrenzheng_Taiplayer_ffmpegGrabber(
		JNIEnv *env, jobject thiz, jstring pfilename) {
	const char* filename = (*env)->GetStringUTFChars(env, pfilename, 0);
	int ret = 0;
	if (debug != 0) {
		av_log_set_level(AV_LOG_ERROR);
		av_log_set_callback(my_log_callback);

	}
	av_register_all();
	avformat_network_init();
	oc = avformat_alloc_context();
	LOGD("avformat_alloc_context");
	ret = avformat_open_input(&oc, filename, NULL, NULL);
	if (ret != 0) {
		LOGD("avformat_open_input error");
		return -1;
	}
	LOGD("avformat_open_input");
	if (avformat_find_stream_info(oc, NULL) < 0) {
		LOGD("Couldn't find stream information.\n");
		return -2;
	}
	LOGD("avformat_find_stream_info");
	av_dump_format(oc, 0, filename, 0);
	audio_st = NULL;
	int i;
	for (i = 0; i < oc->nb_streams; i++) {
		if (oc->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_st = oc->streams[i];
			audio_c = oc->streams[i]->codec;
			LOGD("AVMEDIA_TYPE_AUDIO ,%d,%d", oc->streams[i]->codec->codec_id,
					AV_CODEC_ID_AAC);

		}
	}
	if (audio_st != NULL) {
		AVCodec *codec = avcodec_find_decoder(audio_c->codec_id);
		if (codec == NULL) {
			LOGD("avcodec_find_decoder ERROR,%d", audio_c->codec_id);
			return -3;
		}
		LOGD("找到音频解码器：%s", codec->name);
		ret = avcodec_open2(audio_c, codec, NULL);
		if (ret < 0) {
			LOGD("avcodec_open2 ERROR");
			return -4;
		}
		samples_frame = av_frame_alloc();
		isready = 1;
		av_init_packet(&pkt);
		av_init_packet(&pkt2);
		return 0;
	}

	return -5;
}
JNIEXPORT jbyteArray JNICALL Java_com_wzzn_myrenzheng_Taiplayer_getAudio(
		JNIEnv *env, jobject thiz) {
	if (isready == 0) {
		return NULL;
	}

	//LOGD("isready OK");
	int ret = 0;
	int done = 0;
	while (!done) {
		if (pkt2.size <= 0) {
			if (av_read_frame(oc, &pkt) < 0) {
				LOGD("av_read_frame ERROR");

				return NULL;
			}
		}
		//LOGD("av_read_frame OK");
		if (pkt.stream_index == audio_st->index) {
//			LOGD("pkt.stream_index == audio_st->index");
			int got_frame = 0;
			int len = avcodec_decode_audio4(audio_c, samples_frame, &got_frame,
					&pkt);
			if (len < 0) {
				LOGD("avcodec_decode_audio4 null");
				continue;
			}
			int sample_format = samples_frame->format;
			int planes =
					av_sample_fmt_is_planar(sample_format) != 0 ?
							samples_frame->channels : 1;

			int data_size = av_samples_get_buffer_size(NULL, audio_c->channels,
					samples_frame->nb_samples, audio_c->sample_fmt, 1) / planes;
//			LOGD("avcodec_decode_audio4 OK got_frame=%d planar=%d DATA_SIZE=%d",
//					got_frame, planes, samples_frame->linesize[0]);
			if (swr_ctx == NULL) {
				swr_ctx = swr_alloc_set_opts(swr_ctx, audio_c->channel_layout,
						AV_SAMPLE_FMT_S16, audio_c->sample_rate,
						audio_c->channel_layout, AV_SAMPLE_FMT_FLTP,
						audio_c->sample_rate, 0, NULL);
				swr_init(swr_ctx);
			}
			int dst_nb_samples = av_rescale_rnd(
					swr_get_delay(swr_ctx, audio_c->sample_rate)
							+ samples_frame->nb_samples, audio_c->sample_rate,
					audio_c->sample_rate, AV_ROUND_UP);
			uint8_t **dst_data = NULL;
			int dst_linesize;
			av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, 2,
					dst_nb_samples, AV_SAMPLE_FMT_S16, 0);
			int len2 = swr_convert(swr_ctx, dst_data, dst_nb_samples,
					(const uint8_t **) samples_frame,
					samples_frame->nb_samples);
			jbyteArray jbarray = (*env)->NewByteArray(env, data_size);
			(*env)->SetByteArrayRegion(env, jbarray, 0, data_size, dst_data[0]);
			if (dst_data) {
				av_freep(&dst_data[0]);
			}
			av_freep(&dst_data);
			return jbarray;
		}
	}

}
