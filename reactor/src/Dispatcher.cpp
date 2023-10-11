#include "Dispatcher.h"
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h> //close
#include <assert.h>
#include "EventLoop.h"//cpp文件用到了里面的方法，所以需要h文件

Dispatcher::Dispatcher(EventLoop* evLoop):m_evLoop(evLoop) {}

Dispatcher::~Dispatcher()
{
}

Epoll::Epoll(EventLoop* evLoop):Dispatcher(evLoop){
	m_epfd = epoll_create(1);
	if (m_epfd == -1) {
		perror("epoll_create");
		exit(-1);
	}
	m_events = new struct epoll_event[m_maxSize];
	if (m_events == NULL) {
		perror("Epoll");
	}
	m_name = "Epoll";
}

void Epoll::epollCtl(int mode) {
	struct epoll_event ev;
	ev.data.fd = m_channel->m_fd;
	ev.events = 0; // 这一句话很关键
	if (m_channel->m_events & (int)FDEvent::ReadEvent) {
		Debug("正在给channel添加读事件");
		ev.events = EPOLLIN;
		//需要设置非阻塞，这样read时就可以返回了，要不然会一直阻塞在那
		int flag = fcntl(m_channel->m_fd, F_GETFL);
		flag |= O_NONBLOCK;
		fcntl(m_channel->m_fd, F_SETFL, flag);
	}
	if (m_channel->writeIsEnable()) { //使用上面的判断方法也是可以的
		ev.events |= EPOLLOUT;
	}
	int ret = -1;
	switch (mode) {
	case 1:ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_channel->m_fd, &ev); Debug("epoll成功添加读事件fd:%d", m_channel->m_fd); break;
	case 2:ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, m_channel->m_fd, &ev); Debug("epoll成功修改写事件：%d， fd:%d", m_channel->writeIsEnable(), m_channel->m_fd); break;
	default:break;
	}
	if (ret == -1) {
		printf("epoll_ctl\n");
		exit(0);
	}
}

bool Epoll::add() {
	epollCtl(1);
}

bool Epoll::remove() {
	//如果是删除，需要将fd从对应的map中删除
	//不对。。。这个函数应该只关心从树上删除的情况，其他集合的同步应该在EventLoop::addChannel和 EventLoop::deleteChannel进行同步
	//但是需要在这里调用接口，删除过程应该是一条线的，而不是这执行一下，那执行一下啊
	int ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, m_channel->m_fd, NULL);
	if (ret == -1) {
		printf("epoll_ctl\n");
		exit(0);
	}
	//删除所有的资源和对应关系
	//m_channel->destroyFunc(m_channel->m_args); 这是错误的
	m_channel->destroyCallBack(m_channel->m_args);
}

bool Epoll::modify() {
	epollCtl(2);
}

bool Epoll::wait() {
	Debug("epoll_wait开始等待");
	int nums = epoll_wait(m_epfd, m_events, m_maxSize, -1);
	if (nums == -1) {
		//在调试qt程序时会出现中断错误，进入到这里，导致无法正常进行，这里就屏蔽掉entir信号
		if (errno != EINTR)
		{
			perror("epoll_wait==-1");
			exit(0);
		}
		perror("epoll_wait==-1");
	}
	Debug("epoll_wait等待完成，epfd树%d有%d事件触发", m_epfd, nums);
	for (int i = 0; i < nums; i++) {
		Debug("epoll_wait正在处理第%d件事情", i);
		int fd = m_events[i].data.fd;
		int event = m_events[i].events;
		//如果客户端断开了连接，那么直接从epoll树上删除该节点
		if (event & EPOLLERR || event & EPOLLHUP) {
			//根据fd找到对应的channel
			//remove(eventLoop->cmap->list[fd], eventLoop);
			//水平触发未处理，可能会参数HUP事件
			//如果突然断开客户端会出现这种问题，但是这是加上路由机制之后才频繁发生的，之前没有出现过，在添加路由机制之后服务器还出现段错误的问题
			perror("epoll_wait错误,正在断开连接");
			m_evLoop->addTask(m_evLoop, m_evLoop->packageTask(m_evLoop->m_channelMap[fd], TaskType::DEL));
			continue;
		}
		if (event & EPOLLIN) {
			//读事件触发
			this->m_evLoop->activeEventProcess(fd, (int)FDEvent::ReadEvent);
		}
		if (event & EPOLLOUT) {
			//写事件触发
			m_evLoop->activeEventProcess(fd, (int)FDEvent::WriteEvent);
		}
	}
}

Epoll::~Epoll() {
	delete[] m_events;
	//后面两个很容易遗忘
	close(m_epfd);
}

Poll::Poll(EventLoop* evLoop):Dispatcher(evLoop) {
	//calloc会自动初始化，而malloc不会自动初始化
	m_fds = new struct pollfd[m_maxSize];
	for (int i = 0; i < m_maxSize; i++) {
		m_fds[i].fd = -1;
		m_fds[i].events = 0;
		m_fds[i].revents = 0;
	}
	m_maxfd = 1;  //这个初始化的时候一定要初始化1，肯定不能初始化为0或者最大值（如果最大值，那么maxfd全程不变），因为第一次的时候一定要保证能进去添加，那个时候也就更新了maxfd
	m_name = "Poll";
}


