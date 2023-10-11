#pragma once
//包含需要的ffmpeg头文件
extern "C" {
	//必须要使用extern c https://blog.csdn.net/ustcxxy/article/details/32088383
	#include <libavformat/avformat.h>	//libavformat
	#include <libavcodec/bsf.h>	//视频添加sps pps的时候需要指定编码器，网上都是avcodec.h但是我的不行，已经记录到bug8中了
	#include<libavcodec/avcodec.h>		//需要使用#define FF_PROFILE_AAC_LOW  1
}


