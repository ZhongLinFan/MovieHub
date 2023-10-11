#pragma once
#include "mediaService.pb.h"  //里面包含了要操作的结构体定义
#include <list>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <netinet/in.h> //sockaddr_in
#include "ffmpeg.h"		//必须要包含这个包，要不然很多结构体找不到
#include "Log.h"

#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC 97
#define RTP_VESION  2

enum FrameType {
	VIDEO = 1,
	AUDIO = 2
};

//存储了一帧的相关信息
struct FrameInfo {
	std::list<mediaService::RtpPacket*>::iterator realAddr; //当前帧在rtpPacket的实际位置
	int sendTimes; //当前包发送了多少次
	int packetNums; //当前帧包含多少个rtp包
	int frameIdx;		//代表当前帧是整个视频的第几帧，这4个参数非常重要
};

//后面发现视频和音频需要分开，然后发现都是双份的定义，所以这里直接定义一个结构体，就比较轻松了
struct FrameList {
public:
	FrameList(FrameType frameType) {
		startFrameIdx = -1;
		frameNum = 0;
		frameModid = frameModid;
	}
	//注意，这两个函数的主要困难点是怎么求得duration，至于如何求，需要哪些参数，下面其实解释了，这里列举之前看的一些博客（下面列举的不再列举）
	//https://blog.csdn.net/liguan1102/article/details/96323986 https://blog.csdn.net/weixin_43796767/article/details/116671238  https://blog.csdn.net/supermanwg/article/details/52798445
	//https://blog.csdn.net/yangguoyu8023/article/details/107545229
	//以及怎么获取这些参数（profile sampleRate fps）
	//https://blog.csdn.net/weixin_44517656/article/details/110355462	https://blog.csdn.net/Griza_J/article/details/126118779  https://blog.csdn.net/weixin_44517656/article/details/109645489
	//上面的已经提到profile sampleRate了，至此，所有的参数都可以找到了
	FrameList(FrameType frameType, int clockRate, int profile, int sampleRate):FrameList(frameType) {
		//时钟频率clockRate 一般为90khz，也就是1秒分成90000份，然后，nbSamples是音频帧的的样本点数目，一帧一般是1024（aac），然后是采样率，也就是1秒采样多少个音频帧，aac一般是44100Hz，也就是1秒采样44100个样本
		//那么多久发送一帧音频帧呢？nbSamples/sampleRate * clockRate，因为一秒为44100个样本，但是一帧包含1024个样本，所以需要nbSamples/sampleRate，这表示这一个帧的一秒的比例，我一秒分成90000，所以*90000
		// 但是demuxer那里是以宏的形式定义的nbSamples，比如profile里存的01 就是 AAC-LC 代表1024https://cloud.tencent.com/developer/article/1952656 这篇文章很重要
		//https://blog.csdn.net/hejinjing_tom_com/article/details/125586093 https://blog.csdn.net/kingzhou_/article/details/125976480
		int nbSamples = getNbSamples(profile);
		duration = (float)(nbSamples / sampleRate) * clockRate;	//注意前面的为小数，如果不取float，那么会为0，发现还是0，那就改成下面的吧
		duration = clockRate * nbSamples / sampleRate;
		Debug("nbSamples：%d，sampleRate：%d，clockRate：%d，音频duration：%d", nbSamples, sampleRate, clockRate, duration);
	}

