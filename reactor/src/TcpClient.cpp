#include "TcpClient.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

TcpClient::TcpClient()
{
	m_rbuffer = new ReadBuffer(2);
	m_wbuffer = new WriteBuffer(2);
	m_evloop = new EventLoop(std::this_thread::get_id(), -1);
	auto readFunc = std::bind(&TcpClient::processRead, this);
	auto writeFunc = std::bind(&TcpClient::connected, this);  //建立连接的写回调，建立完之后，需要更改
	auto destroyFunc = std::bind(&TcpClient::destroy, this);
	m_channel = new Channel(-1, FDEvent::Nothing, readFunc, writeFunc, destroyFunc, nullptr); // 因为还没分配套接字，所以默认是-1，等连接上之后再改
	//m_evloop->addTask(m_evloop, m_channel, TaskType::ADD); //这里如果不添加，那么连接过程都是修改，肯定不行，
	//但是如果errno == EINPROGRESS的添加，那么=0的情况是需要直接调用connected的，那里是修改，所以这里直接先添加把
	//但是尝试了一下，这里还没有fd会添加失败，所以必须在下面进行添加
	sprintf(name, "Client-%d", -1);

	//初始化服务器地址
	m_sOrcAddr = nullptr;

	//这样可以保证不管是服务器还是客户端都知道路由表的存在
	m_msgRouter = new TcpMsgRouter();

	//初始化Hook函数
	connOnStart = nullptr;
	connOnLost = nullptr;
	connOnLostArgs = nullptr;
	connOnStartArgs = nullptr;
	m_connected = false;
	Debug("TcpClient初始化完成");
}

TcpClient::TcpClient(const char* ip, short port):TcpClient()  //委托构造函数，c++11新特性
{
	//初始化服务器地址
	m_sOrcAddr = new struct sockaddr_in;
	setSockAddr(m_sOrcAddr, ip, port);
}

bool TcpClient::connectServer()
{
	Debug("开始连接服务器");
	if (m_channel->m_fd != -1) {
		close(m_channel->m_fd); //说明原来已经在使用了，那么可以直接关掉，不用删掉资源，而是直接用原来的资源
	}
	//创建套接字
	m_channel->m_fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK, IPPROTO_TCP);
	if (m_channel->m_fd == -1) {
		perror("socket");
	}
	int ret = connect(m_channel->m_fd, (struct sockaddr *)m_sOrcAddr, m_addrLen);
	if (ret == -1) {
		//如果是这个值，这个是非阻塞客户端模式下的通用性问题，需要再次判断，是否可写
		if (errno == EINPROGRESS) {
			//添加写事件
			Debug("正在连接。。。EINPROGRESS");
			m_channel->writeEnable(true);
			Debug("添加写事件任务");
			m_evloop->addTask(m_evloop, m_evloop->packageTask(m_channel, TaskType::ADD));
		}
		else {
			Debug("连接失败");
			perror("connect");
			exit(0);
		}
	}
	else {
		//关闭写事件
		m_channel->writeEnable(false);
		m_evloop->addTask(m_evloop, m_evloop->packageTask(m_channel, TaskType::ADD)); //注意这里必须是添加任务，因为是第一次
		//更改写回调函数
		auto writeFunc = std::bind(&TcpClient::processWrite, this);  //建立连接的写回调，建立完之后，需要更改
		m_channel->changeWriteCallBack(writeFunc, nullptr);
		
	}
	return true;
}

bool TcpClient::connected()
{
	//说明建立连接成功，关闭写事件，添加读事件检测吗，更改写回调
	m_channel->writeEnable(false);
	m_channel->readEnable(true);
	m_evloop->addTask(m_evloop, m_evloop->packageTask(m_channel, TaskType::MODI));
	//更改写回调函数
	auto writeFunc = std::bind(&TcpClient::processWrite, this);  //建立连接的写回调，建立完之后，需要更改
	m_channel->changeWriteCallBack(writeFunc, nullptr);
	//再次验证是否真正建立（保底）
	Debug("已连接到%s:%d", inet_ntoa(m_sOrcAddr->sin_addr), ntohs(m_sOrcAddr->sin_port));
	m_connected = true;
	//hook函数执行
	if (connOnStart != nullptr) {
		connOnStart(this, connOnStartArgs);
	}
	return true;
}

bool TcpClient::addMsgRouter(int msgid, TcpMsgRouter::msgCallBack callBack, void* userData) {  //msgCallBack* callBcak不能是msgCallBack callBcak，因为bool submit(int msgid, msgCallBack callBack, void* userData);第二个参数是一个地址
	m_msgRouter->submit(msgid, callBack, userData);
}

bool TcpClient::destroy()
{
	//执行hook函数
	if (connOnLost != nullptr) {
		connOnLost(this, connOnLostArgs);
	}
	//可能一个客户端不止一个链接
	//删除对应关系
	this->m_evloop->m_channelMap.erase(m_channel->m_fd);
	delete this;
	exit(1);
}

void TcpClient::run()
{
	m_evloop->run();
}

void TcpClient::changeServer(const char* ip, short port)
{
	if (m_sOrcAddr == nullptr) {
		m_sOrcAddr = new struct sockaddr_in;
	}
	setSockAddr(m_sOrcAddr, ip, port);
}

void TcpClient::setSockAddr(sockaddr_in* addr, const char* ip, short port)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
	m_addrLen = sizeof(*m_sOrcAddr);  //一定要注意，如果是指针，这里需要是对象
}
