#include "Thread.h"


Thread::Thread(int index)
{
	m_seq = index;
	m_evloop = nullptr;
	m_worker = nullptr;  //不要使用NULL
}

Thread::~Thread()
{
	//这个是不会被调用的，因为线程池在程序关闭之前都是存在的
	delete m_worker;
}

void Thread::run()
{
	//这里一定要使用std::thread(&Thread::working,否则会有两个错误
	m_worker = new std::thread(&Thread::working, this); //working是隐藏有一个this参数的，所以这里要传入一个this参数，this是这个函数的所有者
	//有以下理解：
	//当前run肯定是主线程执行的，当主线程执行完上述代码，会认为thread可以正常工作，（里面的属性都初始化完成了等）
	//但是当前函数的thread执行完，其实只是开辟了一块空间，然后设置这个新线程的执行函数是working，设置完之后可能，主线程就已经使用新线程的EventLoop了，
	//因为他的时间片还没结束，可以执行新的逻辑，但是实际上当前新线程还没有机会抢到时间片去执行，去初始化EventLoop，所以需要保证EventLoop初始化完成，这个run函数才能退出
	//这个时候就需要条件变量了，条件变量需要搭配互斥锁使用（因为主线程和子线程都访问了EventLoop）
	//那么同步的过程大概是主线程执行完thread先等待EventLoop执行完毕，因为我看有没有执行完毕需要访问EventLoop，所以这里访问的时候加锁和解锁，
	//同样的，子线程也在相应共享位置加锁和解锁，不过子线程初始化完成之后，需要唤醒主线程，让当前函数处理完毕，然后去执行其他的逻辑
	//这里为啥用unique_lock<mutex> locker(m_mutex);是因为m_cond需要使用封装后的互斥锁，unique_lock<mutex> locker(m_mutex);这个相当于锁上了
	std::unique_lock<std::mutex> locker(m_mutex);
	//这里为啥用while，用if其实也对，因为只有一个主线程，但是加上while也是正确的，也是同步变量的一般写法
	while (m_evloop == nullptr) {
		m_cond.wait(locker);
	}
	//unique_lock<mutex> locker(m_mutex); （这个是加锁的）//解锁可加可不加，因为locker是个局部对象，当结束生命周期会自动解开
}

void Thread::working()
{
	Debug("threadheadle开始工作");
	m_threadID = std::this_thread::get_id();
	//和主线程访问共享资源
	m_mutex.lock();
	m_evloop = new EventLoop(m_threadID, m_seq);
	m_mutex.unlock();
	//初始化完毕唤醒阻塞在条件变量上的某一个线程
	m_cond.notify_one();
	m_evloop->run();
	Debug("threadheadle工作完成");
}


