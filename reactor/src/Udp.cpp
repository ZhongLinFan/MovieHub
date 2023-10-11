#include <functional>
#include "Udp.h"
#include <string.h>
#include "Log.h"
#include "MsgRouter.h"
#include "Message.h" //下面开辟空间了所以需要include

Udp::Udp()
{
	bzero(&m_recvAddr, sizeof(m_recvAddr));
	m_recvaddrLen = sizeof(m_recvAddr);
	m_sendAddrLen = sizeof(m_recvAddr);
	//m_sendAddrQ.clear();队列没有清空操作
	m_rbuffer = nullptr;  //在自己的子类初始化
	m_wbuffer = nullptr;
	m_request = new Message();
	m_response = new Message();
	m_msgRouter = new UdpMsgRouter();
	m_evLoop = new EventLoop(std::this_thread::get_id(), -1);
	connOnStart = nullptr;
	connOnLost = nullptr;
	connOnStartArgs = nullptr;
	connOnLostArgs = nullptr;
}

Udp::~Udp()
{
	delete m_request;
	delete m_response;
	//消息路由机制
	delete m_msgRouter;
	//事件循环机制
	delete m_evLoop;  //最终只有一个fd被监听
	//fd对应的写事件和读事件
	delete m_channel;
}

int Udp::processRead()
{
	Debug("正在读fd：%d发来的消息并解析", m_channel->m_fd);
	int ret = 0; ;
	while((ret = m_rbuffer->readSocketToBuffer(m_channel->m_fd, &m_recvAddr, &m_recvaddrLen)) > 0) {//因为是一个包一个包读的，所以多个客户端同时发时，可以保证wait一次，读完一次
		//业务逻辑处理
		Debug("正在解析数据");
		//这个while的作用是防止多个请求连续过来
		while (m_rbuffer->getReadSize() != 0)
		{
			int dealSize = m_request->analyze(this); //解析数据（业务处理，并将想要的数据放到m_response）
			if (dealSize != 1) //解析数据（业务处理，并将想要的数据放到m_response）
			{
				std::cout << "包长度不足，已忽略" << std::endl;
				m_request->m_state = HandleState::MsgHead;
				//读缓冲区清空之前读到的数据
				m_rbuffer->readFromBuffer(nullptr, ret - dealSize, false);
				break;  //如果不为0 说明还需要数据，这个包不完整，直接丢掉（tcp可能需要继续等）
			}
		}
		Debug("读事件是否开启；%d", this->m_channel->readIsEnable());
	}
	//else {
		//客户端断开连接
		// 哪来的客户端需要断开，完全不需要管
		//m_evLoop->addTask(m_evLoop, m_evLoop->packageTask(m_channel, TaskType::DEL));
	//}
	return 0;
}

