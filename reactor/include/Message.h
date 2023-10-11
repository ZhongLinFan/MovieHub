#pragma once
#include "Buffer.h" 
#include <functional>
//#include "MsgRouter.h"  //cpp文件用到了，不需要放在这  和netcation关联，如果取消会报错,这里用到了NetConnection指针，下面前置声明一下就行
//标记当前解析状态
enum class HandleState :int {
	MsgHead,  //如果在这个状态，说明读到8个字节
	MsgBody, //如果在这个状态
	WaitingHead,
	WaitingBody,
	BadMsg,
	Response,
	Done,
};

#define MSG_HEAD_LEN 8
#define MSG_LEN_LIMIT (65535 - MSG_HEAD_LEN)

//因为服务器端和客户端都是一样的消息结构，不像http那样，响应和解析结构不一致，才定义请求和响应两个结构体
class Udp;
class NetConnection;
class Message {
public:
	Message();
	~Message();
	//int analyze(Buffer* buf, Message* response);
	int analyze(NetConnection* conn); //因为在处理路由机制的时候需要在解析完成后调用使用TcpConnection作为参数传递给call，所以这里的参数改了

	//上面是tcp的，如果需要udp的那么就需要另一个函数
	int analyze(Udp* udp);
	HandleState getRequest(ReadBuffer* buf);
public:
	int m_msgid;
	int m_msglen;
	char* m_data;
	HandleState m_state;
};

