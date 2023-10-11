#include "rtp.h"
#include "MediaServer.h"
#include "MediaRoom.h"
#include "Log.h"

RTP::RTP(MediaServer* mediaServer, MediaRoom* mediaRoom, int videoStreamId, int audioStreamId, int clockRate, int fps, int profile, int sampleRate)
	:videoCatch(VIDEO, clockRate, fps),
	audioCatch(AUDIO, clockRate, profile, sampleRate){
	//初始化两个rtp头
	//参考连接：https://blog.csdn.net/huabiaochen/article/details/104576088 和 https://www.cnblogs.com/liushui-sky/p/13846456.html
	rtpHeaderInit(&videoCatch.rtpHeader, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0, 0, 0, 0x88923423);
	rtpHeaderInit(&audioCatch.rtpHeader, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_AAC, 1, 0, 0, 0x32411);
	m_mediaServer = mediaServer;
	m_mediaRoom = mediaRoom;

	//这边在获得帧之后，需要判断当前帧类型
	//存储音频流和视频流的索引
	m_videoStreamId = videoStreamId;
	m_audioStreamId = audioStreamId;
	//求出各个线程获取帧的休眠时间 -5主要是为了有库存
	vThreadSleepMs = 1000 / fps - 5;	
	aThreadSleepMs = FrameList::getNbSamples(profile) * 1000 / sampleRate  - 5;
}

void RTP::sendRtpPacket(mediaService::RtpPacket* rtpPacket, sockaddr_in& toAddr)
{
	//注意如果想更改header，需要使用mutable_header()，这个返回的是指针，而不是header()
	rtpPacket->mutable_header()->set_base(htonl(rtpPacket->header().base()));//从主机字节顺序转变成网络字节顺序
	rtpPacket->mutable_header()->set_timestamp(htonl(rtpPacket->header().timestamp()));
	rtpPacket->mutable_header()->set_ssrc(htonl(rtpPacket->header().ssrc()));

	m_mediaServer->udpSendMsg(mediaService::ID_TransRtpPacket, rtpPacket, &toAddr);

	//把这不小心注销之后，时间戳就变了，一会增加100，一会增加1,。。。。。。。。序列号也是，所以这些一定不能少
	rtpPacket->mutable_header()->set_base(ntohl(rtpPacket->header().base()));//从主机字节顺序转变成网络字节顺序
	rtpPacket->mutable_header()->set_timestamp(ntohl(rtpPacket->header().timestamp()));
	rtpPacket->mutable_header()->set_ssrc(ntohl(rtpPacket->header().ssrc()));
}

int RTP::sendFrame(enum FrameType frameType, int frameIdx, sockaddr_in& toAddr) {
	//加锁
	mutex.lock();
	FrameInfo* frame = getFrameInfo(frameType, frameIdx);
	if (frame != nullptr) {
		if (frameType == VIDEO)Debug("当前发送的帧类型：视频");
		if (frameType == AUDIO)Debug("当前发送的帧类型：音频");
		Debug("当前发送的第几帧：%d	当前帧需要发送几个rtp包：%d", frameIdx, frame->packetNums);
		std::list<mediaService::RtpPacket*>::iterator it = frame->realAddr;
		for (int i = 0; i < frame->packetNums; i++) {
			sendRtpPacket(*it, toAddr);
			it++;
		}
		frame->sendTimes++;
		//当前包发送完成，
	}
	mutex.unlock();
	return frame == nullptr ? 0 : frame->frameIdx;
}

FrameInfo* RTP::getFrameInfo(enum FrameType frameType, int frameIdx) {
	FrameList* frameList = nullptr;
	if (frameType == VIDEO)	frameList = &videoCatch;
	else if (frameType == AUDIO) frameList = &audioCatch;
	else { std::cout << "帧类型错误" << std::endl; return nullptr; }

	//如果当前缓存帧的个数为0，则必须要催促赶紧获取帧
	//我这急需要发一帧，所以可以强制获取一帧，注意这个函数，如果解封装没有解完，肯定能获取一帧
	//注意这里并不设计为frameIdx > frameList->startFrameIdx + frameList->frameNum - 1这种形式，两者情况不一样
	if (frameList->frameNum == 0) {
		//如果为0，那就不直接等到休眠了，而是直接获取
		getOnePacket(frameType);
	}

	//有了idx2Addr，就不用遍历frames了
	std::cout << "要取的帧：" << frameIdx << "\t当前列表起始帧：" << frameList->startFrameIdx << "\t当前列表结束帧：" << frameList->startFrameIdx + frameList->frameNum - 1 << std::endl;
	if (frameIdx < frameList->startFrameIdx) {
		std::cout << "帧获取过慢，已返回当前缓存的第一帧" << std::endl; return nullptr;
		//说明当前客户端播放的太慢了，需要播放第一帧，这个时候应该告诉room这件事情
		return frameList->frames[frameList->startFrameIdx];
	}
	else if (frameIdx > frameList->startFrameIdx + frameList->frameNum - 1) {  //注意这里一定要减1，否则会段错误
		std::cout << "帧获取超前" << std::endl; 
		return nullptr;
	}
	//正常帧
	else {
		std::cout << "正在返回帧：" << frameIdx << std::endl;
		std::cout << "当前帧是第几帧：" << frameList->frames[frameIdx]->frameIdx;
		return frameList->frames[frameIdx];
	}
	return nullptr;
}