int Udp::processWrite()
{
	std::unique_lock<std::mutex> locker(m_sendMutex);  //当前作用域加锁
	//在写lb服务器转发消息的时候，按照lb服务器主要bug里面的20逻辑改了之后，如下
	//if (pkgLen <= 0) {  //这个条件不够好，需要改成地址个数
	// if (m_sendAddrQ.size() <= 0) {
	//Debug("正在关闭写事件");
	//m_channel->writeEnable(false); //将当前的channel开启写事件，注意这个开启之后，还没更新到检测树上，还需要任务队列-》处理任务-》判断这个channel的读写事件并更新到树上
	//m_evLoop->addTask(m_evLoop, m_evLoop->packageTask(m_channel, TaskType::MODI));  //这个代码块在这里的话，不会进行唤醒操作，前提是创建的线程和运行的是同一个
	//}
	//按照qt18的bug，上面的改成如下信号机制：
	m_sendCond.wait(locker, [this]() {
		return m_sendAddrQ.size() > 0;
		});
	Debug("processWrite正在加锁");
	//由于qt18的bug，增加了长度机制，那么可以连续发送多条消息，
	while (m_sendAddrQ.size() > 0) {
		auto& targetAddr = m_sendAddrQ.front();
		int pkgLen = targetAddr.msgLen;
		Debug("开始组织数据并响应，预响应的消息为：%s，包大小为：%d", m_wbuffer->m_data + m_wbuffer->m_readPos, pkgLen);
		int ret = 0;
		int totalLen = 0;
		while (totalLen < pkgLen) { //应该是< 而不是<=
#if 0
			//加了条件变量之后，肯定会有地址
			if (m_sendAddrQ.size() == 0) {
				//如果为0说明上层是有问题的，那么该怎么办，应该消耗掉这个包，让影响最低，怎么消耗呢
				//将接收的包地址压入队列
				std::cout << "udp包地址为空，请检查相应发送接口" << std::endl;
				//m_sendAddrQ.push(m_recvAddr);  //这个也有隐患，可能里面啥都没有
				return;
	}
#endif
			Debug("processWrite：当前要发送的地址信息：ip：%s, port:%d", inet_ntoa(targetAddr.addr.sin_addr), ntohs(targetAddr.addr.sin_port));
			ret = m_wbuffer->writeBufferToSocket(m_channel->m_fd, pkgLen, &targetAddr.addr, m_sendAddrLen);
			//这里不能简单的判断ret是否大于0，就认为写成功了，而是判断当前buf还有没有数据
			if (ret > 0) {
				totalLen += ret;
				//写完之后，如果没有写成功，那么写缓冲区，不会删除这个包的，所以这边也要同步，只有大于0，我才会弹出地址
				m_sendAddrQ.pop();
			}
			else if (ret == 0) {
				perror("sendto");
				break; //当前不可写下次再写，注意这时候写事件不关闭
			}
			else {
				//写失败，删除当前客户端，返回的是其他错误的-1
				//不用删除呀，根本就没有连接好吗
				//m_evLoop->addTask(m_evLoop, m_evLoop->packageTask(m_channel, TaskType::DEL)); //删除对应的客户端连接
				perror("sendto");
				return -1;
			}
		}
		Debug("本次发送的字节大小为：%d", totalLen);
	}
	////在写lb服务器转发消息的时候，按照lb服务器主要bug里面的20逻辑改了之后，这个代码块就省略了
	//这个if判断很关键
	//按照18的逻辑需要加上，并且条件不能改成地址
	if (m_wbuffer->getReadSize() == 0) {
		//发送完之后记得关闭写事件检测，这里又忘记了
		//增加条件变量机制之后，不能添加任务调用了，而需要直接修改，底层已经提供了直接修改的方法了
		Debug("正在关闭写事件--直接关闭");
		m_channel->writeEnable(false); //将当前的channel开启写事件，注意这个开启之后，还没更新到检测树上，还需要任务队列-》处理任务-》判断这个channel的读写事件并更新到树上，多线程情况下，不是左边的逻辑
		m_evLoop->modifyChannel(m_channel);
	}
	return 0;
}

int Udp::destroy()
{
	return 0;
}

