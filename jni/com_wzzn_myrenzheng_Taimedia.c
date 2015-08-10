#include <string.h>
#include <time.h>
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavutil/avutil.h"
#include "include/libswscale/swscale.h"
#include "com_wzzn_myrenzheng_Taimedia.h"
#include "include/libyuv.h"
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "myrenzheng", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "rzerror", __VA_ARGS__)
#define GET_ARRAY_LEN(array,len){len = (sizeof(array) / sizeof(array[0]));}
int video_width, video_height;
FILE * video_file;
AVPacket video_pkt, audio_pkt;
AVFormatContext *avForCtx;
AVStream *audio_st = NULL, *video_st = NULL;
double audio_pts, video_pts;
int frameCount, audioFrameCount, start;
AVCodec *video_codec = NULL, *audio_codec = NULL;
AVCodecContext *video_c, *audio_c;
uint8_t *video_outbuf, *picture_buf, *audio_outbuf;
uint8_t * frame_buf;
int got_video_packet[1];
int video_outbuf_size, audio_outbuf_size, audio_input_frame_size;
struct AVFrame* picture, *tmp_picture, *audioframe;
struct SwsContext* img_convert_ctx = NULL;
struct SwrContext* samples_convert_ctx = NULL;
int data_size;
int isready = 0;
int debug = 0;

void my_log_callback(void *ptr, int level, const char *fmt, va_list vargs) {
	LOGE(fmt, vargs);
}

JNIEXPORT jint JNICALL Java_com_wzzn_myrenzheng_Taimedia_recordAudio(
		JNIEnv *env, jobject thiz, jbyteArray adata, jlong ptime) {
	if (isready == 0) {
		return -1;
	}
	jint dataLength = (*env)->GetArrayLength(env, adata);
	jbyte* jBuffer = malloc(dataLength * sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, adata, 0, dataLength, jBuffer);

	int ret = 0;

	//LOGD("data_size=%d   dataLength = %d", data_size, dataLength);
	audioframe->data[0] = (uint8_t *) jBuffer;
	audioframe->linesize[0] = dataLength;
	audioframe->quality = audio_c->global_quality;
	int got_audio_packet = 0;
	ret = avcodec_encode_audio2(audio_c, &audio_pkt, audioframe,
			&got_audio_packet);
	if (ret < 0) {
		char errbuf[1024];
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGD("avcodec_encode_audio2 :%d %s", ret,errbuf);
		return -2;
	}
	audioframe->pts = audioframe->pts + audioframe->nb_samples;
//	LOGD("avcodec_encode_audio2 ret = %d audio_pkt.size= %d ", ret,
//			audio_pkt.size);
	if (got_audio_packet < 1) {
		return -11;
	}
	if (audio_pkt.pts != AV_NOPTS_VALUE) {
		//LOGD("audio pts AV_NOPTS_VALUE %d", 0);
		audio_pkt.pts = av_rescale_q(audio_pkt.pts, audio_c->time_base,
				audio_st->time_base);
	}
	if (audio_pkt.dts != AV_NOPTS_VALUE) {
		//LOGD("audio dts AV_NOPTS_VALUE %d", 0);
		audio_pkt.dts = av_rescale_q(audio_pkt.dts, audio_c->time_base,
				audio_st->time_base);
	}
	audio_pkt.flags = audio_pkt.flags | AV_PKT_FLAG_KEY;
	audio_pkt.stream_index = audio_st->index;
//	LOGD("audio_pkt.pts %lld   audio_pkt.dts :%lld stream_index:%d timestamp:%lld",
//			audio_pkt.pts, audio_pkt.dts, audio_pkt.stream_index,(int64)ptime);
	ret = av_interleaved_write_frame(avForCtx, &audio_pkt);
	if (ret < 0) {
		char errbuf[1024];
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGD(
				"audio_pkt av_write_frame av_interleaved_write_frame errrrror %d,%s",
				ret, errbuf);
	}
	av_free_packet(&audio_pkt);
	free(jBuffer);
	return 0;

}

JNIEXPORT jint JNICALL Java_com_wzzn_myrenzheng_Taimedia_ffmpegGetavcodecversion(
		JNIEnv * env, jobject thiz) {
	return (int) avcodec_version();
}

