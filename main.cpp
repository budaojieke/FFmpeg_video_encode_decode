// ffmpegdemo.cpp : 定义控制台应用程序的入口点。
//
 
#include "stdafx.h"
#include <stdio.h>
 
#define __STDC_CONSTANT_MACROS
 
#ifdef _WIN32
 
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};
#else
 
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
 
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif
 
int main()
{
	//文件格式上下文
	AVFormatContext	*pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
	int		i = 0, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame, *pFrameYUV;
	unsigned char *out_buffer;
	AVPacket *packet;
 
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	char input_file[] = "input.mkv";
    const char* out_file = "ds.h264";
    
	FILE *fp_yuv = fopen("output.yuv", "wb+");
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, input_file, NULL, NULL) != 0) 
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	//读取一部分视音频数据并且获得一些相关的信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) 
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
 
	//查找视频编码索引
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
 
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
 
	//编解码上下文
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	//查找解码器
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) 
	{
		printf("Codec not found.\n");
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) 
	{
		printf("Could not open codec.\n");
		return -1;
	}
/***************************************/
    //Guess Format
	fmt = av_guess_format(NULL, out_file, NULL);
	pFormatCtx->oformat = fmt;

	//Open output URL
	if (avio_open(&pFormatCtx->pb,out_file, AVIO_FLAG_READ_WRITE) < 0){
		printf("Failed to open output file! \n");
		return -1;
	}

	video_st = avformat_new_stream(pFormatCtx, 0);

	if (video_st==NULL){
		return -1;
	}
#if 1
	//Param that must set
	pCodecCtx = video_st->codec;
	//pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_w;  
	pCodecCtx->height = in_h;
	pCodecCtx->bit_rate = 400000;  
	pCodecCtx->gop_size=250;

	pCodecCtx->time_base.num = 1;  
	pCodecCtx->time_base.den = 25;  

	//H264
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 51;

	//Optional Param
	pCodecCtx->max_b_frames=3;
#endif
	// Set Option
	AVDictionary *param = 0;
	//H.264
	if(pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
	}

	//Show some Information
	av_dump_format(pFormatCtx, 0, out_file, 1);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec){
		printf("Can not find encoder! \n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec,&param) < 0){
		printf("Failed to open encoder! \n");
		return -1;
	}
/***************************************/
	//申请AVFrame，用于原始视频
	pFrame = av_frame_alloc();
	//申请AVFrame，用于yuv视频
	pFrameYUV = av_frame_alloc();
	//分配内存，用于图像格式转换
	int picture_size;
	picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	out_buffer = (unsigned char * *)av_malloc(picture_size);
	avpicture_fill((AVPicture *)pFrame, out_buffer, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	//Write File Header
	avformat_write_header(pFormatCtx,NULL);
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	//手工调试函数，输出tbn、tbc、tbr、PAR、DAR的含义
	av_dump_format(pFormatCtx, 0, input_file, 0);
	printf("-------------------------------------------------\n");
 
	//申请转换上下文
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
	pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
 
	//读取数据
	while (av_read_frame(pFormatCtx, packet) >= 0) 
	{
		if (packet->stream_index == videoindex) 
		{
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) 
			{
				printf("Decode Error.\n");
				return -1;
			}
 
			if (got_picture >= 1) 
			{
				//成功解码一帧
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);//转换图像格式
 
				y_size = pCodecCtx->width*pCodecCtx->height;
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				printf("Succeed to decode 1 frame!\n");
			}
			else
			{
				//未解码到一帧，可能时结尾B帧或延迟帧，在后面做flush decoder处理
			}
		}
		av_free_packet(packet);
	}
 
	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (true) 
	{
		if (!(pCodec->capabilities & AV_CODEC_CAP_DELAY))
			return 0;
 
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (ret < 0)
		{
			break;
		}
		if (!got_picture)
		{
			break;
		}
 
		sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize);
 
		int y_size = pCodecCtx->width*pCodecCtx->height;
		fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
		fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
		printf("Flush Decoder: Succeed to decode 1 frame!\n");
	}
 
	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	fclose(fp_yuv);
 
    return 0;
}