//为了屏蔽上层的逻辑，地址不需要在上层压入栈，只需要传递进来就行，在这里负责将数据压入写缓冲区，以及将地址压入队列
int Udp::sendMsg(sockaddr_in& addr, bool copyResponse)
{
	//int IsSend = 0;  //这个主要是不想在那么多分支返回，导致写很多unlock，错误，不能使用这个
	Debug("正在执行SendMsg"); 
	Debug("sendMsg：当前要发送的地址信息（网络地址）：ip：%d, port:%d", addr.sin_addr.s_addr, addr.sin_port);
	//检验包的合法性
	if (m_response->m_msglen > MSG_LEN_LIMIT) {
		std::cout << "包过长，无法发送" << std::endl;
		return 0;//代表什么也没做
	}
	bool WriteIsActive = false;
	bool isPushAddr = true;

	//udp不能简单的判断 getReadSize==0就决定是否开启写事件
	//因为在连续的发送udp包给不同的主机的时候，会存在地址覆盖的问题（TCP因为只能是一对一，所以不存在覆盖的问题）
	//但是udp不一样，比如上层业务层在收到一个包（也可能没有收到，可能是主动发包的），然后会从wait那里出来执行预先注册好的业务函数
	//而在业务函数那里，如果需要不断地执行sendMsg发给不同地主机（这个时候地址就是udp属性的addr），
	// 但是注意，sendMsg只负责将数据写到缓冲区，至于数据的发送需要等待业务函数执行完回到wait那里才能检测到写事件，才能发送
	//一定要注意，在执行完这个业务函数之前，他是不会回到wait那里的，也就是即使对端发来数据，那也不会处理事件，因为根本没在wait那里
	// （这里要清楚如果是其他线程调用sendMsg，那么如果写缓冲区没有数据缓冲，会立马往自己的channel写，唤醒udp的wait，然后往对端写数据
	// 如果有写数据，一样，不会唤醒wait，而是放到写缓冲区就执行下一个sendMsg了，如果是udp自己的线程连续调用sendMsg，也是一样的，也就是
	// 如果写缓冲区没有数据缓冲，会立马往自己的channel写，但是当前线程还没有到wait，而是继续执行下一个sendMsg（这个时候就不会往
	// 自己的channel写数据了），当所有的sendMsg执行完之后，才会回到wait，然后检测，立马响应写事件）
	//但是业务函数那里需要执行多条sendMsg，这个时候后面的地址就覆盖了前面的地址，导致只能发给一个地址
	//这怎么处理，其实也挺好处理，第一个将原来的地址改为地址队列，那么我写缓冲区的数据准备好了，地址队列也准备好了，一一对应的取就行了
	//第二种待发送消息队列，sendMsg在发送第二个消息的时候，发现当前缓冲区有数据，并且第二个发送的数据和第一个的地址不一样，
	//那么就把这个消息和地址压到队列中，等到第一个消息发送完毕，再从队列弹出
	//第三个就是在udp类中增加同步信号量，业务层那里每次sendMsg之后，就会阻塞等待（如果当前线程一阻塞，但是wait会有事件触发），当将数据发送到对端之后，然后唤醒
	//第三个是无效的，因为本质上只有一个线程，你让他休眠，他醒来之后还是会回到对应的即将要执行的位置，可以将udp的sendMsg放到一个线程里，那么就可以用第三种方式
	//但是我最终还是打算采用第一种方式，因为比较简单
	//但是读的包也需要记录包的地址，这咋办，感觉需要一个单独的地址和一个队列，单独的地址用来存放默认接收和发送（一个主机）的，
	//而（连续接收（不同主机）是不存在的，因为我每次读一个包出来，然后就去执行业务逻辑了，也就是我这边是缓冲区是读出来一个包然后立马处理掉
	//）和发给不同的主机需要使用队列（也就是这一个包在处理业务时需要发送多个包）那processWrite怎么选择队列还是变量呢，
	//感觉还是只用一个队列比较好，也就是读包的时候，我需要将地址压入队列，然后在处理这个包的时候，必须要消耗掉这个包的地址（也就是如果你发包
	//业务层就不用压入地址了，但是如果发包发的不是对应的udp，那么你需要弹出当前地址，然后再压入新的地址，这还挺麻烦的）
	//可以采用读地址和写地址分离，读地址只需要一个变量，写地址需要一个队列，我一定要自始至终保证队列地址和写缓冲区的包的一个对应关系
	//上层即使回复消息给读地址读到的变量，也需要将这个变量的地址押入到队列中，这样就完美解决了问题，所以之前写的关于udp的发送接收过程都要进行小幅度更改

	//在写lb服务器转发消息的时候，按照lb服务器主要bug里面的20逻辑改了之后，如下，增加了判断是否已经开启的条件
	//m_msgInsertMutex.lock();  //加锁, 先加锁吧，后面如果按照qtCLient的bug18思路优化，感觉可以去掉,使用下面的方式加锁，当前生命周期会一直加锁
	std::unique_lock<std::mutex> locker(m_sendMutex);
	Debug("sendMsg正在加锁");
	//这个判断条件改了，可以看qt注意bug18，之前还判断缓冲区，完全没必要
	auto& lastMsgAddr = m_sendAddrQ.back();
	if (!m_channel->writeIsEnable()) {//如果没有缓冲，且没有开启写事件，那么可以开启
		WriteIsActive = true;
	}
	else {
		WriteIsActive = false;  //只要前面还有包在发，说明一定开启过写事件，此时后续的包都不需要开启  ，，存疑！！！，上面的条件改过之后，这个可以确认为false了
		//如果不为空，那么比对最后的包的地址和当前的地址是否一样，如果一样，什么也不干，直接压入数据，不压入地址
		//如果不一样，那么需要强制先发送出缓冲数据
		//参考bug18，不强制发送数据了，而是只记录到缓冲中去
		if ((lastMsgAddr.addr.sin_addr.s_addr != addr.sin_addr.s_addr) || (lastMsgAddr.addr.sin_port != addr.sin_port)) {
			//如果地址不同，那就记录到msg，并压入地址，否则不压入地址
			//processWrite();
			isPushAddr = true;
		}
		else {
			isPushAddr = false;
		}
	}
	if (copyResponse) {
		if (m_response->m_state != HandleState::Done) {
			Debug("当前响应状态：%d", m_response->m_state);
			//m_msgInsertMutex.unlock();
			//这里必须return，
			return 0; //因为这个return是根本不想执行下面的语句，上面的也是
		}
		Debug("当前的数据为:%d,%d,%s", m_response->m_msgid, m_response->m_msglen, m_response->m_data);
		Debug("m_wbuffer->getReadSize():%d", m_wbuffer->getReadSize());
		m_wbuffer->writeToBuffer((const char*)&m_response->m_msgid, 4); //可以强转，这是个技巧
		m_wbuffer->writeToBuffer((const char*)&m_response->m_msglen, 4);
		if (m_wbuffer->writeToBuffer(m_response->m_data, m_response->m_msglen) <= 0) {
			Debug("数据写入写缓冲区失败，写数据大小为：%d", m_response->m_msglen);
			m_wbuffer->readFromBuffer(nullptr, MSG_HEAD_LEN, false);	
			//m_msgInsertMutex.unlock();
			//这里必须return， //代表当前任务执行失败
			return -1; //因为这个return是根本不想执行下面的语句，上面的也是
		}
		Debug("当前即将写的写缓冲区的内存情况为：m_readPos：%d，m_writePos：%d，getReadSize()：%d，getWriteSize()：%d，data：%s",
			m_wbuffer->m_readPos, m_wbuffer->m_writePos, m_wbuffer->getReadSize(), m_wbuffer->getWriteSize(), m_wbuffer->m_data + m_wbuffer->m_readPos);
	}
	//到这里说明数据已经成功压入写缓冲区，所以这里需要把地址压入队列，如果数据压入失败，那么也就执行不到这，也就不会压入地址
	//m_sendAddrQ.push(addr);
	//不能直接压入，需要进行一系列的判断

	Debug("压入前当前udp地址队列情况：");//注意，队列只能访问队头和队尾，不能遍历
	Debug("当前udp地址队列大小：%d", m_sendAddrQ.size());
	if (m_sendAddrQ.size() != 0) {
		Debug("队头（网络地址）：ip：%d, port:%d", m_sendAddrQ.front().addr.sin_addr.s_addr, m_sendAddrQ.front().addr.sin_port);
		Debug("队尾（网络地址）：ip：%d, port:%d", m_sendAddrQ.back(),addr.sin_addr.s_addr, m_sendAddrQ.back().addr.sin_port);
	}
	struct sendMsgInfo msgInfo;
	int msgLen = m_response->m_msglen + 8; //这个8是消息头
	if (isPushAddr) {
		Debug("正在压入消息地址");
		msgInfo.addr = addr;
		msgInfo.msgLen = msgLen;
		m_sendAddrQ.push(msgInfo);
	}
	else {
		lastMsgAddr.msgLen += msgLen;
	}
	Debug("当前udp地址队列情况：");//注意，队列只能访问队头和队尾，不能遍历
	Debug("当前udp地址队列大小：%d", m_sendAddrQ.size());
	Debug("队头（网络地址）：ip：%d, port:%d", m_sendAddrQ.front().addr.sin_addr.s_addr, m_sendAddrQ.front().addr.sin_port);
	Debug("队尾（网络地址）：ip：%d, port:%d", m_sendAddrQ.back().addr.sin_addr.s_addr, m_sendAddrQ.back().addr.sin_port);
	Debug("sendMsg正在解锁");
	m_sendCond.notify_one();  //唤醒一个线程
	m_response->m_state = HandleState::MsgHead; //代表当前任务已处理
	//激活写事件
	if (WriteIsActive) {
		Debug("SendMsg正在激活写事件");
		m_channel->writeEnable(true);
		m_evLoop->addTask(m_evLoop, m_evLoop->packageTask(m_channel, TaskType::MODI));
	}
	return 1;//代表当前任务执行成功
}


