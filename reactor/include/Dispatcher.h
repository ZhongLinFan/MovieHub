#pragma once
#include "Channel.h"
//#include "EventLoop.h" //不需要，因为有前置声明
//不对，因为这个类里用到了EventLoop的方法，不仅仅是一个结构体，所以前置声明解决不了问题，不过可以把它放到cpp文件
#include <sys/epoll.h>
#include <poll.h>
#include <sys/select.h>
#include <string>
#include "Log.h"

//所以当前的Dispatcher要记录哪个一下自己所属的eventLoop，这就有点一层一层绑定的意思了
//class EventLoop;  //上面有Evloop头文件了这个不需要了
class EventLoop;  //上面有Evloop头文件了这个不需要了，这个也必须要
class Dispatcher {
public:
	Dispatcher(EventLoop* evLoop);
	//添加cfd
	virtual bool add() = 0;
	//删除cfd
	virtual bool remove() = 0;
	//修改cfd
	virtual bool modify() = 0;
	//事件检测
	virtual bool wait() = 0;
	//清除事件循环
	~Dispatcher();  //如果声明了不定义，在使用静态库时会出问题
	inline void setChannel(Channel* channel) {
		m_channel = channel;
	}
public:
	std::string m_name;
	EventLoop* m_evLoop; //记录所属的evloop，要不然激活函数处理的时候没法调用，其实完全可以省掉m_evLoop的调用activeEventProcess，
					//因为activeEventProcess也只是分类调用了channel的回调函数，从而省掉这个属性，从而使得结构更简洁，
					//	这里先保留吧，后期能改再改
					// 不对，不能省掉这个属性，因为通过fd找对应channel还是需要evLoop
	Channel* m_channel;
};

class Epoll:public Dispatcher {
public:
	Epoll(EventLoop* evLoop);
	//添加
	bool add() override;
	//删除
	bool remove() override;
	//修改
	bool modify() override;
	//事件检测
	bool wait() override;
	//清除数据
	//清除的是eventLoop对应的dispatcherData的数据
	~Epoll();
	void epollCtl(int mode);
public:
	int m_epfd;
	struct epoll_event* m_events;
	const int m_maxSize = 10240;
};

class Poll:public Dispatcher {
public:
	Poll(EventLoop* evLoop);
	//添加
	bool add() override;
	//删除
	bool remove() override;
	//修改
	bool modify() override;
	//事件检测
	bool wait() override;
	//清除数据
	//清除的是eventLoop对应的dispatcherData的数据
	~Poll();
public:
	int m_maxfd;
	struct pollfd* m_fds;
	const int m_maxSize = 1024;
};

class Select:public Dispatcher {
public:
	Select(EventLoop* evLoop);
	//添加
	bool add() override;
	//删除
	bool remove() override;
	//修改
	bool modify() override;
	//事件检测
	bool wait() override;
	//清除数据
	//清除的是eventLoop对应的dispatcherData的数据
	~Select();
public:
	int m_maxfd;
	fd_set* m_readfds;
	fd_set* m_writefds;
	const int m_maxSize = 1024;
};