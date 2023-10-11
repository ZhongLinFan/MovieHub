#include "NetConnection.h"
#include <assert.h>
#include <errno.h>
#include <functional>
//#include "TcpServer.h"  这里为啥会有这个， 这个竟然是负责引用Message的。。。
#include "Message.h"
#include <cstdlib> // 随机数
#include <string.h>

NetConnection::NetConnection()
{
	srand(time(NULL));  //随机数种子产生真正的随机数
	m_request = new Message();
	m_response = new Message();
	m_rbuffer = nullptr; 
	m_wbuffer = nullptr;
	m_dymaicParam = nullptr;
}
int NetConnection::processRead()
{
	Debug("正在读fd：%d发来的消息并解析", m_channel->m_fd);
	int ret = m_rbuffer->readSocketToBuffer(m_channel->m_fd);
	if (ret > 0) {
		//业务逻辑处理
		//std::cout << "开始解析数据" << std::endl;
		//这个while的作用是防止多个请求连续过来
		while (m_rbuffer->getReadSize() != 0)
		{
			int analyzeState = m_request->analyze(this); //解析数据（业务处理，并将想要的数据放到m_response）
			if (analyzeState == 0)break;  //如果为0 说明还需要数据，就不需要继续while了
		}
		
		//为什么要有一个sendmsg函数，而且必须要有（并且激活写事件的控制权也必须要放到sendmsg）
		// 因为考虑到主动发送机制和被动发送机制的统一性，
		// 首先被动发送机制肯定会直接调用processRead，然后分析数据，最后根据分析出的数据，
		// 调用sendmeg函数将响应消息放到m_wbuffer里，然后激活写事件调用processWrite
		// 主动发送机制则是直接调用sendmeg函数将响应消息放到m_wbuffer里，然后激活写事件调用processWrite
		// 可以看到他们后面一部分是相同的，完全可以让他们都走同一个逻辑
		//sendMsg();//包括组织m_response数据到m_wbuffer，以及开启写事件
		// 因为实现了消息路由机制，发送消息到wbuffer里或者说给不给对端回应（因为如果回复的消息小于等于0直接返回）的权力交给了业务函数
		
		//注意这里不一定处理哦，当前响应数据块没准备好，那么就直接不执行
		//开启写事件!!!!
		//m_wbuffer->writeToBuffer(m_rbuffer, ret); //简单的拷贝数据
		//m_channel->writeEnable(true); //将当前的channel开启写事件，注意这个开启之后，还没更新到检测树上，还需要任务队列-》处理任务-》判断这个channel的读写事件并更新到树上
		//m_evloop->addTask(this->m_evloop, m_channel, TaskType::MODI);
	}
	else {
		//客户端断开连接
		m_evloop->addTask(this->m_evloop, m_evloop->packageTask(m_channel, TaskType::DEL));
	}
	return true;
}
int NetConnection::processWrite() {
	//业务逻辑处理
	//这里简单的将m_rbuffer的数据拷贝到写缓冲区
	Debug("开始组织数据并响应，预响应的消息为：%s", m_wbuffer->m_data + m_wbuffer->m_readPos);
	//发送数据
	int sendSize = m_wbuffer->getReadSize(); //不加这个也是可以的，我这个就是一次只发送size个字节，多了我也不发
	int ret = 0;
	int totalLen = 0;
	while (totalLen < sendSize) { //这里其实还是不能保证只写sendSize个数据，因为可能当前不可写的时候，那个时候又有数据到这里了，那么可能就会
		//把当时的所有数据都写出去，这个时候就会出现ret > sendSize的情况
		//这里应该是小于而不是<=
		ret = m_wbuffer->writeBufferToSocket(m_channel->m_fd);
		//这里不能简单的判断ret是否大于0，就认为写成功了，而是判断当前buf还有没有数据
		if (ret > 0) {
			totalLen += ret;
		}
		else if (ret == 0) {
			break; //当前不可写下次再写，注意这时候写事件不关闭
		}
		else {
			//写失败，删除当前客户端，返回的是其他错误的-1
			m_evloop->addTask(this->m_evloop, m_evloop->packageTask(m_channel, TaskType::DEL)); //删除对应的客户端连接
		}
	}
	Debug("本次发送的字节大小为：%d", totalLen);
	//这个if判断很关键
	if (m_wbuffer->getReadSize() == 0) {
		//发送完之后记得关闭写事件检测，这里又忘记了
		Debug("正在关闭写事件");
		m_channel->writeEnable(false); //将当前的channel开启写事件，注意这个开启之后，还没更新到检测树上，还需要任务队列-》处理任务-》判断这个channel的读写事件并更新到树上
		m_evloop->addTask(this->m_evloop, m_evloop->packageTask(m_channel, TaskType::MODI));
	}
	return true;
}

