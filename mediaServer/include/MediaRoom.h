#pragma once
#include "Avdemuxer.h"  //解封装器
#include "rtp.h"
#include "rtsp.h"
#include <list>
#include <string>
#include <netinet/in.h> //sockaddr_in

enum PlayState {
	PLAY = 1,
	STOP = 2
};

//使用User，在创建结构体的时候会出错，不知道为啥
//对成员‘uid’的请求出现在‘userTemp’中，而后者具有非类类型‘User()’userTemp.uid = 1
struct OnlineUser {
public:
	OnlineUser() {};
public:
	int uid;
	int state;  //播放状态
	//int curFrameIdx; //当前用户播到哪一帧了,注意这里代表当前视频的第几帧
	//不行，必须设置音频帧的idx，和视频帧的idx
	int curAudioIdx;
	int curVideoIdx;
	sockaddr_in addr; //存储当前用户的客户端地址
};


class MediaServer;
class MediaRoom {
public:
	MediaRoom(MediaServer* mediaServer, std::string path);
	//定时发送rtp包的函数，比如30秒发送一帧图像，23秒发送一帧音频，这里是一帧为单位，
	//当前房间定时发包的逻辑（判断每个用户的状态，决定是否发包）注意这这里是当前房间所有用户都发的
	void sendFrame(enum FrameType frameType, int sleep_ms);
	//增加删除用户
	void addUser();
	void removeUser();
	//启动房间
	void run();
public:
	std::list<int> roomId;  //当前电影的fid
	std::list<std::string> roomPath;	//当前电影的路径
	std::list<OnlineUser> users;	//当前房间的用户列表
	int userNums;	//当前房间人数
	//还应该有一个缓冲rtp包的函数，（这里应该有一个判断当前包是否包含完整帧的逻辑）
	Avdemuxer* demuxer;   //负责将mp4文件解封装，然后输出rtp包到当前房间的rtp发送缓冲队列中的线程，线程在run里创建
							//注意一个房间肯定只有一个解封装器，其他的并不需要
						//这里必须要使用*，不能使用demuxer，因为
	bool isRunning;		//当前房间的运行标志
	//每个房间有一个RTP对象，负责将avpacket的包打包成rtp包
	//由于RTP也有mutex，所以这里也必须使用指针,RTSP同理
	RTP* rtpPacketManager;	//负责管理当前房间的rtp缓存  //这个也是一个线程
	//每个房间应该有一个rtsp线程，负责和客户端通信，注意这个rtsp的通信理论上是需要tcp的，但是我这里选择了udp，后续可以改
	//不过这里没有用到线程，因为rtsp的属性在得到当前电影的时候就已经确定好了，所以执行相关函数的时候是直接发送的
	RTSP* rtspManager;	
};