bool Udp::addMsgRouter(int msgid, msgCallBack callBack, void* userData)
{
	m_msgRouter->submit(msgid, callBack, userData);
	return true;
}

UdpServer::UdpServer(const char* ip, unsigned short port)
{
	//获得套接字
	m_rbuffer = new ReadBuffer();
	m_wbuffer = new WriteBuffer();
	Debug("m_readPos:%d, m_writePos:%d", m_rbuffer->m_readPos, m_rbuffer->m_writePos);
	int fd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
	if (fd == -1) {
		perror("socket");
		exit(-1);
	}
	//绑定本地的ip和端口
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	int len = sizeof(sockaddr_in);
	int ret = bind(fd, (sockaddr*)&addr, len);
	if (ret == -1) {
		perror("socket");
		exit(-1);
	}
	auto readFunc = std::bind(&UdpServer::processRead, this);
	auto writeFunc = std::bind(&UdpServer::processWrite, this);
	auto destroyFunc = std::bind(&UdpServer::destroy, this);
	m_channel = new Channel(fd, FDEvent::ReadEvent, readFunc, writeFunc, destroyFunc, nullptr);

	//将channel添加到检测集合
	m_evLoop->addTask(m_evLoop, m_evLoop->packageTask(m_channel, TaskType::ADD));
}

