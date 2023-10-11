#pragma once
#include "NetConnection.h"
#include "Log.h"

class TcpConnection :public NetConnection {
public:
	//fd是为了给tcpConnect起名字，和初始化channel
	TcpConnection(struct EventLoop* evloop, int fd, struct sockaddr_in* client, TcpMsgRouter* msgRouter);
	~TcpConnection();
	bool destroy();  //回调函数的实体，通过该函数删除掉所有的对应关系和释放所有的资源,回调的话，要么静态，要么绑定器
public:
};