	FrameList(FrameType frameType, int clockRate, int fps):FrameList(frameType) {
		duration = clockRate / fps;	//视频的就很好办了，
		Debug("fps：%d，clockRate：%d，视频duration：%d", fps, clockRate,  duration);
	}
	//获取nbSamples的函数，后续可以扩充	不能定义在结构体外面，会重定义
	static int getNbSamples(int profile) {	//这个还应该声明为静态函数，其他地方也要用到，而且还不操作成员变量，设置为静态很合适
		if (profile == FF_PROFILE_AAC_LOW) return 1024;
		return 1024;
	}
public:
	std::list<mediaService::RtpPacket*> rtpPacket;
	//std::list<FrameInfo*> frames;  //这个也改成指针吧，有了idx2Addr，好像不需要这个frames了吧，因为idx2Addr也能遍历呀，不过删除就比较麻烦了olog2，先删除试试
	//这个map主要是为了避免遍历frameInfoList的，所以设计了unordered_map，后面把frameInfoList拆分了
	std::unordered_map<int, FrameInfo*> frames;	//int是第几帧
	//两个线程的休眠时间不同，需要获取视频和音频的频率，所以这里保存，避免反复获取
	int duration;	//视频帧或者音频帧的时间间隔
	int frameNum; //当前缓存了多少帧
	//当前缓存的起始帧，之前的帧都没法播放了
	int startFrameIdx; //因为将std::list<FrameInfo*>换成了std::unordered_map<int, FrameInfo*>，所以需要有一个起始值，因为unordered_map不排序
	//int wholeSendedIdx;	//当前list全部发送的地址索引，这个不能有，因为人数是动态的，现在可能全部发完，但是之后可能就又进来人了
	//需要保存一个video的rtpHeader和audio的rtpHeader，这个只需要初始化的时候传进来相关参数即可
	mediaService::RtpHeader rtpHeader;
	FrameType frameModid;		//这个代表是视频帧还是音频帧，视频帧是1，音频帧是2，
};


//支持rtsp协议的类，不过目前打算一开始不使用tcp通信，而是使用udp通信，然后建立连接之后，直接使用当前的udp作为发包工具
//不过字段的定义应该是在protobuf那里，也就是新增一个mediaServer的protobuf协议文件
//这里只负责处理那个包，而不负责定义
class MediaServer; //保存了指针，需要前置声明
class MediaRoom;//保存了指针，需要前置声明
class RTP {
public:
	//RTP报文首部初始化
	RTP(MediaServer* mediaServer, MediaRoom* mediaRoom, int videoStreamId, int audioStreamId, int clockRate, int fps, int profle, int sampleRate);

	//发送RTP包的函数，辅助下面的发送一帧的函数，这个需要mediaServer的发送函数。。。。
	void sendRtpPacket(mediaService::RtpPacket* rtpPacket, sockaddr_in& toAddr);

	//发送一帧的数据（一帧可能含有多个rtp包）FrameInfo包含了一帧的数据
	//注意，返回发送的帧idx，因为可能传进来的和发送的idx不一样
	//上层必须提供发送的是音频帧还是视频帧，因为frameIdx音频和视频是重复的
	//由于分开了音频和视频，所以上层在调用的时候需要指定出帧的类型
	int sendFrame(enum FrameType frameType, int frameIdx, sockaddr_in& toAddr);

	//负责辅助sendFrame的，上层只负责提供frameIdx，而这个sendMsg负责将idx转换为真实的地址，也就是遍历list
	//这个遍历不太好吧。。。。因为FrameInfo中的迭代器本质上就是为了不让去遍历的，不对，FrameInfo中的迭代器是为了避免遍历rtpPacket这个list的
	//但是我依然不想遍历frameInfoList，可以使用unordered_map来完成
	FrameInfo* getFrameInfo(enum FrameType frameType, int frameIdx);
	
#if 0 
	//定时的获取一帧音频帧数据，并且打包成rtp包，单独开一个线程
	//这个需要访问demuxer
	//将getAudioPacket和getVideoPacket合成一个，不过写到最后的时候，上面有强制获取1帧的需求，而这个函数是需要一直执行的。。。那就再定义一个函数来获取一个帧
	void getAudioPacket() {
		//获取当前音频采样率，然后设置休眠时间
		AvPacket* packet = mediaRoom->demuxer->getFrame(mediaRoom->demuxer->audioFrame);
		//将当前帧封装成rtp包
		avPacket2Rtp(packet);
		//注意释放packet，这样就可以保证avdemuxer不存在内存泄漏
		av_packet_unref(packet);
		
		//别忘记维护frameIdx2Addr
		std::this_thread::sleep_for(std::chrono::milliseconds(audioCatch.duration-5));
	}

	//定时的获取一个视频帧数据
	void getVideoPacket() {
		//获取当前fps，然后设置休眠时间
		AvPacket* packet = mediaRoom->demuxer->getFrame(mediaRoom->demuxer->videoFrame);
		//将当前帧封装成rtp包
		avPacket2Rtp(packet);
		//注意释放packet
		av_packet_unref(packet);
		std::this_thread::sleep_for(std::chrono::milliseconds(videoCatch.duration-10));
	}
#endif

	//将上面的两个函数合成了一个
	//定时的获取一个视频帧数据
	void getPacket(enum FrameType frameType, int sleep_ms);

