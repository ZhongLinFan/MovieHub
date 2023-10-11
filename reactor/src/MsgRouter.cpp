#include "MsgRouter.h"
#include "Udp.h"
#include "NetConnection.h"  //这里用到了方法，需要加
#include "Message.h"  //m_request->m_msgid;用到了方法，直接加上
TcpMsgRouter::TcpMsgRouter() :m_router(), m_args() {}

TcpMsgRouter::~TcpMsgRouter() {}

bool TcpMsgRouter::submit(int msgid, msgCallBack callBack, void* userData)
{
	if (m_router.find(msgid) != m_router.end()) {
		std::cout << "当前msgid已注册" << std::endl;
		return false;
	}
	Debug("正在添加业务函数，序号为：%d", msgid);
	m_router[msgid] = callBack;
	m_args[msgid] = userData;
	return true;
}

void TcpMsgRouter::call(NetConnection* conn)
{
	int msgid = conn->m_request->m_msgid;
	Debug("当前业务序号：%d", msgid);
	if (m_router.find(msgid) == m_router.end()) {
		std::cout << "当前msgid未注册，已按默认动作处理" << std::endl;
		//调用一个默认动作;
		return;
	}
	auto handleFunc = m_router[msgid];
	void* userData = m_args[msgid];
	Debug("userData的地址：%x", userData);
	handleFunc(conn, userData);
	Debug("调用业务回调函数成功");
	//这里并不发送数据，而是全部交给业务层
}

UdpMsgRouter::UdpMsgRouter() :m_router(), m_args()
{
}

UdpMsgRouter::~UdpMsgRouter()
{
}

bool UdpMsgRouter::submit(int msgid, msgCallBack callBack, void* userData)
{
	if (m_router.find(msgid) != m_router.end()) {
		std::cout << "当前msgid已注册" << std::endl;
		return false;
	}
	Debug("正在添加业务函数，序号为：%d", msgid);
	m_router[msgid] = callBack;
	m_args[msgid] = userData;
	return true;
}

void UdpMsgRouter::call(Udp* host)
{
	int msgid = host->m_request->m_msgid;
	Debug("当前业务序号：%d", msgid);
	if (m_router.find(msgid) == m_router.end()) {
		std::cout << "当前msgid未注册，已按默认动作处理" << std::endl;
		//调用一个默认动作;
		return;
	}
	auto handleFunc = m_router[msgid];
	void* userData = m_args[msgid];
	Debug("Udp的地址：%x", host);
	Debug("userData的地址：%x", userData);
	handleFunc(host, userData);
	Debug("调用业务回调函数成功");
}
