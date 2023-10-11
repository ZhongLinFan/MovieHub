#pragma once
#include "Buffer.h"
#include "EventLoop.h"
#include "Channel.h"
#include <functional>
#include "Log.h"
#include "Message.h"  //相互包含，我就直接写成class了
//需要提供对外的Mseeage接口，不过加reactor的头文件的话就不需要了
#include <queue>
#include <thread>  //c++11的锁
#include <condition_variable> //c++11的条件变量

//设计思想参考qtClientbug的18，很长的bug。。。。
struct sendMsgInfo {
public:
	sockaddr_in addr;
	int msgLen;
	int flag; //预留，比如标记是否发送成功等，或者优先级，是否保留当前地址等等
};

//TcpClinet是单线程模型，也就是没有线程池，不过设计成了非阻塞模式，也就是connnect不会阻塞，read和write也不会阻塞，也就是有多线程的能力
//UdpClinet也是单线程模型，没有线程池，也设计成非阻塞模式，也就是readfrom和writeto不会阻塞
//注意客户端（不管是tcp还是udp）没必要弄一个线程池，然后每个线程都有一个evloop，因为客户端和服务器连接顶多也就10个fd，这个放到一个evloop上完全足够用了
//客户端完全可以支持多线程，不过其他线程可以干其他的事情
class UdpMsgRouter;
//class Message; //相互包含，这里就直接class了，将上面的include去掉了，后面因为udp需要对外提供接口，这里直接用#
//如果要使用类里面的成员变量需要UdpMsgRouter::msgCallBack，需要加上头文件
class Udp {
public:
	Udp();
	~Udp();
	//读事件触发的回调函数
	int processRead();
	//写事件触发的回调函数
	int processWrite();

	//销毁函数触发的回调
	int destroy();

	//主动发包机制
	int sendMsg(sockaddr_in& addr, bool copyResponse = true);

	//设置hook函数
	using hookCallBack = std::function<int(Udp* udp, void* userData)>;
	inline bool setConnStart(hookCallBack callBack, void* args = nullptr) {
		connOnStart = callBack;
		connOnStartArgs = args;
	}
	inline bool setConnLost(hookCallBack callBack, void* args = nullptr) {
		connOnLost = callBack;
		connOnLostArgs = args;
	}
	//设置路由
	using msgCallBack = std::function<void(Udp* host, void* userData)>;
	//bool setMsgRouter(int msgid, hookCallBack callBack, void* userData);如果使用UdpMsgRouter::msgCallBack就不行，显示UdpMsgRouter::msgCallBack未定义，即使都加了前置声明也是这样，不过这是编译Tcpserver.cpp的时候出现的
	//bool addMsgRouter(int msgid, hookCallBack callBack, void* userData);但是使用hook的，注册的时候显示不匹配
	bool addMsgRouter(int msgid, msgCallBack callBack, void* userData=nullptr);

	//对外封装启动函数
	void run();
public:
	//服务器或者客户端的ip和地址
	//在cpp和对应的bug记录表里也写了，这里需要改成队列
	sockaddr_in m_recvAddr;
	socklen_t m_recvaddrLen;
	//最好用个互斥量限制一下
	std::queue<sendMsgInfo> m_sendAddrQ;
	socklen_t m_sendAddrLen;  //其实和上面长度合并也未尝不可，因为ipv4的地址一样长，如果以后支持ipv6了，那么这里肯定要改成队列
	//请求和响应
	Message* m_request;
	Message* m_response;
	//读和写buf(客户端不需要内存池，所以都需要在各自的子类初始化)
	ReadBuffer* m_rbuffer;
	WriteBuffer* m_wbuffer;
	//消息路由机制
	UdpMsgRouter* m_msgRouter;
	//事件循环机制
	EventLoop* m_evLoop;  //最终只有一个fd被监听
	//fd对应的写事件和读事件
	Channel* m_channel;

	//hook
	hookCallBack connOnStart;
	hookCallBack connOnLost;
	void* connOnStartArgs;
	void* connOnLostArgs;

	//保证地址消息和缓冲的消息是一致的，可以提供读写分离的，因为增加和删除不冲突，队列支持两端同时删除和插入，buf也是支持的
	//std::mutex m_msgInsertMutex;
	//std::mutex m_msgRemoveMutex;
	//上面不对，可以参考qtClientbug的18，很长的bug。。。。
	std::mutex m_sendMutex;
	std::condition_variable m_sendCond;
};

class UdpServer:public Udp {
public:
	UdpServer(const char* ip, unsigned short port);
	~UdpServer();

public:
};

class UdpClient :public Udp{
public:
	//注意客户端没必要指定自己的ip和端口，这个发到对端，对端可以直接获取，客户端也用不到自己的ip和端口
	UdpClient(const char* ip, unsigned short port);
	~UdpClient();
public:
	
};