JNIEXPORT jint JNICALL Java_com_wzzn_myrenzheng_Taimedia_release(
		JNIEnv * env, jobject thiz) {
	if (isready == 0) {
			return -1;
	}
	avio_close(avForCtx->pb);
	av_free(avForCtx);
	return 0;
}
/*
 * Class:     com_wzzn_myrenzheng_Taimedia
 * Method:    recordVideo
 * Signature: ([B)I
 */

JNIEXPORT jint JNICALL Java_com_wzzn_myrenzheng_Taimedia_recordVideo(
		JNIEnv *env, jobject thiz, jbyteArray vdata, jint pwidth, jint pheight,
		jlong ptime) {
	if (isready == 0) {
		return -1;
	}
	jint dataLength = (*env)->GetArrayLength(env, vdata);
	jbyte* jBuffer = malloc(dataLength * sizeof(jbyte));
	(*env)->GetByteArrayRegion(env, vdata, 0, dataLength, jBuffer);
	int ret = 0;
	int size = pwidth * pheight * 3 / 2;
	uint8_t* output = (uint8_t *) malloc(size);

	uint8_t* src_y = (uint8_t*) jBuffer;
	int src_stride_y = pwidth;
	uint8_t* src_vu = (uint8_t*) jBuffer + (pwidth * pheight);
	int src_stride_vu = pwidth;
	uint8_t* dst_y = output;
	int dst_stride_y = pwidth;
	uint8_t* dst_u = dst_y + (pwidth * pheight);
	int dst_stride_u = pwidth / 2;
	uint8_t* dst_v = dst_u + (pwidth * pheight) / 4;
	int dst_stride_v = pwidth / 2;
	double timr1_Finish = (double) clock();

	NV21ToI420(src_y, src_stride_y, src_vu, src_stride_vu, dst_y, dst_stride_y,
			dst_u, dst_stride_u, dst_v, dst_stride_v, pwidth, pheight);
	double timr2_Finish = (double) clock();
	//LOGD("NV21ToI420 11消耗时间: %.2fms", (timr2_Finish - timr1_Finish));
	const uint8* s_src_y = dst_y;
	int s_src_stride_y = dst_stride_y;
	const uint8* s_src_u = dst_u;
	int s_src_stride_u = dst_stride_u;
	const uint8* s_src_v = dst_v;
	int s_src_stride_v = dst_stride_v;
	int s_src_width = pwidth;
	int s_src_height = pheight;
	uint8_t* s_dst_y = (uint8_t *) malloc(320 * 240 * 3 / 2);
	int s_dst_stride_y = 320;
	uint8_t* s_dst_u = s_dst_y + (320 * 240);
	int s_dst_stride_u = 320 / 2;
	uint8_t* s_dst_v = s_dst_u + (320 * 240) / 4;
	int s_dst_stride_v = 320 / 2;

	I420Scale(s_src_y, s_src_stride_y, s_src_u, s_src_stride_u, s_src_v,
			s_src_stride_v, s_src_width, s_src_height, s_dst_y, s_dst_stride_y,
			s_dst_u, s_dst_stride_u, s_dst_v, s_dst_stride_v, 320, 240,
			kFilterNone);
	double timr3_Finish = (double) clock();
	//LOGD("I420Scale 11消耗时间: %.2fms", (timr3_Finish - timr2_Finish));

	ret = avpicture_fill((AVPicture *) picture, s_dst_y, PIX_FMT_YUV420P, 320,
			240);
	if (ret < 0) {
		LOGD("avpicture_fill :%d", ret);
		return -2;
	}
	av_init_packet(&video_pkt);
	video_pkt.data = video_outbuf;
	video_pkt.size = video_outbuf_size;
	double encode1_time = (double) clock();
	if ((ret = avcodec_encode_video2(video_c, &video_pkt, picture,
			got_video_packet)) < 0) {
		LOGD("avcodec_encode_video2 %d", ret);
		return -11;

	}
	double timr4_Finish = (double) clock();
	//LOGD("avcodec_encode_video2 11消耗时间: %.2fms", (timr4_Finish - timr3_Finish));
	picture->pts = picture->pts + 1; // magic required by libx264
	if (got_video_packet[0] == 0) {
		return -12;
	}
	if (video_pkt.pts != AV_NOPTS_VALUE) {
		video_pkt.pts = av_rescale_q(video_pkt.pts, video_c->time_base,
				video_st->time_base);
	}
	if (video_pkt.dts != AV_NOPTS_VALUE) {
		video_pkt.dts = av_rescale_q(video_pkt.dts, video_c->time_base,
				video_st->time_base);
	}
	video_pkt.flags = video_pkt.flags | AV_PKT_FLAG_KEY;
	video_pkt.stream_index = video_st->index;
//	LOGD("video_pkt.pts %lld   video_pkt.dts :%lld stream_index:%d timestamp:%lld",
//			video_pkt.pts, video_pkt.dts, video_pkt.stream_index,(int64)ptime);
	if ((ret = av_interleaved_write_frame(avForCtx, &video_pkt)) != 0) {
		char errbuf[1024];
		av_strerror(ret, errbuf, sizeof(errbuf));
		LOGD("video_pkt av_interleaved_write_frame errrrror %d %s", ret,
				errbuf);
	}
	free(s_dst_y);
	free(output);
	free(jBuffer);
	return 0;
}

