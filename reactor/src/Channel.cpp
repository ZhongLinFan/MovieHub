#include "Channel.h"
#include <unistd.h> // close

Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc , void* args)
{
	//我一开始还保留malloc后来改成new ，这个是在外面开辟的
	m_fd = fd;
	m_events = (int)events;
	readCallBack = readFunc;
	writeCallBack = writeFunc;
	destroyCallBack = destroyFunc;
	m_args = args;
}

void Channel::writeEnable(bool mode)
{
	if (mode) {
		m_events |= static_cast<int>(FDEvent::WriteEvent);
	}
	else {
		m_events &= ~static_cast<int>(FDEvent::WriteEvent); //~我把这个放到里面了
	}
}

void Channel::readEnable(bool mode)
{
	if (mode) {
		m_events |= static_cast<int>(FDEvent::ReadEvent);
	}
	else {
		m_events &= ~static_cast<int>(FDEvent::ReadEvent);
	}
}

bool Channel::writeIsEnable()
{
	return m_events & (int)FDEvent::WriteEvent;
}

bool Channel::readIsEnable()
{
	return m_events & (int)FDEvent::ReadEvent;
}

Channel::~Channel()
{
	close(m_fd);
	//free(channel);
}

#if 0
//这种方法不可行，必须要在外面绑定，否则参数不匹配，也就是传输之前就要打包好
bool Channel::changeWriteCallBack(int(*writeFunc)(void*), void* args)
{
	auto writeCallBack = std::bind(writeFunc, this);
	m_args = args;
	return true;
}
#endif

bool Channel::changeWriteCallBack(handleFunc writeFunc, void* args)
{
	writeCallBack = writeFunc;
	m_args = args;
	return true;
}