void RTP::getPacket(enum FrameType frameType, int sleep_ms) {
	//获取当前fps，然后设置休眠时间
	while (1) {
		getOnePacket(frameType);
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
	}
}

void RTP::getOnePacket(enum FrameType frameType) {
	//获取当前音频采样率，然后设置休眠时间
	mutex.lock();
	AVPacket* packet = nullptr;
	if (frameType == VIDEO) {
		packet = m_mediaRoom->demuxer->getFrame(m_mediaRoom->demuxer->videoFrame);
		tempVideoGetTimes += 1;
	}
	else {
		packet = m_mediaRoom->demuxer->getFrame(m_mediaRoom->demuxer->audioFrame);
		tempAudioGetTimes += 1;
	}
	//将当前帧封装成rtp包
	avPacket2Rtp(packet);
	//注意释放packet，这样就可以保证avdemuxer不存在内存泄漏
	if(frameType == VIDEO)printRtp(&videoCatch);
	if(frameType == AUDIO)printRtp(&audioCatch);
	av_packet_unref(packet);
	mutex.unlock();
}

void RTP::avPacket2Rtp(AVPacket* frame) {
	//注意打包完成之后，别忘记释放掉avpacket
	//sps和pps都打包在idr帧里面了

	//获取帧类型，是视频帧还是音频帧
	FrameList* frameList = nullptr;
	FrameType frameType;
	if (frame == nullptr) {
		std::cout << "空地址包裹，已返回" << std::endl;
		return;
	}
	if (frame->stream_index == m_videoStreamId) {
		frameList = &videoCatch;
		frameType = VIDEO;
	}
	else if (frame->stream_index == m_audioStreamId) {
		frameList = &audioCatch;
		frameType = AUDIO;
	}
	else {
		std::cout << "avPacket2Rtp帧类型错误" << std::endl;
		return;
	}
	//注意 i帧可能产生很多帧数据，可以过滤掉aud帧，直接舍弃即可（一般就在开头，如果是视频帧）
	//rtp形式：如果是打包h264，那么应该有rtpHeader+2字节的FU indicator和FU header + nalu_header(type)+nalu_data
	if (frameType == VIDEO) {
		//先过滤掉aud的帧 0 0 0 1 9 f0
		int startIdx = 0;
		uint8_t audNalu[] = { 0,0,0,1,9,0xf0 };
		if (memcmp(audNalu, frame->data, 6) == 0) {
			startIdx = 6;
		}
		Debug("frame的大小%d, startIdx：%d", frame->size, startIdx);
		//如果是i帧，那么前面肯定有3个帧，那么需要拆开成4个帧
		//那我就不管了，直接从头搜索字符串，然后碰到一个startCode，就找到下一个的字符串，然后开始将中间的这个nalu打包，直到文件尾
		//字符指针，指到末尾，当前avpacket也就分析完了
		int i = startIdx;	//这里指向帧的startCode
		//当前分析的帧的起止位置
		int curFrameStart = -1;
		int nextFrameStart = -1;
		while (i < frame->size) {
			//先找到起始位置
			//注意 每次数组的起始位置都是startCode，然后，每一帧在分割的的时候不应该包含开始的间隔符，因为间隔符包含一些信息，需要客户端解析，
			int startKind = edgeFlag(frame->data + i, 0); //从第i+0开始比较，返回当前帧的类型
			if (startKind == 0) {
				std::cout << "当前视频帧起始符号错误，已忽略" << std::endl;
				return; //如果当前帧起始符错误，则不处理
			}
			i += startKind;	//i += 代表已经处理
			curFrameStart = i;	//指向帧的实际内容的第一个字节，也就是NAL unit heade那个字节

			//找到当前帧的结束位置
			while (!edgeFlag(frame->data + i, 0) && i < frame->size) {
				i++;
			}
			//跳出while循环说明，要么当前i指向的是一个startCode的第一个字节，要么已经到达数组末尾
			if (i != frame->size) {
				nextFrameStart = i;	//这里减1不减1都行，看习惯，不要减1了，因为如果减1，那么后面的size等都要+1等
			}
			else {
				nextFrameStart = frame->size;	//注意这里是指向的是不属于自己的位置
			}
			//i不用操作了，因为i指向的是一个startCode的第一个字节，要么已经到达数组末尾

			//然后下面对curFrameStart和curFrameEnd的数据进行操作
			int frameSize = nextFrameStart - curFrameStart;
			//先获得包的naluType，因为后面判断是否需要加上时间片
			uint8_t naluType = frame->data[curFrameStart] & 0x1F;	//低5位存的是type
			uint8_t fNRI = frame->data[curFrameStart] & 0xE0;		//高3位下面会用到
			//单包打包，注意单一打包和分包，的结构不一样，单一打包很简单，只需要拷贝过去就行
			//rtpHeader + (nalu_header(type) + nalu_data)
			//https://blog.csdn.net/u010178611/article/details/82592393 和 https://www.cnblogs.com/cyyljw/p/8858226.html 
			//和 https://zhuanlan.zhihu.com/p/25685166等都介绍了相关的打包模式
			if (frameSize <= 1400) {	//这里需要等于，因为下面else在求个数时，包含1400，也就是如果被分到1400被分到else，那其实还是单包发送
				//创建一个rtp包即可
				mediaService::RtpPacket* videoRtpPacket = new mediaService::RtpPacket();
				//需要注意怎么初始化的
				videoRtpPacket->mutable_header()->CopyFrom(videoCatch.rtpHeader);	//填充rtpHeader		
				//注意，c字符串的一部分，转为string，这样写没有问题，可以在线简单测一下，不行，char是可以进行下面的操作的，但是uint8_t是不行的，会报错
				//需要对指针类型进行一下强转，不行，详情看音频的那里定义的地方，会出很多问题，还是使用uint8_t
				uint8_t payload[frameSize];
				Debug("当前rtpPacket的大小：%d", frameSize);
				memcpy(payload, frame->data + curFrameStart, frameSize);
				//这个方式需要和上面做区分，这个是内置类型
				videoRtpPacket->set_payload(std::string((char*)payload, frameSize));		//这个并不够高效

				//更新frameInfo和frameList
				//这个更新方式和aac的那个方式一样的，我就直接复制过来了
				
				updateFrameList(&videoCatch, videoRtpPacket, 1);

				//注意这个自增的操作，如果直接 videoCatch.rtpHeader.base()++赋值进去，那是先赋值再+，而++videoCatch.rtpHeader.base()则会报错，不支持
				uint32_t updatedSeq = videoCatch.rtpHeader.base();
				updatedSeq++;
				videoCatch.rtpHeader.set_base(updatedSeq);
				//注意为啥这里判断，多包打包不判断，因为sps pps，sei不可能超过1400字节
				//不是PPS和SPS（7和8类型的）和6那种类型的的要加时间戳（6,7,8不是视频数据，不能累计）
				if (!(naluType == 7 || naluType == 8 || naluType == 6)) {
					//注意rtp的timestamp定义的一秒为90000 https://blog.csdn.net/liguan1102/article/details/96323986
					//https://blog.csdn.net/freeabc/article/details/116769830 标准文档里的
					videoCatch.rtpHeader.set_timestamp(videoCatch.rtpHeader.timestamp() + videoCatch.duration);
				}
			}
			//多包打包
			else {
				//内存结构为：RTP_header + FU_identifier + FU_Header + FU_payload（这个是不包含nalutype的纯数据部分）
				//参考链接：https://blog.csdn.net/u010178611/article/details/82592393 以及我之前做项目这里出现的问题，
				//那个项目的注释里记录： 记录Frame帧拷贝到哪了  ,竟然不包括第一个字节，也就是将第一个字节换成下面要封装的两个字节！！！！！！
				//先记录当前发的包的个数，和最后一个包的长度
				int fullPacketNums = frameSize / 1400;	//多少个完整包
				int lastPacketBytes = frameSize % 1400;
				//总的rtp包数
				int packetNums = lastPacketBytes == 0 ? fullPacketNums : fullPacketNums + 1;

				//rtpHeader已经准备好了

				//内部还要再来一个while，这个包的个数还不确定
				//完整包和剩余包一起发送，网上都是分成3部分发的，冗余代码太多，我之前的代码是两次发完的，这里直接一个while发出去

				//需要再使用一个指针j，指向当前curFrameStart中处理到哪里了（因为中间要拷贝的）
				//注意这里为啥是curFrameStart + 1，因为nalutype那个字节不会拷贝的，需要被忽略，所以这里就直接加1了
				int j = curFrameStart + 1;
				Debug("packetNums：%d, fullPacketNums：%d", packetNums, fullPacketNums);
				while (packetNums) {
					//准备工作，创建videoRtpPacket，并填充一些东西
					mediaService::RtpPacket* videoRtpPacket = new mediaService::RtpPacket;
					videoRtpPacket->mutable_header()->CopyFrom(videoCatch.rtpHeader);	//填充rtpHeader		
					//准备工作，先求出当前包大小，以及初始化数组
					//注意，注意，注意，curFrameStart对应的那个字节需要舍弃，
					//确定最后一帧的rtp长度
					int packetSize = 0;
					if (packetNums == 1) {	//是最后一帧，最后一帧应该是1，packerNums如果为00，不会进入这个循环体的
						packetSize = lastPacketBytes == 0 ? 1400 : lastPacketBytes;		//如果lastPacketBytes == 0说明最后一个包是1400字节，如果不为0，说明最后一个包是lastPacketBytes字节
					}
					else {
						packetSize = 1400;
					}
					//不要使用std::string 会出很多问题，使用uint8_t再转换
					//std::string payload;
					uint8_t payload[2 + packetSize];	//注意这里的packetSize是不包含nalutype那个字节的，所以不用-1
					//先初始化FU_identifier + FU_Header

					//填充FU_identifier，高三位与NALU第一个字节的高三位相同，低5位为28，h264规范中规定的
					payload[0] = fNRI | 0x1C;  //0x1c为低5位的28

					//填充FU_Header
					//S：标记该分片打包的第一个RTP包(0) E：比较该分片打包的最后一个RTP包(1) R: 保留位，我看网上都是0  Type：NALU的Type(3-7)
					//如果为第一帧，则将这一帧的第0位置1，低5位和NALU的Type的Type一致
					if (j == curFrameStart + 1) {	//说明是第一帧	//注意这里为啥是 curFrameStart + 1
						payload[1] = 0x80 | naluType;//0x80为高三位（ser为100），naluType为低5位
						//FrameList
						updateFrameList(&videoCatch, videoRtpPacket, packetNums);
					}
					//最后一帧
					//这里为1,注意
					else if (packetNums == 1) {
						//如果为最后一帧（lastPacketBytes不管为不为0，前面都判断好了），则将这一帧的第1位置1，低5位和NALU的Type的Type一致
						payload[1] = 0x40 | naluType;//0x40（ser为010）为高三位，naluType为低5位
						//如果不是第一帧，直接push即可
						videoCatch.rtpPacket.push_back(videoRtpPacket);
					}
					else {
						//中间帧
						payload[1] = naluType;	//ser为000
						//如果不是第一帧，直接push即可
						videoCatch.rtpPacket.push_back(videoRtpPacket);
					}
					//将实际数据复制到payload，注意如果是
					//这里的j不用判断是否为nalutype那个字节，因为while外面已经判断过了
					memcpy(payload + 2, frame->data + j, packetSize);
					//将payload放到videoRtpPacket的payload
					//videoRtpPacket->set_payload(std::string((char*)payload, packetSize));		//这个并不够高效  错的 需要+2 bug40
					videoRtpPacket->set_payload(std::string((char*)payload, packetSize + 2));		//这个并不够高效

					//更新j
					j += packetSize;	//演示了一下，发现是没有问题的
					//然后更新rtpHeader，为下一帧做准备
					//seq++，这个还真不好+，不对可以直接对base++，因为base就是在低16位
					//注意这个base++的真正含义
					//这些方式都是按照上面来的
					uint32_t updatedSeq = videoCatch.rtpHeader.base();
					updatedSeq++;
					videoCatch.rtpHeader.set_base(updatedSeq);

					//当前包处理完，减去1
					packetNums--;
				}
				//当前帧存储完，才更新时间戳
				videoCatch.rtpHeader.set_timestamp(videoCatch.rtpHeader.timestamp() + videoCatch.duration);
			}
		}
	}
	//上面限制了要么是video，要么是audio
	//如果打包的是aac应该有rtpHeader+4个字节+ adts_data（一般只有一帧，也就是几百个字节）（注意adts_header(7字节)在封装的时候舍弃了。。。。因为rtp头已经包含相关信息了）
	else {
		//目前只处理1400的帧，大于的话直接忽略
		if (frame->size < 1400) {
			mediaService::RtpPacket* audioRtpPacket = new mediaService::RtpPacket;		//	一般音频帧很固定的几百字节，所以这里定义一个就行了
			//先获取adts头相关信息
			size_t adtsHeaderSize = 0;		//这里必须使用size_t否则下面这行会报错，无法转换
			uint8_t* adtsHeader = av_packet_get_side_data(frame, AV_PKT_DATA_NEW_EXTRADATA, &adtsHeaderSize);
			int frameSize = frame->size;  //注意帧大小不包括adts头的（https://blog.csdn.net/huabiaochen/article/details/104576088）
			//rtpHeader已经准备好
			audioRtpPacket->mutable_header()->CopyFrom(audioCatch.rtpHeader);	//填充rtpHeader	
			//初始化payload 注意，uint8_t操作，不要使用string，会有意想不到的错误，比如下面的一段程序，就会出错
			/*
				#include <iostream>
				using namespace std;

				int main()
				{
					uint8_t a[] = {0, 1, 2, '=','5','='};
					std::string temp(reinterpret_cast<char*>(a),2,2);
					//temp.append(a[1], 3)
					std::cout << temp << std::endl;
				}
			*/
			uint8_t payload[frameSize + 4];
			payload[0] = 0x00;	//第一个字节必须为0x00
			payload[1] = 0x10;	//第二个字节必须为0x10
			payload[2] = (frameSize & 0x1FE0) >> 5;	//第三个字节保存数据大小的高八位。frameSize & 0x1FE0是获取size的高8位
			//（因为最多只保存13bit，所以是从低6位往高数8位的，所以就是0x1FE0），然后往低5位右移5位即可得到，强制赋值给一个字节刚刚好（会截断）
			payload[3] = (frameSize & 0x1F) << 3;		//第四个字节的高5位保存数据大小的低5位。frameSize & 0x1F得到低5位的值（8位，那么低5位往高移三位即可）
			//然后是不带adts头的数据
			//payload += frame->data;  //为什么可以直接加，可以看https://blog.csdn.net/qq_45909595/article/details/104152595 这是string的操作
			memcpy(payload + 4, frame->data, frameSize);
			//如果不想赋值，可以一开始就操作audioRtpPacket.payload()
			//audioRtpPacket->set_payload(std::string((char*)payload, frameSize));		//这个并不够高效	错的需要+4 bug40
			audioRtpPacket->set_payload(std::string((char*)payload, frameSize + 4));	
			//将得到的结果audioRtpPacket更新到framInfo和franeList,需要更新的有framInfo的全部，franeList的rtpPacket frames frameNum startFrameIdx
			// startFrameIdx怎么更新，不可能size为0，那就定为0，size不为0，就设置为begin的值，中间size也可能为0，可以判断startFrameIdx是否为0
			//FrameList
#if 0 
			//这个startFrameIdx需要在插入之前更新，因为我需要知道插入之前的状态
			//如果当前队列为空，且idx也为0，说明以前没开启，所以设为1，如果不为size不为空，那么起始帧idx，根本不需要动
			//这个相当于间接的给整个视频的帧编序号了
			if (audioCatch.rtpPacket.size() == 0) {
				if (audioCatch.startFrameIdx == -1) {  //初始化为-1
					audioCatch.startFrameIdx = 1;
				}
				//如果size为0，但是startFrameIdx不为0，那么+1即可，因为这种情况代表所有的帧都发送完全了，startFrameIdx最终会指向队尾的，因为如果我删除的话，也会更新这个值
				//这个时候又来了一个帧，那就+1即可，不过如果清除old帧的时候，不更新startFrameIdx，那肯定有问题的，逻辑就不对，而且不可能从后往前清理，而且必须是一帧一帧的清理，
				//所以不用担心，但是操作的时候必须mutex
				else {
					audioCatch.startFrameIdx += 1;
				}
			}
#endif 
#if 0 
			//合在一个函数里了，这里就直接注释，但是为了防止原始的思想丢失，这里就直接注释
			//上面这个注释是不对的，rtpPacket和startFrameIdx是没关系的，应该是frames
			if (audioCatch.frames.size() == 0) {
				if (audioCatch.startFrameIdx == -1) {  //初始化为-1
					audioCatch.startFrameIdx = 1;
				}
				//如果size为0，但是startFrameIdx不为0，那么+1即可，因为这种情况代表所有的帧都发送完全了，startFrameIdx最终会指向队尾的，因为如果我删除的话，也会更新这个值
				//这个时候又来了一个帧，那就+1即可，不过如果清除old帧的时候，不更新startFrameIdx，那肯定有问题的，逻辑就不对，而且不可能从后往前清理，而且必须是一帧一帧的清理，
				//所以不用担心，但是操作的时候必须mutex
				else {
					audioCatch.startFrameIdx += 1;
				}
			}
			//如果audioCatch.rtpPacket.size() == 0，不为0，那就不需要更新startFrameIdx
			//当前帧是整个音视频的第几帧怎么获取，这个帧肯定是放在最后的，所以当前帧是startFrameIdx + frameNum（注意此时frameNum还没更新，因为startFrameIdx本身是从1开始的）
			int frameIdx = audioCatch.startFrameIdx + frameNum;
			audioCatch.frameNum += 1;

			audioCatch.rtpPacket.push_back(audioRtpPacket);	//注意这里压入的是指针

			FrameInfo* audioFrame = new FrameInfo;
			audioCatch.frames[frameIdx] = audioFrame;

			//更新audioFrame
			audioFrame->rtpPacketSize = 1;		//音频帧的个数都为1
			//audioFrame->realAddr = audioCatch.rtpPacket.back(); 这里不应该是back，而应该是当前帧的rtpPacket中的第一个，视频帧肯定不能这样写，而且back返回的是值
			audioFrame->realAddr = audioCatch.rtpPacket.rbegin();	//最后一个元素 //先这样写吧，后面再更新
			audioFrame->sendTimes = 0;
			audioFrame->frameIdx = frameIdx;
			audioFrame->frameModid = AUDIO;
#endif
			updateFrameList(&audioCatch, audioRtpPacket, 1);
			//然后更新rtpHeader，为下一帧做准备
			//seq++，这个还真不好+，不对可以直接对base++，因为base就是在低16位
			//这些方式都是按照上面来的
			uint32_t updatedSeq = audioCatch.rtpHeader.base();
			updatedSeq++;
			audioCatch.rtpHeader.set_base(updatedSeq);
			//当前帧存储完，才更新时间戳
			audioCatch.rtpHeader.set_timestamp(audioCatch.rtpHeader.timestamp() + audioCatch.duration);
			return;
		}
		std::cout << "当前音频帧过大，已忽略" << std::endl;
	}
}