/*
 * Class:     com_wzzn_myrenzheng_Taimedia
 * Method:    ffmpegRecorder
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_wzzn_myrenzheng_Taimedia_ffmpegRecorder(
		JNIEnv *env, jobject thiz, jstring pfilename, jint pwidth, jint pheight,
		jint pframeRate, jint paudioRate) {
	int ret = 0;
	if (debug != 0) {
		av_log_set_level(AV_LOG_ERROR);
		av_log_set_callback(my_log_callback);

	}
	av_register_all();
	avformat_network_init();
	video_width = pwidth;
	video_height = pheight;
	const char* filename = (*env)->GetStringUTFChars(env, pfilename, 0);
	avForCtx = avformat_alloc_context();
//avformat_alloc_output_context2(&avForCtx, NULL, "flv", filename);
	avForCtx->oformat = av_guess_format("flv", filename, NULL);
	if (!avForCtx) {
		LOGD("avformat_alloc_output_context2 error");
		return -1;
	}
	if (avForCtx->oformat != NULL) {
		LOGD("-flv Can Guess Format-");

	} else {
		LOGD("-flv Can not Guess Format-");
		return -1;

	}
	strcpy(avForCtx->filename, filename);
	video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (video_codec != NULL) {
		LOGD("找到视频解码器：%s", video_codec->name);
		avForCtx->oformat->video_codec = AV_CODEC_ID_H264;
	} else {
		LOGD("没有找到视频解码器");
		return -2;
	}
	audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (audio_codec != NULL) {
		LOGD("找到音频解码器：%s", audio_codec->name);
		avForCtx->oformat->audio_codec = AV_CODEC_ID_AAC;
	} else {
		LOGD("没有找到音频解码器");
		return -3;
	}
	AVRational videoRate = av_d2q(pframeRate, 1001000);

	LOGD("frame rate : %d", videoRate.num);
	video_st = avformat_new_stream(avForCtx, video_codec);
	if (video_st == NULL) {
		return -4;
	}

	video_c = video_st->codec;
	video_c->codec_id = AV_CODEC_ID_H264;
	video_c->codec_type = AVMEDIA_TYPE_VIDEO;
	video_c->bit_rate = 180000;
	video_c->width = (pwidth + 15) / 16 * 16;
	video_c->height = pheight;
	video_c->time_base = av_inv_q(videoRate);
	video_st->time_base = av_inv_q(videoRate);
	//video_c->gop_size = 12;
	video_c->pix_fmt = AV_PIX_FMT_YUV420P;
	video_c->profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
	video_c->global_quality = FF_QP2LAMBDA * 0;
	if ((avForCtx->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
		video_c->flags = video_c->flags | CODEC_FLAG_GLOBAL_HEADER;
	}
	if ((video_codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
		video_c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	}
	if ((audio_st = avformat_new_stream(avForCtx, 0)) == NULL) {
		return -5;
	}
	audio_c = audio_st->codec;
	audio_c->codec_id = avForCtx->oformat->audio_codec;
	audio_c->codec_type = AVMEDIA_TYPE_AUDIO;
	audio_c->bit_rate = 32000;
	audio_c->sample_rate = paudioRate;

	audio_c->time_base.num = 1;
	audio_c->time_base.den = paudioRate;
	audio_st->time_base.num = 1;
	audio_st->time_base.den = paudioRate;
	audio_c->channels = 1;
	audio_c->channel_layout = av_get_default_channel_layout(audio_c->channels);

	audio_c->sample_fmt = AV_SAMPLE_FMT_S16; //AV_SAMPLE_FMT_FLTP AV_SAMPLE_FMT_S16;

	audio_c->bits_per_raw_sample = 16;
	if ((avForCtx->oformat->flags & AVFMT_GLOBALHEADER) != 0) {
		audio_c->flags = audio_c->flags | CODEC_FLAG_GLOBAL_HEADER;
	}
	if ((audio_codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
		audio_c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	}
	av_dump_format(avForCtx, 0, filename, 1);
	AVDictionary *options = NULL;
	//av_dict_set(&options, "crf", "0", 0);
	av_dict_set(&options, "profile", "baseline", 0);
	av_dict_set(&options, "level", "3.1", 0);
	av_dict_set(&options, "preset", "superfast", 0);
	av_dict_set(&options, "tune", "zerolatency", 0);

	if (avcodec_open2(video_c, video_codec, &options) < 0) {
		LOGD("没有打开视频");
		return -5;
	}

	av_dict_free(&options);
	video_outbuf = NULL;
	if ((avForCtx->oformat->flags & AVFMT_RAWPICTURE) == 0) {
		video_outbuf_size = avpicture_get_size(video_c->pix_fmt, video_c->width,
				video_c->height);
		video_outbuf = av_malloc(video_outbuf_size);
	}
	if ((picture = av_frame_alloc()) == NULL) {
		return -6;
	}
	picture->pts = 0;
//	picture->format = video_c->pix_fmt;
//	picture->width = video_c->width;
//	picture->height = video_c->height;

	int size = avpicture_get_size(video_c->pix_fmt, video_c->width,
			video_c->height);
	picture_buf = av_malloc(size);
	AVDictionary *metadata1 = NULL;
	video_st->metadata = metadata1;
	options = NULL;
	av_dict_set(&options, "crf", "0", 0);
	if ((ret = avcodec_open2(audio_c, audio_codec, &options)) < 0) {
		LOGD("没有打开音频 %d", ret);
		return -8;
	}
	av_dict_free(&options);
	audio_outbuf_size = 256 * 1024;
	audio_outbuf = av_malloc(audio_outbuf_size);
	if (audio_c->frame_size <= 1) {

		audio_outbuf_size = FF_MIN_BUFFER_SIZE;
		audio_input_frame_size = audio_outbuf_size / audio_c->channels;
		if (audio_c->codec_id == AV_CODEC_ID_PCM_U16BE) {
			audio_input_frame_size >>= 1;
		}
		LOGD("1111audio_c->frame_size %d", audio_input_frame_size);
	} else {
		audio_input_frame_size = audio_c->frame_size;
		LOGD("2222audio_c->frame_size %d", audio_input_frame_size);
	}

	int planes = 1;
	data_size = av_samples_get_buffer_size(NULL, audio_c->channels,
			audio_input_frame_size, audio_c->sample_fmt, 1) / planes;
	audioframe = av_frame_alloc();
	audioframe->pts = 0;
	audioframe->nb_samples = audio_input_frame_size;
	audioframe->format = audio_c->sample_fmt;

	frame_buf = (uint8_t *) av_malloc(data_size);
	ret = avcodec_fill_audio_frame(audioframe, audio_c->channels,
			audio_c->sample_fmt, (const uint8_t*) frame_buf, data_size, 0);
	if (ret < 0) {
		LOGD("avcodec_fill_audio_frame  :%d", ret);
		return -2;
	}
	av_init_packet(&audio_pkt);
	AVDictionary *metadata2 = NULL;
	audio_st->metadata = metadata2;
	if ((avForCtx->oformat->flags & AVFMT_NOFILE) == 0) {

		if (ret = avio_open(&avForCtx->pb, filename, AVIO_FLAG_WRITE) < 0) {
			LOGD("avio_open failed:%d", ret);
			return -10;

		}
	}
	ret = avformat_write_header(avForCtx, NULL);
	if (ret < 0) {
		LOGD("avformat_write_header failed %d", ret);
		return -3;
	}
	isready = 1;
	return 0;
}

