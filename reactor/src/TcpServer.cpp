#include "TcpServer.h"
//#include "Buffer.h" 没用到
#include "Channel.h"
#include <stdlib.h>
#include <functional>
#include <unistd.h> //usleep
#include <assert.h>
//#include "EventLoop.h" 头文件有了
#include "MsgRouter.h"  //用到了成员函数必须要有
#include "Dispatcher.h"  // 头文件的evloopdeh文件里只有Dispatcher的前项声明，而没有定义（不能定义，必须前向声明），这里需要用到，所以这里加上

//静态变量初始化
std::map<int, TcpConnection*> TcpServer::m_Fd_TcpConn; //静态变量初始化的方法https://code84.com/834017.html
TcpMsgRouter TcpServer::m_msgRouter;  //调用构造函数（无参构造）

NetConnection::hookCallBack TcpServer::connOnStart = nullptr;
NetConnection::hookCallBack TcpServer::connOnLost = nullptr;
void* TcpServer::connOnStartArgs = nullptr;
void* TcpServer::connOnLostArgs = nullptr;

TcpServer::TcpServer(unsigned short port, int threadNums)
{
	Debug("开始初始化服务器");
	m_port = port;
	listenerInit();
	m_mainLoop = new EventLoop(std::this_thread::get_id(), -1); //注意这个作用主要在evLoopAddTask上判断当前线程是否是主线程才用到，在那里有解释
	m_threadNums = threadNums;
	m_threadPool = new ThreadPool(m_mainLoop, threadNums);
	m_bufPool = BufferPool::getInstance();  //全局唯一，其他地方创建，也是全局唯一，注意，在初始化时必须先创建，
	//因为如果不创建，instance不知道指向什么位置了（可以设置为NULL，后续也会出问题），就会导致后续申请时出问题
	m_seq = -1;
	Debug("初始化服务器完成");
}

bool TcpServer::listenerInit()
{
	
	//创建监听的套接字
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_lfd == -1) {
		printf("socket\n");
		exit(0);
	}
	//设置端口复用
	int opt = 1;
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1) {
		printf("setsockopt\n");
		exit(0);
	}
	//绑定端口
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = INADDR_ANY; //本地的回环地址
	ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1) {
		printf("bind\n");
		exit(0);
	}
	//设置监听
	ret = listen(m_lfd, 128); // 这里已经处于监听状态
	if (ret == -1) {
		printf("listen\n");
		exit(0);
	}
	return true;
}

void TcpServer::run()
{
	Debug("开始启动服务器"); 
	m_threadPool->run();//注意 不能在evLoopRun下面启动线程池，因为evLoopRun是一直循环的下面的代码永远执行不了
	//注意，函数指针传递的时候非静态函数需要等待对象创建完成，并且使用绑定器才能传递给包装器使用，如果没有包装器，那么非静态成员函数将永远不能作为参数传递，
	//不过静态成员函数在类存在时就已经存在，可以直接传递给包装器或者作为普通形参(也就是不使用包装器作为形参)进行传递，但此时不能使用类内部非静态变量，如果想使用，可以传递过去
	//所以由于channel那边是包装器，那么这边如果想要传递过去就需要绑定器
	auto acceptTcp = std::bind(&TcpServer::acceptTcpConnection, this); // 注意这里的函数地址怎么写的
	Channel* lchannel = new Channel(m_lfd, FDEvent::ReadEvent, acceptTcp, nullptr,nullptr, nullptr);
	m_mainLoop->addTask(this->m_mainLoop, m_mainLoop->packageTask(lchannel, TaskType::ADD));
	Debug("启动服务器成功！！（mainLoop启动，lfd加入，线程池启动）");  //注意vs编写的编码默认为系统的gbk，而notepad++或者Linux系统默认utf-8，所以在linux打开会成为乱码
	Debug("sever基础信息：主线程ID：%d，mainLoop序号：%d,检测模型类型：%s，mainLoop自己的cfd：%d, 端口号：%d，lfd：%d", m_seq, m_mainLoop->m_seq,
		m_mainLoop->m_dispatcher->m_name.c_str(), m_mainLoop->m_pipefd[0], m_port, m_lfd);
	Debug("线程池信息：");
	for (int i = 0; i < m_threadNums; i++) {
		Debug("线程池ID：%d,绑定的事件循环ID：%d，事件循环自己的fd：%d", m_threadPool->m_pool[i]->m_seq, m_threadPool->m_pool[i]->m_evloop->m_seq, m_threadPool->m_pool[i]->m_evloop->m_pipefd[0]);
	}
	m_mainLoop->run();  //需要保证这一句代码是最后一句，因为这个函数里有一个while死循环
}