UdpServer::~UdpServer()
{
}

UdpClient::UdpClient(const char* ip, unsigned short port)
{
	Debug("正在初始化UdpClient");
	//这里必须要设置的足够大，因为recvfrom只能读一次，即使读不完下次也没机会读了
	m_rbuffer = new ReadBuffer(MSG_LEN_LIMIT + MSG_HEAD_LEN);
	m_wbuffer = new WriteBuffer(MSG_LEN_LIMIT + MSG_HEAD_LEN);
	//在udp增加地址队列之后，发现server可以很好的运行，但是到这里发现客户端不行，客户端其实只需要给同一个udp发消息就行了
	//如果硬要套进去，也可以，就是初始化的时候在接收地址里填入初始地址，然后发包的时候，传进来那个地址就行了，每次发包都传进来那个地址
	// 因为接收的数据肯定是同一个服务器的，所以这是没啥问题的
	//所以上面的地址队列应该放到sever属性的，不过不管了，直接就这样弄吧，这样可以间接增加了客户端可以和多台服务器通信的能力了
	m_recvAddr.sin_family = AF_INET;
	m_recvAddr.sin_port = htons(port);
	m_recvAddr.sin_addr.s_addr = inet_addr(ip);
	int fd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);  //在这里直接设置非阻塞模式了
	if (fd == -1) {
		perror("socket");
		exit(-1);
	}

	auto readFunc = std::bind(&UdpClient::processRead, this);
	auto writeFunc = std::bind(&UdpClient::processWrite, this);
	auto destroyFunc = std::bind(&UdpClient::destroy, this);
	m_channel = new Channel(fd, FDEvent::ReadEvent, readFunc, writeFunc, destroyFunc, nullptr);

	//将channel添加到检测集合
	m_evLoop->addTask(m_evLoop, m_evLoop->packageTask(m_channel, TaskType::ADD));

	//之前在这执行的hook，我不知道怎么测试的，这根本就是不行的，因为我还没法设置hook，你怎么执行
	//hook函数
	//if (connOnStart != nullptr) {
		//Debug("正在执行connOnStart");
		//connOnStart(this, connOnStartArgs);
	//}
	Debug("UdpClient初始化完成");
}

UdpClient::~UdpClient()
{
}

void Udp::run()
{
	//把hook放到这里了
	//hook函数
	if (connOnStart != nullptr) {
		connOnStart(this, connOnStartArgs);
	}
	m_evLoop->run();
}