bool Poll::add() {
	Debug("poll树上开始挂fd, ThreadID:%ld", pthread_self());
	for (int i = 0; i < m_maxfd; i++) {
		if (m_fds[i].fd == -1) {
			m_fds[i].fd = m_channel->m_fd;
			m_maxfd = m_maxfd > m_channel->m_fd ? m_maxfd : m_channel->m_fd + 1;
			if (m_channel->m_events & (int)FDEvent::ReadEvent) {
				m_fds[i].events = POLLIN;
				//设置非阻塞模式
				int flag = fcntl(m_fds[i].fd, F_GETFL);
				flag |= O_NONBLOCK;
				fcntl(m_fds[i].fd, F_SETFL, flag);
				Debug("poll成功添加fd:%d", m_fds[i].fd);
			}
			if (m_channel->m_events & (int)FDEvent::WriteEvent) {
				m_fds[i].events |= POLLOUT;
			}
			break;
		}
	}
}

bool Poll::remove() {
	for (int i = 0; i < m_maxfd; i++) {
		if (m_fds[i].fd == m_channel->m_fd) {
			m_fds[i].fd = -1;
			//channel->fd可能不太正确,不过影响应该不大
			m_maxfd = m_maxfd == m_channel->m_fd + 1 ? m_channel->m_fd : m_maxfd;
			//删除所有的资源和对应关系
			m_channel->destroyCallBack(m_channel->m_args);
			break;
		}
	}
}

bool Poll::modify() {
	for (int i = 0; i < m_maxfd; i++) {
		if (m_fds[i].fd == m_channel->m_fd) {
#if 0
			//这种方法只能处理一个事件，不能完整的表达m_channel->m_events的意思
			if (m_channel->m_events & (int)FDEvent::ReadEvent) {
				m_fds[i].events = POLLIN;
			}
			if (m_channel->m_events & (int)FDEvent::WriteEvent) {
				Debug("正在修改Poll树上的fd：%d写事件", m_fds[i].fd);
				//注意这个加不加或都可以，因为我虽然我当前只检测写事件，但是当我响应完之后（也就是仅仅响应的过程我不检查读事件了，
				//如果加上或，那么响应的过程也检测读事件，就这么一点区别），我又立马改成了只检测读事件，
				m_fds[i].events |= POLLOUT;
			}
#else
			m_fds[i].events = 0; // 这一句很关键，因为不管之前是什么状态，我只认m_channel->m_events，否则比如我之前检测读事件，然后这次
			//不检测，那么下面的逻辑就清空不了
			if (m_channel->m_events & (int)FDEvent::ReadEvent) {
				m_fds[i].events = POLLIN;
			}
			if (m_channel->m_events & (int)FDEvent::WriteEvent) {
				Debug("正在修改Poll树上的fd：%d写事件", m_fds[i].fd);
				m_fds[i].events |= POLLOUT;
			}

#endif
			break;
		}
	}
}

bool Poll::wait() {
	Debug("evLoop:%d的poll开始等待事件发生", m_evLoop->m_seq);
	int ret = poll(m_fds, m_maxfd, -1);  //注意这里不能一直等待，如果一直等待，我在主线程中往子线程中放入cfd之后，虽然子线程处于监听状态，但是他不会检测新的节点，
	//有两个方法，一个是发射一个信号，另一个是不设置-1
	if (ret == -1) {
		printf("poll\n");
		exit(-1);
	}
	if (ret == 0) {
		return 0;
	}
	//perror("poll_wait");
	//Debug("evLoop:%d的poll等待完成，poll有%d事件触发, ThreadID:%ld", eventLoop->seq, ret, pthread_self());
	int count = 0;  //响应事件的次数
	for (int i = 0; i < m_maxfd; i++) {
		/*
		我这返回的事件集合中fd对应的值并不会为0，而是原来的值，只不过revents为0
		* 所以我这采用revents来判断
			if (data->fds[i].fd == -1) {
				continue;  // 如果fd为-1
			}
		*/
		Debug("正在遍历第%d个事件,对应的fd为:%d, events:%d, revents:%d", i, m_fds[i].fd, m_fds[i].events, m_fds[i].revents);
		//下面这样写是不对的，因为fds返回的描述符都是靠前的
		if (m_fds[i].revents == 0) {
			continue;  // revents
		}
		/*
		* 不需要在这里写，回调呀，事件触发，直接走之前注册好的回调
		if (data->fds[i].fd == eventLoop->pipefd[1]) {
			char buf[128];
			read(data->fds[i].fd, buf, 128); //如果不读由于是非阻塞，会一直返回
			Debug("已被唤醒,主线程说：%s", buf);
		}
		*/
		int fd = m_fds[i].fd;
		int event = m_fds[i].revents;
		//如果客户端断开了连接，那么直接从epoll树上删除该节点
		if (event & POLLERR || event & POLLHUP) {
			Debug("客户端断开了连接");
			//根据fd找到对应的channel
			//pollRemove(eventLoop->cmap->list[fd], eventLoop);
			continue;
		}
		if (event & POLLIN) {
			//读事件触发
			m_evLoop->activeEventProcess( fd, (int)FDEvent::ReadEvent);
		}
		if (event & POLLOUT) {
			//写事件触发
			Debug("写事件触发");
			m_evLoop->activeEventProcess(fd, (int)FDEvent::WriteEvent);
		}
		if (++count >= ret)break;
	}
}