int TcpServer::acceptTcpConnection()
{
	//接收客户端
	Debug("正在接收客户端");
	struct sockaddr_in* client = new struct sockaddr_in;  //这个不需要在这里析构，而是TcpConnection结束时析构
	socklen_t tmpClientLen = sizeof(struct sockaddr_in);
	int cfd = accept(m_lfd, (struct sockaddr*)client, &tmpClientLen); //(struct sockaddr*)client不是(struct sockaddr*)&client！！！！
	if (cfd == -1) {
		delete client; //释放刚刚开辟的空间
		perror("accept");
		if (errno == EINTR) {
			return -1; //说明被中断信号打断了，进入while重新尝试建立连接
		}
		else if (errno == EMFILE) {
			//说明建立的连接太多，资源不够，这个时候应该中止主线程，然后等待子线程唤醒TODO，先简单的休眠
			usleep(200);
			return -1;
		}
		else {
			//未知错误，需要退出处理
			exit(-1);
		}
	}
	//判断当前连接数是否大于最大限制
	if (onlines() >= m_clientLimit) {
		std::cout << "当前连接数量过多，已拒绝客户端["<< inet_ntoa(client->sin_addr) <<":"
			<< ntohs(client->sin_port) <<"]的连接" << std::endl;
		close(cfd);
		return -1;
	}
	Debug("正在接收客户端cfd：%d", cfd);
	//取出一个evloop,如果子线程数量为0，那么得到的就是mainLoop
	EventLoop* evLoop = m_threadPool->takeEvLoop();
	addFd_TcpConn(cfd, new TcpConnection(evLoop, cfd, client, &TcpServer::m_msgRouter));  //这里并不需要将tcpConnection的返回值赋给谁，因为在tcpConnectionInit里，已经把任务加到对应的树上了，
	//注意，TcpConnection全都是从这里new出来的，但是销毁就不一定了，所以删除需要在析构函数里进行处理
	//这个时候应该把map设置为静态的，或者参数传入TcpServer
	//当然这个addFd_TcpConn在TcpConnection构造函数中实现也可以
	//或者统一一点，所有的关于fd对应的集合更新关系都在EventLoop::addChannel这里进行
	//从树上删除的话自然要在对应的deleteChannel进行操作
	Debug("fd:%d对应的TcpConnection封装成功，当前在线人数为：%d", cfd, onlines());
	return true;
}

bool TcpServer::addFd_TcpConn(int fd, TcpConnection* tcp)
{
	if (m_Fd_TcpConn.find(fd) == m_Fd_TcpConn.end()) {
		m_Fd_TcpConn[fd] = tcp;
		return true;
	}
	std::cout << "当前fd已存在，请及时排查错误" << std::endl;
	return false;
}

bool TcpServer::delFd_TcpConn(int fd)
{
	assert(m_Fd_TcpConn.find(fd) != m_Fd_TcpConn.end());
	m_Fd_TcpConn.erase(fd);
	return true;
}

int TcpServer::onlines()
{
	return m_Fd_TcpConn.size();
}

bool TcpServer::addMsgRouter(int msgid, msgCallBack callBack, void* userData)
{
	m_msgRouter.submit(msgid, callBack, userData);
	return false;
}
