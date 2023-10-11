#include "EventLoop.h"
#include <assert.h>
#include <unistd.h> //pipe
#include <functional> //bind
#include <string>
#include "Dispatcher.h"


EventLoop::EventLoop(std::thread::id threadID, int seq)
{
	Debug("开始初始化evLoop");
	m_isQuit = true;   //可以设置一个退出函数TODO
	m_dispatcher = new Epoll(this);  // 每一个m_dispatcher都绑定了一个EventLoop，注意这个创建的过程虽然时主线程执行的，但是this依然时子反应堆
	//下面是非常低级的错误
	// vector的使用：https://blog.csdn.net/icecreamTong/article/details/127933977
	//m_channelMap = std::map<int, Channel*>();
	//m_tasks = std::vector<int>();
	m_threadID = threadID;
	m_seq = seq;
	//初始化一个管道
	if (pipe(m_pipefd) == -1) {
		perror("pipe");
	}
	//将自己的文件描述符包装成channel，放到待检测集合中
	//evloop->pipefd[0]为读端
	auto wakeUp = std::bind(&EventLoop::readMyData, this);
	m_myChannel =  new Channel(m_pipefd[0], FDEvent::ReadEvent, wakeUp, nullptr, nullptr, nullptr);
	addTask(this, packageTask(m_myChannel, TaskType::ADD));
	//evloop->dispatcher->add(evloop->myChannel, evloop); //添加到待检测集合中,没有通过任务添加是因为事件循环还没启动
	//也可以添加到事件循环，等到启动肯定会自己去处理了
	Debug("evLoop初始化完成");
}

EventLoop::~EventLoop()
{
	delete m_dispatcher;
	delete m_myChannel;
}

void EventLoop::run()
{
	if (m_isQuit) {
		m_isQuit = false;
		while (!m_isQuit) {
			m_dispatcher->wait();
		}
	}
}

void EventLoop::activeEventProcess(int fd, int event)
{
	if (fd > 0) {
		//处理的函数都封装在了channel里了，先取出来
		if (m_channelMap.find(fd) == m_channelMap.end()) {
			//说明当前连接已被销毁，直接返回
			return;
		}
		Channel* channel = m_channelMap[fd];
		assert(channel->m_fd == fd);
		//读事件
		if (event & (int)FDEvent::ReadEvent && channel->readCallBack) {
			channel->readCallBack(channel->m_args);
			Debug("fd：%d的读事件响应完成", fd);
		}
		//写事件
		if (event & (int)FDEvent::WriteEvent && channel->readCallBack) {
			channel->writeCallBack(channel->m_args);
			Debug("fd：%d的写事件响应完成", fd);
		}
	}
}

void EventLoop::addTask(EventLoop* evloop, Task* task)
{
	assert(task);
	//由于主子线程可能都会访问任务队列，所以需要加锁
	//Debug("addTask加锁");
	m_mutex.lock();
	m_tasks.push(task);
	m_mutex.unlock();
	if (m_threadID == std::this_thread::get_id()) {
		tasksProcess();
	}
	else {
		//子线程干的事情
		Debug("正在唤醒子线程");
		wakeupThread(evloop);  //不是this
	}
}

void EventLoop::wakeupThread(EventLoop* evloop)
{
	assert(evloop);
	std::string buf = "该起来干活了";
	//父进程向写端中写数据
	write(evloop->m_pipefd[1], buf.c_str(), buf.size());
}

int EventLoop::readMyData()
{
	char data[128] = { 0 }; //如果不初始化，会输出乱码
	read(m_pipefd[0], data, sizeof(data)); ////如果不读由于是非阻塞，会一直返回
	Debug("已被唤醒,主线程说：%s", data);
	tasksProcess();
}

void EventLoop::tasksProcess()
{
	Debug("正在处理任务,当前一共有%d个任务", m_tasks.size());
	while (!m_tasks.empty()) {
		//添加检测描述符
		m_mutex.lock();
		auto task = m_tasks.front();
		m_tasks.pop();
		m_mutex.unlock();
		if (task->type == TaskType::ADD) {
			Debug("正在添加fd：%d到反应堆：%d", task->channel->m_fd, m_seq);
			addChannel(task->channel);
		}
		//删除检测的描述符
		else if (task->type == TaskType::DEL) {
			Debug("正在从反应堆%d，删除fd：%d", m_seq, task->channel->m_fd);
			deleteChannel(task->channel);
		}
		//修改检测的描述符号
		else if (task->type == TaskType::MODI) {
			modifyChannel(task->channel);
		}
		else if (task->type == TaskType::NORMAL) {
			if (task->body.call == nullptr) {
				std::cout << "任务异常，已忽略" << std::endl;
			}
			else {
				task->body.call(this, task->body.args);  //注意 这个this很重要
				//如果一个任务还往这个任务队列里放任务，那么就会导致某一个任务多次被执行，最终delete的时候出问题，可以先pop
			}
		}
		else {
			std::cout << "未知任务" << std::endl;
		}
		//这里是在操作共享资源，需要加锁
		
		delete task;
		Debug("任务处理成功");
	}
}

bool EventLoop::addChannel(Channel* channel)
{
	//添加一个fd到对应的channel的时候
	//需要维护的关系表:TCPServer的m_Fd_TcpConn;evloop的m_channelMap;
	// 新增的资源：fd，TCPConnect，channel，读buf，写buf
	//先处理ChannelMap
	assert(m_channelMap.find(channel->m_fd) == m_channelMap.end());  //发现不是NULL，是因为任务队列的p忘记往后更新了，以及忘记更新head的指向了
	m_channelMap[channel->m_fd] = channel;
	//注意还要添加到其他地方
	//然后添加到检测集合中
	m_dispatcher->setChannel(channel);
	m_dispatcher->add();
	return true;
}

bool EventLoop::deleteChannel(Channel* channel)
{
	//在这释放关系并不行，因为客户端也要走这一条路，还是应该放在各自的destroy函数里进行处理
	//删除需要按照上面释放的资源进行删除
	//m_channelMap[channel->m_fd] = 0;
	//Debug("正在释放关系的fd为：%d", channel->m_fd);
	//m_channelMap.erase(channel->m_fd);
	//删除TCPServer的对应关系

	//TcpServer::delFd_TcpConn(channel->m_fd);

	//处理检测集合
	m_dispatcher->setChannel(channel);
	m_dispatcher->remove();
	//destroyChannel(channel);
	//经过洗礼，发现在这删除关于这个TCPCONNECTION是成熟的选择
	return true;
}

bool EventLoop::modifyChannel(Channel* channel)
{
	//先判断ChannelMap的合理性
	assert(m_channelMap.find(channel->m_fd) != m_channelMap.end());
	//注意这里需要检验一下事件是不是0，也就是任何事件都没有，如果是的话，那么修改之后不会检查任何事件了，那么永远不会有响应
	assert(channel->writeIsEnable() || channel->readIsEnable());
	m_dispatcher->setChannel(channel);
	m_dispatcher->modify();
	return true;
}

Task* EventLoop::packageTask(Channel* channel, TaskType mode) {
	assert(channel);
	Task* task = new Task();
	task->channel = channel;
	task->type = mode;
	return task;
}
Task* EventLoop::packageTask(taskCallBack task, void* args, TaskType mode) {
	Task* tsk = new Task();
	tsk->type = mode;
	tsk->body.call = task;
	tsk->body.args = args;
	return tsk;
}