	//配合上面的packet获取帧,这里强制获取一帧，肯定能等到结果，如果这个也返回false，那么就说明当前播放完了
	void getOnePacket(enum FrameType frameType);

	//将一个avapcket，打包成rtp包，因为不可能每个用户都打包一下，所有用户打包一个就够了，缓存下来
	//这个函数应该也单独开启一个线程，目前来说不开启线程也行，没必要自找麻烦TODO
	void avPacket2Rtp(AVPacket* frame);

	//辅助avPacket2Rtp，判断NALU边界函数，顺便返回其startCode类型
	//必须要使用uint8_t，不能使用char*，否则会出错
	int edgeFlag(const uint8_t* buf, int i);

	//辅助avPacket2Rtp，更新frameInfo和frameList
	void updateFrameList(FrameList* frameCatch, mediaService::RtpPacket* rtpPacket, int packetNums);
	
	//删除已经全部发送完全的帧，或者是长期存在的帧，可以考虑遍历frameInfo,找到最后一个发送次数和房间中人数一样的，前面的帧都可以删除
	//这个函数应该是上层调用的，也就是room每次发送完一帧给所有的用户，都会调用，但是上层当前应该传递有多少用户，或者也不用，因为毕竟已经保存了mediaRoom
	void deleteOldPacket();

	//删除某一模块的packet，这个可以上层调用，也可以供上面的deleteOldPacket调用，上面的deleteOldPacket()可以是定时删除，或者满了再删除
	//注意std::unordered_map<int, FrameInfo*>体现不出来优势在这里，getFrameInfo才可以体现出来
	void deleteOldPacket(FrameList* frameList, int curUserNums);

	//启动rtp，定时发送一帧数据到对端
	void run();
	//负责辅助初始化rtpHeader的
	//注意这个和之前的不一样，需要设置为uint32_t，其他博客都是uint8_t，因为我如果想要给32位的高2位赋值，必须要通过与操作，那么按位与的话，必须要保证csrcLen等也是32位的
	void rtpHeaderInit(mediaService::RtpHeader* rtpHeader, uint32_t csrcLen, uint32_t extension, uint32_t padding, uint32_t version,
		uint32_t payloadType, uint32_t marker, uint32_t seq, uint32_t timestamp, uint32_t ssrc);

	//打印内存结构的函数
	void printRtp(FrameList* framelist) {
		std::cout << "*************************" << std::endl;
		if (framelist->frameModid == VIDEO) {
			std::cout << "帧列类型：" << "视频" << "\t";
			std::cout << "获取视频帧次数：" << tempVideoGetTimes << "\t";
		}
		else {
			std::cout << "帧列类型：" << "音频" << "\t";
			std::cout << "获取音频帧次数：" << tempAudioGetTimes << "\t";
		}
		std::cout << "当前有几帧缓存：" << framelist->frameNum << "("  << framelist->frames.size()  << ")" << "\t当前缓存的起始帧：" << framelist->startFrameIdx << "\t当前帧间间隔：" << framelist->duration << std::endl;
		std::cout << "当前FrameList中rtpHeader信息：" << "\t暂时忽略" << std::endl;
		std::cout << "当前帧列信息：" << std::endl;
		for (auto it = framelist->frames.begin(); it != framelist->frames.end(); it++) {
			std::cout << "\t" << it->first << "帧：" << "发送次数：" << it->second->sendTimes << "\t含有"
				<< it->second->packetNums << "个rtp包\t是整个视频的第" << it->second->frameIdx << "帧" << std::endl;
			int j = 1;
			auto startIt = it->second->realAddr;
			for (; j <= it->second->packetNums; j++, startIt++) {
				std::cout << "\t\t当前rtpPacket中rtpHeader信息：" << "\t暂时忽略" << std::endl;
				//注意这里的(*startIt)->payload()[0]
				std::cout << "\t\t\t" << j << "包的第一字节：";
				printf("%x", (*startIt)->payload()[0]);
				std::cout << " " << "第二字节：";
				printf("%x", (*startIt)->payload()[1]);
				std::cout << " " << "第三字节：";
				printf("%x", (*startIt)->payload()[2]);
				std::cout << " " << "第四字节：";
				printf("%x", (*startIt)->payload()[3]);
				std::cout  << "\t大小：" << (*startIt)->payload().size() << std::endl;
			}
		}
	}
public:
	//注意这里视频帧和音频帧的rtp是混合的，但是没有影响，不过索引倒是需要区分开来
	//std::list<mediaService::RtpPacket*> rtpPacket;  //待发送的rtp包缓存，但是需要标识一帧有几个吧，要不热怎么发，那么多用户，需要有一个索引
	//如果想要大范围删除，必须要将rtp的音频和视频帧分离开，所以把他放到了FrameList结构体中
	//然而即使放进去了，也是不行，不过都放进去了，那就不放回来了