int RTP::edgeFlag(const uint8_t* buf, int i) {
	//这里也可以使用memmem
	int edgeflag1 = (buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0x00 && buf[i + 3] == 0x01);
	int edgeflag2 = (buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0x01);
	if ((edgeflag1)) {
		return 4;  //1代表第一种格式,应该返回4个字节，注意一定时4而不是返回1
	}
	else if (edgeflag2) {
		return 3;  //2代表第二种格式，不要返回2
	}
	else {
		return 0;  //0代表不是这种格式，那么就不要
	}
}

void RTP::updateFrameList(FrameList* frameCatch, mediaService::RtpPacket* rtpPacket, int packetNums) {
	//上面这个注释是不对的，rtpPacket和startFrameIdx是没关系的，应该是frames
	// startFrameIdx怎么更新，不可能size为0，那就定为0，size不为0，就设置为begin的值，中间size也可能为0，可以判断startFrameIdx是否为0
	if (frameCatch->frames.size() == 0) {
		if (frameCatch->startFrameIdx == -1) {  //初始化为-1
			frameCatch->startFrameIdx = 1;
		}
		//如果size为0，但是startFrameIdx不为0，那么+1即可，因为这种情况代表所有的帧都发送完全了，startFrameIdx最终会指向队尾的，因为如果我删除的话，也会更新这个值
		//这个时候又来了一个帧，那就+1即可，不过如果清除old帧的时候，不更新startFrameIdx，那肯定有问题的，逻辑就不对，而且不可能从后往前清理，而且必须是一帧一帧的清理，
		//所以不用担心，但是操作的时候必须mutex
		else {
			frameCatch->startFrameIdx += 1;
		}
	}

	frameCatch->rtpPacket.push_back(rtpPacket);	//注意这里压入的是指针

	//注意这个frameIdx必须等到startFrameIdx更新完，在更新，因为刚开始外界得到的startFrameIdx可能为-1
	//当前帧是整个音视频的第几帧怎么获取，这个帧肯定是放在最后的，所以当前帧是startFrameIdx + frameNum（注意此时frameNum还没更新，因为startFrameIdx本身是从1开始的）
	int frameIdx = frameCatch->startFrameIdx + frameCatch->frameNum;
	FrameInfo* frame = new FrameInfo;
	frameCatch->frames[frameIdx] = frame;

	//这个应该放到frameIdx后面，比如，刚开始如果放在前面那么frameIdx会一开始就为2
	//如果frameCatch.rtpPacket.size() == 0，不为0，那就不需要更新startFrameIdx
	frameCatch->frameNum += 1;

	//更新audioFrame startFrameIdx更新完再更新（特别是刚开始）
	frame->packetNums = packetNums;		//音频帧的个数都为1
	//audioFrame->realAddr = audioCatch.rtpPacket.back(); 这里不应该是back，而应该是当前帧的rtpPacket中的第一个，视频帧肯定不能这样写，而且back返回的是值
	//audioFrame->realAddr = frameCatch.rtpPacket.rbegin();	//最后一个元素 //先这样写吧，后面再更新
															//注意这里使用rbegin是不行的，因为rbegin是反向迭代器，和正向迭代器类型不一致，我要保存的是正向迭代器
	//可以参考这个https://www.codenong.com/2678175/
	frame->realAddr = std::next(frameCatch->rtpPacket.end(), -1);	//这个是正向迭代器，这个网页还有其他方式
	frame->sendTimes = 0;
	frame->frameIdx = frameIdx;
}