Poll::~Poll() {
	delete[] m_fds;
}


Select::Select(EventLoop* evLoop):Dispatcher(evLoop) {
	m_maxfd = 0;
	//calloc会自动初始化，而malloc不会自动初始化
	m_readfds = new fd_set();
	m_writefds = new fd_set();
	FD_ZERO(m_readfds);
	FD_ZERO(m_writefds);
	m_name = "Select";
}

bool Select::add() {
	if (m_channel->m_events & (int)FDEvent::ReadEvent) {
		FD_SET(m_channel->m_fd, m_readfds);
		//需要设置非阻塞，这样read时就可以返回了，要不然会一直阻塞在那
		int flag = fcntl(m_channel->m_fd, F_GETFL);
		flag |= O_NONBLOCK;
		fcntl(m_channel->m_fd, F_SETFL, flag);
		Debug("selelct成功添加fd:%d", m_channel->m_fd);
	}
	if (m_channel->m_events & (int)FDEvent::WriteEvent) {
		FD_SET(m_channel->m_fd, m_writefds);
	}
	m_maxfd = m_maxfd > m_channel->m_fd ? m_maxfd : m_channel->m_fd;
}

bool Select::remove() {
	FD_CLR(m_channel->m_fd, m_readfds);
	FD_CLR(m_channel->m_fd, m_writefds);
	m_maxfd = m_maxfd > m_channel->m_fd ? m_maxfd : m_channel->m_fd - 1;
	//删除所有的资源和对应关系
	m_channel->destroyCallBack(m_channel->m_args);
}

bool Select::modify() {
	/*
		下面的方式不对，添加完写事件之后，删除不了
		if (m_channel->m_events & (int)FDEvent::ReadEvent) {
			FD_SET(m_channel->m_fd, m_readfds);
		}
		if (m_channel->m_events & (int)FDEvent::WriteEvent) {
			FD_SET(m_channel->m_fd, m_writefds);
		}
	*/
	if (~m_channel->m_events & (int)FDEvent::ReadEvent) { //没有读事件
		FD_CLR(m_channel->m_fd, m_readfds);
	}
	else {
		FD_SET(m_channel->m_fd, m_readfds);
	}
	if (~m_channel->m_events & (int)FDEvent::WriteEvent) { //没有写事件，也可以用channel提供的函数, 
		Debug("正在清除fd：%d的写事件", m_channel->m_fd);
		FD_CLR(m_channel->m_fd, m_writefds);
	}
	else {
		Debug("正在添加fd：%d的写事件", m_channel->m_fd);
		//FD_SET(m_channel->m_fd, m_readfds);，这样肯定不行呀。。。。添加到了读集合中了。。。。
		FD_SET(m_channel->m_fd, m_writefds);
	}
	m_maxfd = m_maxfd > m_channel->m_fd ? m_maxfd : m_channel->m_fd;
}

bool Select::wait() {
	fd_set rdtemp = *m_readfds;
	fd_set wdtemp = *m_writefds;
	Debug("select正在等待，threadID:%ld", pthread_self());
	//一个很狗血的是这里一直等不到，检查了好几遍，通过wireshark抓包，发现是浏览器地址填错了。。。。
	int ret = select(m_maxfd + 1, &rdtemp, &wdtemp, NULL, NULL);
	if (ret == -1) {
		perror("select");
		exit(0);
	}
	perror("select");
	Debug("select等待完成,有%d个事件产生，threadID, %ld", ret);
	//这个要是maxfd + 1要不然最大的那个文件描述符处理不了，如果是一开始，那么主线程唤醒不了子线程
	for (int i = 0; i < m_maxfd + 1; i++) {
		//异常的未检测
		if (FD_ISSET(i, &rdtemp)) {
			//读事件触发
			Debug("Select正在响应读事件");
			m_evLoop->activeEventProcess(i, (int)FDEvent::ReadEvent);
		}
		if (FD_ISSET(i, &wdtemp)) {
			//写事件触发
			Debug("Select正在响应写事件");
			m_evLoop->activeEventProcess(i, (int)FDEvent::WriteEvent);
		}
	}
}

Select::~Select() {
	delete[] m_readfds;
	delete[] m_readfds;;
}
