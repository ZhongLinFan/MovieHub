#pragma once
//#include "Dispatcher.h"这里没有用到里面的方法，可以放在cpp文件
#include <map>
#include <queue>
#include "Channel.h"
#include <thread>
#include <mutex>
#include "Log.h"
#include <functional>
#include "Task.h"  //这里用到了实体，必须包含头文件
#include <vector>

// 没有m_cond，m_myChannel，m_tasks是ChannelElement类型，虽然属性里没有m_myChannel，但是构造函数里直接有的
//初始化时默认没有启动，我的是启动了
//m_cmap = map<int, channel*>();m_tasks = vector<int>()
//addChannel等系列函数有很大不同，而且也有很多函数我这没有

//还是放在另一个文件里吧
class Dispatcher;
class EventLoop {
public:
	//evloop初始化
	EventLoop(std::thread::id threadID, int seq); //seq为了使得名字有编号，比如反应堆1号
	//销毁
	~EventLoop();
	//启动evloop,开始事件检测
	void run();
	//处理已经被激活的事件，epoll树上的激活的事件
	void activeEventProcess(int fd, int event);
	//向任务队列中添加任务,mode表示任务的类型，是添加fd，删除fd，还是修改fd
	void addTask(EventLoop* evloop, Task* task);
	//向自己的文件描述符里写数据
	void wakeupThread(EventLoop* evloop);
	//读自己的文件描述符里的数据
	int readMyData();
	//处理队列中的任务
	void tasksProcess();
	//添加文件描述符到待检测集合
	bool addChannel(Channel* channel);
	//从待检测集合删除文件描述符
	bool deleteChannel(Channel* channel);
	//修改文件描述符到待检测集合
	bool modifyChannel(Channel* channel);

	//为了能让任务队列处理不同类型的任务（不只是channel），增加一个打包任务的函数
	Task* packageTask(Channel* channel, TaskType mode);  //tcp连接管理的打包
	Task* packageTask(taskCallBack task, void* args=nullptr, TaskType mode = TaskType::NORMAL); // 普通任务的打包

public:
	bool m_isQuit;
	Dispatcher* m_dispatcher;  //用指针记录变量地址，不要直接用变量存储，否则会报类型不完全
	/*
		struct ChannelMap* cmap; 记录fd和对应的channel的对应关系，因为channel里存储了相应的回调函数，不是TcpConnection，不过是他也可以，因为他里面也存放了对应的channel
		但是那样逻辑性就不强了，因为读写回调就是在channel里面已经注册好的，直接调用即可

		任务队列，也可以称之为消息队列
		struct ChannelElement* head;
		struct ChannelElement* tail;
	*/
	//对应关系队列
	std::map<int, Channel*> m_channelMap;
	//任务队列,一开始使用的是vector。。。。导致头部没法弹出。。。
	//std::queue<ChannelElement*> m_tasks;  //要么是主线程的就绪的cfd，这时需要交给子线程封装，TODO要么是一般任务
	std::queue<Task*> m_tasks;  //为了改成一般任务，把任务类型改成了Task
	std::thread::id m_threadID;
	int m_seq;
	std::mutex m_mutex;
	int m_pipefd[2]; //pipefd[0] 是主线程操控的，pipefd[1]是子线程操控的，
	Channel* m_myChannel;
	//std::map<int, Channel*> m_channelMap;如果只是记录对应的fd和回调之间的关系的话，那么刚刚好，但是如果需要统计当前线程的负载情况
	//比如当前线程1分钟的吞吐量，那么用tcpconnection更好一点
	//不行，有一个channel 没有TCPConnection，只是个channel。。而且m_channelMap里面混杂了不是客户端的fd，就是这个m_myChannel
	//上面这个需要改成TCPConnection类型，这样在检测模型中删除对应的TCPConnection就有两种方法，否则只有一种，但是m_channelMap，
	// 因为m_myChannel，这就导致不行，还是老老实实通过回调+传参的方式去执行把（第二种方式，直接调用对应的函数，不需要回调函数）
	// 如果后期还需要，那么就要改了，把自己的那个也封装成TcpCOnnection
	//统计当前线程的吞吐量 kb为单位,单位时间为s

	//当前在线的客户端fd集合，因为在最后集成的时候，发现map没有办法获取当前的fd有哪些
	std::vector<int> m_onlineFd;
};


