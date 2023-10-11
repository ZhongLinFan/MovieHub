#include "TcpConnection.h"
//#include "Channel.h"  //这是子类，父类中都有的完全不需要
//#include "EventLoop.h"
#include <assert.h>
#include <errno.h>
#include <functional>
#include "TcpServer.h"
#include <algorithm>

TcpConnection::TcpConnection(struct EventLoop* evloop, int fd, struct sockaddr_in* client, TcpMsgRouter* msgRouter)
{
	m_rbuffer = new ReadBuffer();
	m_wbuffer = new WriteBuffer();
	auto readFunc = std::bind(&TcpConnection::processRead, this);
	auto writeFunc = std::bind(&TcpConnection::processWrite, this);
	auto destroyFunc = std::bind(&TcpConnection::destroy, this);
	m_channel = new Channel(fd, FDEvent::ReadEvent, readFunc, writeFunc, destroyFunc, nullptr);
	m_evloop = evloop;
	sprintf(name, "tcpConnection-%d", fd);
	m_sOrcAddr = client;
	//调用hook函数
	if (TcpServer::connOnStart != nullptr) {
		TcpServer::connOnStart(this, TcpServer::connOnStartArgs);
	}
	m_evloop->addTask(m_evloop, m_evloop->packageTask(m_channel, TaskType::ADD));
	//让每个客户端连接都知道路由函数的地址
	m_msgRouter = msgRouter;
	//更新当前线程的在线的fd
	m_evloop->m_onlineFd.push_back(fd);
}

TcpConnection::~TcpConnection()
{
	//释放创建时开辟的存放客户端地址的内存
	//一定要new的时候就想到在哪里释放，否则容易忘，这个前提条件就是整个系统的部件释放的时机要理解的很成熟
	//Hook函数
	if (TcpServer::connOnLost != nullptr) {
		TcpServer::connOnLost(this, TcpServer::connOnLostArgs);
	}
	delete m_sOrcAddr; // 虽然在父类但是还是需要析构
}

bool TcpConnection::destroy()
{
	//删除对应关系，我感觉还是在evloop那个deleteChannel删除对应的关系比较好，因为那里可以获得的资源很多，这里如果想要获取对应的关系
	//要么把evloop传进来，要么把evloop对应的资源设置为静态的，要么就在deleteChannel删除对应的关系
	

	//最终在释放客户端资源的时候发现，服务器和客户端资源不一致，释放资源和解除关系都应该在这里
	m_evloop->m_channelMap.erase(m_channel->m_fd);
	std::remove(m_evloop->m_onlineFd.begin(), m_evloop->m_onlineFd.end(), m_channel->m_fd);  //不过要警惕这个remove的删除方法，因为他不是真正的删除
	//如果有其他结构体，可以用其他结构体

	//删除TCPServer的对应关系
	TcpServer::delFd_TcpConn(m_channel->m_fd);
	
	//释放资源
	delete this;
	Debug("客户端资源释放成功");
	return true;
}

