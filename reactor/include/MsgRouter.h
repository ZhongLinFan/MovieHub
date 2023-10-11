#pragma once
#include <functional>
#include <map>
#include "Log.h"
//#include "NetConnection.h"  //这个应该放到cpp文件，这里没用到
class NetConnection;  //但是需要提前声明一下
class TcpMsgRouter {
public:
	//支持的回调函数类型
	//因为hook和消息路由机制使用的是同一个类型的函数，所以可以把这个放到NetConnection那里，并且在这里给他取个别名
	//typedef NetConnection::hookcallBack msgCallBack;  这样行不通，还是分别定义两个把
	using msgCallBack = std::function<void(NetConnection* conn, void* userData)>; //NetConnection* conn这个是需要处理的数据的那个tcpConnection，
	//而这个绑定的this根本没用，甚至是一个完全不相干的类里存放的大量的业务函数，甚至是一个普通函数，所以必须在call的时候传进来当前正在处理的tcpConnection给到第一个形参
	//在TcpServer的main函数外部完全可以定义一个fun1(NetConnection* conn, void* userData));,然后针对这个conn的request进行处理
	TcpMsgRouter();
	~TcpMsgRouter();
	//注册一个回调业务函数
	bool submit(int msgid, msgCallBack callBack, void* userData);  //注意msgCallBack callBack虽然没有*，但是他是个指针类型，所以外面一定要传进来地址
	//调用一个回调业务函数
	//这里其实是负责对解析好的消息（在request里）通过业务处理封装响应消息，并将响应消息写到写缓冲区里（sendMsg函数，被封装到这里来了）
	//所以这里的参数只需要一个netConnection对象
	void call(NetConnection* conn);  //回调函数的逻辑处理可能会用到userData，注意sendMsg是在回调函数处理完之后才执行的
	//（因为一般来说客户端和服务器是有来有回的，把sendMsg放到业务函数里，是把发送函数的权力交给上层）

public:
	std::map<int, msgCallBack> m_router;
	std::map<int, void*> m_args;
};

class Udp;
class UdpMsgRouter {
public:
	using msgCallBack = std::function<void(Udp* host, void* userData)>;
	UdpMsgRouter();
	~UdpMsgRouter();
	//注册一个回调业务函数
	bool submit(int msgid, msgCallBack callBack, void* userData);
	void call(Udp* host);
public:
	std::map<int, msgCallBack> m_router;
	std::map<int, void*> m_args;
};