#pragma once
#include "ThreadPool.h"
#include "EventLoop.h"  //注意evloop只有dispatcher的声明没有定义，而cpp文件需要使用dispatch所以需要在cpp文件添加相应的头文件
#include <arpa/inet.h>
#include "Log.h"
#include "BufferPool.h"
#include "TcpConnection.h"  //用到了
#include <map>
#include "Message.h"  //这个这里也没用到呀，其他模块（比如数据库代理服务器）会用到这里面的东西，所以需要在这里加上
#include "NetConnection.h"
//#include "Task.h"  还有这个也没用到呀
#include <functional>

class TcpServer {
public:
	using msgCallBack = std::function<void(NetConnection* conn, void* userData)>;
	bool listenerInit();
	//初始化TCPServer
	TcpServer(unsigned short port, int threadNums);
	//启动TCPServer
	void run();
	int acceptTcpConnection();
	//针对Fd_EvLoop和Fd_TcpConn设置的一些接口
	// 由于删除肯定是一起删除，添加肯定是一起添加，所以他们公用一个接口就行
	// 为什么设置为静态，看cpp文件
	//添加
	static bool addFd_TcpConn(int fd, TcpConnection* tcp);
	//删除
	static bool delFd_TcpConn(int fd);
	//获取在线客户端数量
	static int onlines();

	//向外部暴露注册路由接口
	//注意由于普通成员函数可以访问所有成员变量，包括静态成员变量，所以这里没必要设置为静态的
	//using msgCallBack = std::function<void(NetConnection* conn, void* userData)>;  //这里也要指定一下类型，否则会说msgCallBack未定义,错误
	bool addMsgRouter(int msgid, msgCallBack callBack, void* userData = nullptr);  //msgCallBack* callBcak不能是msgCallBack callBcak，因为bool submit(int msgid, msgCallBack callBack, void* userData);第二个参数是一个地址

	//向外暴露注册hook接口
	//inline bool setConnStart(NetConnection::hookCallBack* startFunc, void* args = nullptr) 
	inline bool setConnStart(NetConnection::hookCallBack startFunc, void* args = nullptr) {  //hookCallBack本身就是个指针类型，不需要加指针
		connOnStart = startFunc;
		connOnStartArgs = args;
	}
	inline bool setConnLost(NetConnection::hookCallBack lostFunc, void* args = nullptr) {
		connOnLost = lostFunc;
		connOnLostArgs = args;
	}

	//提供一个给某一些客户端发送普通异步任务的函数
	//void sendNormalTask(taskCallBack taskFunc, vector<int>* group);

	//提供一个给指定客户端主动发送消息的入口函数

	//通过当前的fd获取对应的evloop
	inline EventLoop* getFdEvLoop(int fd) {
		if (m_Fd_TcpConn.find(fd) != m_Fd_TcpConn.end()) {
			return m_Fd_TcpConn[fd]->m_evloop;
		}
		else {
			return nullptr;
		}
	}

public:
	EventLoop* m_mainLoop;
	ThreadPool* m_threadPool;
	BufferPool* m_bufPool; //不加也行，反正是静态可以全局获取（加对应的头文件的话）
	//我没有想到要在这里记录线程个数
	int m_threadNums;
	unsigned short m_port;
	int m_lfd;
	int m_seq; //主线程编号
	//记录当前fd和对应TcpConnection的关系
	//维护关系时别忘记加锁
	static std::map<int, TcpConnection*> m_Fd_TcpConn;  //这里记录了系统中所有的fd对应的TcpConnection
	//通过上面的两个结构，可以精准的定位到每一个fd的精确信息,并且可以了解到每一个evLoop的连接人数
	//为什么要服务器主动发消息呢，因为后期可能比如一个客户端和另一个客户端通信，那么就需要服务器主动联系，主动找，不过当一个连接断开时，必须要清空，对应的集合信息
	//然后你不关闭，可是系统可能会关闭，然后客户端又会重新往这注册，就会导致加入失败
	//注意为啥不设置成这样，std::map<int, EventLoop* > EvLoop_Fd;,因为TcpConnection就有对应的EventLoop，这个客户的信息全都在这
	//std::map<EventLoop* ,int> EvLoop_Fd;设置成这样可以了解每个线程的在线情况
	//最终发现这样行不通，因为EventLoop里面有很多fd，而map的key唯一，如果想要了解当前线程的在线情况，可以在当前线程创建对应的集合
	//这样可以在TcpServer里进行查询了

	//服务器最大连接数量，当前的一个m_Fd_TcpConn的简单应用
	const int m_clientLimit = 100;

	//路由机制
	static TcpMsgRouter m_msgRouter;  //因为调用是在message里或者TcpConnection那里调用的，所以肯定需要静态

	//Hook机制
	//因为需要在其他地方调用，所以static
	static NetConnection::hookCallBack connOnStart;
	static NetConnection::hookCallBack connOnLost;
	static void* connOnStartArgs;
	static void* connOnLostArgs;
};