	//std::list<int> frameSize;  //记录每个frame对应的rtp个数
	//上层的h264Frame缓存不需要考虑用户的逻辑，它只需要定时的产生h264文件即可，至于用户的播放不一致问题，应该是这里的rtp考虑
	//不对，如果按照封装的思想，应该是房间负责这个用户间播放不一致的同步问题，当前用户只负责维护rtp缓存队列即可
	//但是room应该告诉这里该清除多少个帧的数据，然后rtp负责删除相应的数据，但是上层room怎么知道删除多少个呢,肯定需要记录各个用户的当前已发送帧
	//的绝对位置，不能是frameSize的下标之类的，因为会动态删除list头部的消息，不可能删除完还要更新room的索引大小，所以必须使用绝对地址
	//上层room中，每个用户可以保存一个迭代器，指向frameSize的具体位置，指向的位置为即将要发送的位置，怎么清除都发送完的数据呢
	//std::list<std::pair<int, int>> frameSize; //第一个int代表当前list对应序号的rtpPacket有几个rtp包，第二个int代表这个帧发了多少次
	//那就每个帧对应一个pair，如果当前帧发送的数量和房间人数一致，就删除，如果小于就说明还没发送完全，不能删除，如果只有几个人没有看这帧数据
	//，并且frameSize的个数已经很多了，那就有必要删除前面的数据了，这个时候又出现问题，没有看的user的迭代器失效了怎么办，
	//好像并不支持判断一个迭代器是否有效（https://bbs.csdn.net/topics/390122112），那该咋办，而且发现从frameSize找rtpPacket的时候好麻烦
	//每发送一帧都要遍历一遍rtpPacket，所以我决定将pair的第二个改为迭代器，那次数怎么办呢，以及如果存储迭代器，失效问题怎么解决
	//std::list<std::pair<std::list<mediaService::RTPPacket*>::iterator, int>> frameSize; //第一个为当前帧的第一个rtp包的迭代器，第二个int为当前帧的rtp包个数
	//上面的问题怎么解决，各个用户肯定需要保存一个东西，而且不能强耦合，
	//想到了一个好办法，可以记录这个视频帧是当前的第几帧，然后各个用户记录播放到哪一帧，这样就可以放心的删除了，如果某个用户要发送的帧
	//小于当前缓存最小帧序号，那就跳着发，很好的设计
	//上面的设计还不行，因为需要定时区分音频帧和视频帧，上层房间让发送音频帧，你怎么找。。。。。
	//std::list<std::pair<std::list<mediaService::RTPPacket*>::iterator, int>> frameIdx; //这个第一个代表上面缓存的帧的起始位置，第二个代表当前视频的第几帧
	//不行，发现还需要设计一个int表示当前帧发了多少次，显然不行，上面定义一个结构体吧
	FrameList videoCatch;
	FrameList audioCatch;
	std::mutex mutex;
	//因为这里需要调用上层的udpSendMsg（在mediaServer）和getFrame（在room那里），所以需要知道相关消息，或者每个房间有一个streamServer呢？然后mediaServer的的udpServer
	//负责处理rtsp和lb服务器的负载情况？这样的话，这里只需要保存一个room指针就可以了
	//算了，目前就只使用MediaServer那个streamServer吧，所以这里需要保存两个指针
	MediaServer* m_mediaServer;
	MediaRoom* m_mediaRoom;

	//这边在获得帧之后，需要判断当前帧类型
	//存储音频流和视频流的索引
	int m_videoStreamId;
	int m_audioStreamId;
	//临时变量，为了判断获取次数和framlist的内存结构是否一致
	int tempAudioGetTimes = 0;
	int tempVideoGetTimes = 0;
	int vThreadSleepMs;
	int aThreadSleepMs;
};