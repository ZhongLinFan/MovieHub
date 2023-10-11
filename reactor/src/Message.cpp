#include "Message.h"
#include "Udp.h" 
//#include "TcpServer.h"  这个不知道为啥加在这
#include "MsgRouter.h"  //cpp文件用到了必须要有头文件
#include "NetConnection.h"  //这个确实用到了，虽然不加没报错，但是应该加
Message::Message()
{
	m_msgid = -1;
	m_msglen = 0;
	m_data = nullptr;
	m_state = HandleState::MsgHead;
}

Message::~Message()
{
}

//解析请求，和组织响应，因为他们的格式都一样（最终就是三个值），那么可以一起解析，只不过响应的函数不一样
int Message::analyze(NetConnection* conn)
{
	if (getRequest(conn->m_rbuffer) == HandleState::Response) {
		//说明解析成功，可以回复消息了
		//必须要回复成功，因为如果回复不成功，那么下次外面read到buf，直接把原来的数据清空了
		// 或者在外面赶紧回复，但是在外面回复还是会激活写事件，也就是添加写事件（sendmsg）之后，我肯定要先执行wait函数，再执行
		// processWrite这个函数的，但是我可以确保在sendmsg里就把当前指向的数据读到接收数据之后，所以不需要关掉当前fd的读事件，
		// 即使有读事件，将原来的数据覆盖掉了，那也是sendmsg之后的事情了
		//回复完之后还原到最初始位置
		//这里简单的将原来的数据做一个回显
		Debug("当前数据解析成功,msgid：%d，msglen：%d，m_data：%s ", m_msgid, m_msglen, m_data);
		Debug("Response,读缓冲区的读指针为：%d，读缓冲区的写指针为：%d", conn->m_rbuffer->m_readPos, conn->m_rbuffer->m_writePos);
		m_state = HandleState::Done; //这里代表请求解析已完成，剩下的交给响应了
		conn->m_msgRouter->call(conn);  //注意这里的输入是request，输出是经过response
		//（response->m_msgid等都是在业务函数里进行处理的，而不是在这里处理，因为我可能在业务逻辑里
		//针对id为1的请求给你封装一个id为200的响应），到达wbuffer

		Debug("request状态已修改为 HandleState::MsgHead");
		//m_state = HandleState::MsgBody;  为什么要写成这个！！！！导致读指针不正常，严重滞后写指针
		m_state = HandleState::MsgHead;//从这里出来说明可以当前消息处理完毕，可以恢复到原始状态，

		m_data = nullptr;
		return 1;
	}
	return 0;
}

int Message::analyze(Udp* udp)
{
	if (getRequest(udp->m_rbuffer) == HandleState::Response) {
		Debug("当前数据解析成功,msgid：%d，msglen：%d，m_data：%s ", m_msgid, m_msglen, m_data);
		Debug("Response,读缓冲区的读指针为：%d，读缓冲区的写指针为：%d", udp->m_rbuffer->m_readPos, udp->m_rbuffer->m_writePos);
		m_state = HandleState::Done; //这里代表请求解析已完成，剩下的交给响应了
		udp->m_msgRouter->call(udp);
		//（response->m_msgid等都是在业务函数里进行处理的，而不是在这里处理，因为我可能在业务逻辑里
		//针对id为1的请求给你封装一个id为200的响应），到达wbuffer
		Debug("request状态已修改为 HandleState::MsgHead");
		//m_state = HandleState::MsgBody;  为什么要写成这个！！！！导致读指针不正常，严重滞后写指针
		m_state = HandleState::MsgHead;//从这里出来说明可以当前消息处理完毕，可以恢复到原始状态，
		m_data = nullptr;
		return 1;
	}
	else if (getRequest(udp->m_rbuffer) == HandleState::WaitingHead) {
		return 0;  //代表未解析
	}
	else if (getRequest(udp->m_rbuffer) == HandleState::WaitingBody) {
		return 8;  // 代表处理了8个字节
	}
	else {
		std::cout << "解析状态出错" << std::endl;
		return 0;
	}
	
}

HandleState Message::getRequest(ReadBuffer* buf)
{
	int size = -1;
	while (m_state != HandleState::Done)
	{
		switch (m_state) {
		case HandleState::MsgHead:
			if (buf->getReadSize() < MSG_HEAD_LEN) {
				Debug("buf->getReadSize() < MSG_HEAD_LEN");
				m_state = HandleState::WaitingHead;
				return m_state;
			}
			//size = buf->readFromBuffer((char *)this, MSG_HEAD_LEN, true);  //读到this这个设计的可能不好，错误的
			//size = buf->readFromBuffer((char*)&this->m_msgid, 4, true);这样是错的，要传进去地址
			size = buf->readFromBuffer((char*)&this->m_msgid, 4, true);
			size = buf->readFromBuffer((char*)&this->m_msglen, 4, true);
			Debug("this->m_msgid:%d, this->m_msglen:%d", this->m_msgid, this->m_msglen);
			Debug("MsgHead,读缓冲区的读指针为：%d，读缓冲区的写指针为：%d", buf->m_readPos, buf->m_writePos);
			m_state = HandleState::MsgBody;
			break;
		case HandleState::MsgBody:
			if (buf->getReadSize() < m_msglen) {
				Debug("MsgBody正在HandleState::WaitingBody， m_msglen为：%d,getReadSize为%d",
					m_msglen, buf->getReadSize());
				m_state = HandleState::WaitingBody;
				return m_state;
			}
			size = buf->readFromBuffer(&m_data, m_msglen);  //注意这里并不会造成m_data指向的数据失效的问题（比如读socket到buffer，扩容buf）
			//因为我当前线程执行到这的时候说明正在分析数据，那么不会处于wait状态，也就是即使对端来数据，我现在也不知道，更不用提读数据到buf以及扩容了
			//如果当前长度不够，那么我就不会读，你想怎么扩容怎么扩容，如果长度够了，那么当前线程会让m_data指向buf的一个固定位置，
			//然后紧接着执行对应的对应业务的函数（执行完m_data就结束了，注意到这依然没有回到wait）（这个业务函数里面可能会有发送sendMsg函数）
			//至于写的m_data(m_response)，执行完sendMsg，你这个m_data就没用了，所以完全不用担心m_data失效问题
			Debug("解析到的数据为：%s,大小为：%d", m_data, size);
			Debug("MsgBody,读缓冲区的读指针为：%d，读缓冲区的写指针为：%d", buf->m_readPos, buf->m_writePos);
			m_state = HandleState::Response;
			return m_state;
		case HandleState::WaitingHead:
#if 0
			//这写的有问题，假如第一次状态设置为了这个，第二次进来还是小于8个字节，那么会一直卡在while里
			if (buf->getReadSize() >= MSG_HEAD_LEN) {
				m_state = HandleState::MsgHead;
			}
			break;
#endif
			if (buf->getReadSize() >= MSG_HEAD_LEN) {
				m_state = HandleState::MsgHead;
				break;
			}
			return m_state;
		case HandleState::WaitingBody:
			if (buf->getReadSize() >= m_msglen) {
				m_state = HandleState::MsgBody;
				break;
			}
			return m_state;
		default:
			return m_state; // 这里不应使用break，因为break会一直陷入while里
		}
	}
}