int NetConnection::sendMsg(bool copyResponse)
{
	Debug("正在执行SendMsg");
	bool WriteIsActive = false;
	if (m_wbuffer->getReadSize() == 0 ) {
		WriteIsActive = true;
	}
	else {
		//应该开启，然后检查一下是否开启,如果没有开启需要开启
		if(!m_channel->writeIsEnable())WriteIsActive = true;
	}
	Debug("本次发送数据，是否激活写事件：%d", WriteIsActive);
	if (copyResponse) {
		//这里需要先判断Msg的解析状态在决定处不处理
		if (m_response->m_state != HandleState::Done) {
			Debug("当前响应状态：%d", m_response->m_state);
			return 0; //代表什么也没做
		}
		
		//如果第一个sendMsg执行到processWrite正在将之前的数据写到对端，但是还没写完，这个时候又有一个业务执行了sendMsg，
		//这个时候就不需要再开启了
		//注意，我的诉求是不管业务层怎么调用sendMsg，我都是一个包一个包的长度去发送
		//但是有可能出现这么一种情况：就是第一个sendMsg正在执行到writeBufferToSocket（注意这个函数的任务一次就是发送 指定字节的数据），
		//这个时候又来一个sendMsg要开始往buf里写，（注意writeBufferToSocket这个只有readPos在那一瞬间需要更改（如果不扩容的话），
		// 而sendMsg只更改writePos，所以一般情况是不需要加锁的，当然可以在扩容那里加一把锁，如果严谨的话）
		//注意，第二次的写，并不妨碍你写，但是我第一次写到对端后（只写那么长的数据），会判断当前可写长度，如果长度不为0，我是不关闭写事件的，这个是关键点，所以这里可以
		//进行下面的逻辑操作，显然为0的时候肯定关闭了写事件，所以需要打开
		//那如果sendMsg函数正在拷贝的时候，又来一个sendMsg呢，显然不行，需要加锁访问
		//其实还是有一个很小的隐患，就是第一个sendMsg在添加任务的过程，第二个sendMsg已经开始往里写数据了，这个时候可能会产生粘包，但是
		//因为分析函数分析的很好，不怕这个，所以这个可以忽略（这个不可能发生，因为addTask还是在sendMsg函数里，sendMsg函数需要互斥访问）
		Debug("当前的数据为:%d,%d", m_response->m_msgid, m_response->m_msglen); 
		Debug("m_wbuffer->getReadSize():%d", m_wbuffer->getReadSize());
		m_wbuffer->writeToBuffer((const char*)&m_response->m_msgid, 4); //可以强转，这是个技巧
		m_wbuffer->writeToBuffer((const char*)&m_response->m_msglen, 4);
		//如果写消息体失败了，那么应该弹出消息头
		//这里限制了不能发送空消息，也就是m_msglen不能为0
		if (m_wbuffer->writeToBuffer(m_response->m_data, m_response->m_msglen) < 0) {
			Debug("数据写入写缓冲区失败，写数据大小为：%d", m_response->m_msglen);
			//注意这里需要倒着读，因为可能前面的第一个包正常，第二个包不正常，那你不可能从头读，而是从尾巴还原
			//m_wbuffer->readFromBuffer(nullptr, MSG_HEAD_LEN, false);
			m_wbuffer->m_writePos -= MSG_HEAD_LEN;
			if (m_wbuffer->m_writePos < 0)m_wbuffer->m_writePos = 0;
			return -1; //代表当前任务执行失败
		}
		Debug("当前即将写的写缓冲区的内存情况为：m_readPos：%d，m_writePos：%d，getReadSize()：%d，getWriteSize()：%d，data：%s",
			m_wbuffer->m_readPos, m_wbuffer->m_writePos, m_wbuffer->getReadSize(), m_wbuffer->getWriteSize(), m_wbuffer->m_data + m_wbuffer->m_readPos);
	}
	
	m_response->m_state = HandleState::MsgHead; //代表当前任务已处理
	//激活写事件
	if (WriteIsActive) {
		Debug("SendMsg正在激活写事件");
		m_channel->writeEnable(true);
		m_evloop->addTask(this->m_evloop, m_evloop->packageTask(m_channel, TaskType::MODI));
	}
	return 1; //代表当前任务执行成功
}

NetConnection::~NetConnection()
{
	//删除对应的delFd_TcpConn
	//TcpServer::delFd_TcpConn(m_channel->m_fd); //在这删除是不成熟的做法，这里都是析构对象，这个对应关系应该是在deleteChannel这里进行的
	//不需要释放evloop，因为evloop是属于线程的，这里引入只是为了标记tcpConnection是属于他的
	//释放channel
	delete m_channel;
	//释放rbuffer
	delete m_rbuffer;
	//释放wbuffer
	delete m_wbuffer;

	//释放创建时开辟的存放客户端地址的内存
	//一定要new的时候就想到在哪里释放，否则容易忘，这个前提条件就是整个系统的部件释放的时机要理解的很成熟
	//delete m_clientAddr;

	//注意，当前需要释放的资源是对象，和对应的同步关系，如果采取delete TcpConnection的方式，那么应该调用一下deleteChannel
	//而且还要主动调用deleteChannel的方法删除对应的关系，包括epoll树上的fd，注意Dispatcher相关的东西肯定是当前服务器结束才停止的，不需要管
	// 而且epoll poll select等也没有new空间（不是构造函数里的）
	//如果采用添加任务的方式，那么可以直接在删除epoll树上的时候，就可以全部清理干净
	//或者直接和epoll树打交道，调用对应的函数删除对应的模型
	//在epoll（Poll::remove() }）上删除完节点之后直接调用（可以是回调函数，也可以将那个函数设置为静态）更好一点
	//一个删除函数，将所有的对应关系和资源释放干净
	//按照我原来的想法是在deleteChannel的地方删除关系，在这里删除资源，在epoll树上删除fd，这个太分散了，后期维护需要在三个地方维护

	//释放请求和响应，其实里面啥都不干
	delete m_request;
	delete m_response;
	//删除连接属性
	if (m_dymaicParam != nullptr) {
		delete m_dymaicParam;
	}
}

