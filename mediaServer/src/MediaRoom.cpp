#include "MediaRoom.h"
#include <thread>
#include <arpa/inet.h>
#include "Log.h"

MediaRoom::MediaRoom(MediaServer* mediaServer, std::string path) {
	//demuxer("/home/tony/myprojects/MovieHub/resource/喜剧之王.mp4");，错误，必须使用参数列表，拷贝构造等，这里直接使用初始化列表了，
	//初始化列表是执行这个Room的拷贝构造之前执行的，可以看lbAgent，那里也碰到了这个问题
	//这里必须要使用*，不能使用嵌套，因为https://www.codenong.com/36371940/
	demuxer = new Avdemuxer(path);	//这个必须要在RTP初始化，因为RTP初始化的时候需要用到demuxer里面读到的信息
	rtpPacketManager = new RTP(mediaServer, this, demuxer->videoStreamId, demuxer->audioStreamId, 
		90000,	//时钟频率 一秒定义为90000	也可以使用demuxer，不过这个一般也设为90000
		av_q2d(demuxer->avFormatContext->streams[demuxer->videoStreamId]->avg_frame_rate),		//视频的fps	还有其他方法，这个只是简单的
		//av_q2d https://blog.csdn.net/Griza_J/article/details/126118779
		demuxer->avFormatContext->streams[demuxer->audioStreamId]->codecpar->profile,	//一般为FF_PROFILE_AAC_LOW  1
		demuxer->avFormatContext->streams[demuxer->audioStreamId]->codecpar->sample_rate);	//音频的采样率，一般为44100
	isRunning = false;
	userNums = 0;
	//rtspManager = new RTSP(mediaServer, this);
	rtspManager = nullptr;
	//加入临时用户用于测试
	OnlineUser userTemp;
	userTemp.uid = 1;
	userTemp.state = PLAY;
	userTemp.curAudioIdx = 1;
	userTemp.curVideoIdx = 1;
	userTemp.addr.sin_family = AF_INET;
	userTemp.addr.sin_port = htons(20000);
	userTemp.addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	users.push_back(userTemp);
	userNums = 1;
}

//给当前用户发送一帧音频帧数据
void MediaRoom::sendFrame(enum FrameType frameType,int sleep_ms) {	//随眠多少ms
	//遍历user列表，发送消息
	while (1) {
		for (auto it = users.begin(); it != users.end(); it++) {
			if (it->state == PLAY) {
				int* curFrameIdxPtr = nullptr;
				//注意这里不能直接++curVideoIdx或者curAudioIdx，会导致即使发不成功也++，可以见bug37
#if 0
				if (frameType == VIDEO) {
					curFrameIdx = it->curVideoIdx;
					it->curVideoIdx++;
				}
				else {
					curFrameIdx = it->curAudioIdx;
					it->curAudioIdx++;
				}
#endif
				if (frameType == VIDEO) {
					curFrameIdxPtr = &it->curVideoIdx;	//注意->优先级高于&
				}
				else {
					curFrameIdxPtr = &it->curAudioIdx;
				}
				int sentFrameIdx = rtpPacketManager->sendFrame(frameType, *curFrameIdxPtr, it->addr);
				if (sentFrameIdx > 0) {	//只有发送成功，才会操作下标，如果发送不成功，就啥都不干
					if (sentFrameIdx == *curFrameIdxPtr) {	//如果发送的和要求发送的下标一致，则++
						(*curFrameIdxPtr)++;
					}
					else {
						//如果不一致，那就设为已发送的帧+1
						(*curFrameIdxPtr) = sentFrameIdx + 1;
					}
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
	}
}

//增加删除用户
void MediaRoom::addUser() {}
void MediaRoom::removeUser() {}
//启动房间
void MediaRoom::run() {
	isRunning = true;
	//启动解封装器
	Debug("启动MediaRoom的run函数");
	demuxer->run();
	//启动rtp
	Debug("启动rtpPacketManager的run函数");
	rtpPacketManager->run();
	//启动一个线程，发送一帧视频数据
	Debug("启动sendVideoFrame");
	//这里的线程休眠参数直接使用rtp的了。感觉都一样的，不需要额外的设置
	std::thread sendVideoFrame(&MediaRoom::sendFrame, this, VIDEO, rtpPacketManager->aThreadSleepMs);
	//启动一个线程，定时的发送一帧音频数据
	Debug("启动sendVideoFrame");
	std::thread sendAudioFrame(&MediaRoom::sendFrame, this, AUDIO, rtpPacketManager->vThreadSleepMs);
	sendVideoFrame.detach();
	sendAudioFrame.detach();
}