void RTP::deleteOldPacket() {
	while (1) {
		{//注意这里的小括号很重要，保证休眠的时候释放锁
			std::lock_guard<std::mutex> lock(mutex);  //先加锁
			//获取当前房间有多少人
			int curUserNums = m_mediaRoom->userNums;
			deleteOldPacket(&videoCatch, curUserNums);
			deleteOldPacket(&audioCatch, curUserNums);
		}
		//10秒维护一次
		std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}

void RTP::deleteOldPacket(FrameList* frameList, int curUserNums) {
#if 1
	//这样删除是一个一个的删除，效率并不高，但是不得不这样删除
	//注意，可以遍历{ }组合的容器的。。。。。很棒的东西，但不支持里面是结构体
	Debug("deleteOldPacket正在删除");
	int startFrame = frameList->startFrameIdx;
	if (startFrame <= 0) {
		std::cout << "当前列表索引异常，无法删除" << std::endl;
		return;
	}
	auto& frames = frameList->frames;
	for (; startFrame < frameList->frameNum; startFrame++) {
		Debug("正在删除startFrame：%d", startFrame);
		auto& frame = frames[startFrame];
		if (frame->sendTimes == curUserNums) {
			//删除rtp包
			Debug("正在删除rtp包");
			auto rtpPacketIt = frame->realAddr;
			int rtpPacketNums = frame->packetNums; //当前rtp包包含几个
			//依次删除对应的rtp包
			for (int i = 0; i < rtpPacketNums; i++) {
				Debug("正在删除%d个包", i+1);
				//先保存当前it的副本，因为如果直接删除it，会导致当前迭代器失效，或者使用++it，不能使用++it，这样会导致删除的是后面那一个，并且也会迭代器失效
				auto curRtpPacketIt = rtpPacketIt;
				Debug("1");
				//curRtpPacketIt++;  //和i++的次数一样多
				//注意是rtpPacketIt++，否则第二次循环的时候删除的还是上一次循环的，就会段错误
				rtpPacketIt++;
				Debug("2");
				frameList->rtpPacket.erase(curRtpPacketIt);  //删除当前rtp包
				//应该释放资源，不要和上面的erase颠倒，看主要bug38
				//需要释放对应的mediaService::RtpPacket*
				delete* curRtpPacketIt;		//这一句导致必须一个一个删除
				Debug("3");
			
				Debug("4");
			}
			Debug("rtp包删除完成");
			//然后删除对应的视频帧索引
			frames.erase(startFrame);	//注意这里是删除的话，输入的是索引
			//释放frame，应该后删除这个
			delete frame;

			frameList->frameNum--;
			frameList->startFrameIdx += 1;	//别忘记更新startFrameIdx，很容易忘记
			Debug("删除帧后的起始帧为：%d", frameList->startFrameIdx);
		}
		else {
			//如果当前帧不相等了，那么后续的肯定不相等
			break;
		}
	}
	//出来的startFrame就是第一个发送不足的帧
	//注意这个startFrame一直是++的
	frameList->startFrameIdx = startFrame;

#else
	//当前方案 不可取
	//可以先找到最后一个发送完全的帧，然后范围的删除
	//不行，如果大范围删除，需要将rtp视频包和音频包分开，因为可能视频包发送完了，但是前面的音频包没有发送完全
	//好像即使分开了也不行，因为mediaService::RtpPacket*需要释放
	auto it = frameList.frames.begin();
	for (; it != frameList.frames.end();) {
		if (curIt->sendTimes == curUserNums) {
			it++;
		}
	}
	//出来之后it指向的是发送完全的包的下一个包，也就是it指向的是最新的没有发送完全的包
	//先获取最新的没有发送完全的包rtp包的地址
	if (it != frameList.frames.begin()) {  //如果还指向，那就没必要删除了
		auto rtpPacketIt = it->realAddr;

	}
#endif	
}

void RTP::run() {
	//开启一个线程定时的获取一个音频帧数据
	std::thread getVideoPacket(&RTP::getPacket, this, VIDEO, vThreadSleepMs);
	//开启一个线程，定时的获取一个视频帧数据
	std::thread getAudioPacket(&RTP::getPacket, this, AUDIO, aThreadSleepMs);
	//可以考虑开启一个删除包的线程，当满的时候就删除
	//重载函数不能直接传给线程，需要绑定器，并且绑定器也不支持绑定一个重载函数，需要显示的指出类型，绑定器删除了，直接使用线程
	//https://blog.csdn.net/xuanwolanxue/article/details/81162010
	std::thread deleteFrame((void(RTP::*)()) & RTP::deleteOldPacket, this);
	deleteFrame.detach();
	getVideoPacket.detach();
	getAudioPacket.detach();
}

void RTP::rtpHeaderInit(mediaService::RtpHeader* rtpHeader, uint32_t csrcLen, uint32_t extension, uint32_t padding, uint32_t version,
	uint32_t payloadType, uint32_t marker, uint32_t seq, uint32_t timestamp, uint32_t ssrc) {
	//注意看设计思路，可以发现RtpHeader这个结构体的4个32位的顺序和协议的顺序一致即可，即前2位是版本号等等，
	//如果不对padding等的高位清0操作，可能会导致赋值比如extension的时候影响到前面已经赋值的，比如padding，所以可以padding << 29;改为(padding & 0x0001) << 29，
	// 注意0x0001只够了，不需要0x00000001，可以截断呀
	// 第一个int32
	//高31-24
	rtpHeader->set_base(0);	//先全部清空
	// << 的优先级比 |的高
	rtpHeader->set_base(rtpHeader->base() | version << 30) ;	//高2位(31-30)，version：版本号， version << 30是将version << 30先移动到高两位上，然后赋值
	//RtpHeader->base	|= padding << 29;	//高29位，padding: 填充标志， padding << 29是将padding << 29先移动到29上，然后赋值
	//改为下面，后面同理
	rtpHeader->set_base(rtpHeader->base() | (padding & 0x0001) << 29);	//高29位，padding: 填充标志， padding << 29是将padding << 29先移动到29上，然后赋值
	rtpHeader->set_base(rtpHeader->base() | (extension & 0x0001) << 28);	//高28位，extension : 扩展标志
	rtpHeader->set_base(rtpHeader->base() | (csrcLen & 0x000F) << 24);	//高27-24位，csrcLen：CSRC计数器的长度，占4位

	//高23-16
	rtpHeader->set_base(rtpHeader->base() | (payloadType & 0x007F) << 17);	//高23-17位，payloadType：有效载荷类型，占7位
	rtpHeader->set_base(rtpHeader->base() | (marker & 0x0001) << 16);	//高16位，marker：标记，占1位

	//高16-0
	rtpHeader->set_base(rtpHeader->base() | (seq & 0xFFFF));			//高16位-0位，seq：用于标识发送者所发送的RTP报文的序列号，每发送一个报文，序列号增1，占16位

	//第二个int32
	rtpHeader->set_timestamp(timestamp);
	//第三个int32
	rtpHeader->set_ssrc(ssrc);
}