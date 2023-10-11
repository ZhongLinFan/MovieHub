#include "ThreadPool.h"

ThreadPool::ThreadPool(EventLoop* mainLoop, int threadNums)
{
	m_threadNums = threadNums;
	m_pool = new Thread*[m_threadNums];
	m_isStart = false;
	m_index = 0;
	m_mainLoop = mainLoop;
	Debug("线程池初始化完成");
}

void ThreadPool::run()
{
	if (m_isStart == true) {
		return;
	}
	for (int i = 0; i < m_threadNums; i++) {
		m_pool[i] = new Thread(i);
		m_pool[i]->run(); 
	}
	m_isStart = true;
}

EventLoop* ThreadPool::takeEvLoop()
{
	//取出要操作的子线程
	if (m_threadNums == 0) {
		return m_mainLoop;
	}
	Thread* thread = m_pool[m_index];
	Debug("取出一个事件循环：第%d号", m_index);
	m_index = ++m_index % m_threadNums; // 更新下一个要取出的值
	return thread->m_evloop;
}

//这个方法的作用主要是为了了解特定进程的在线情况，有一点用，但是用处不大
EventLoop* ThreadPool::takeEvLoop(int index)
{
	//如果子线程个数为0，那么肯定返回的是主反应堆
	if (m_threadNums == 0) {
		return m_mainLoop;
	}
	if (index < 0 || index >= m_threadNums) { //说明该索引值有问题
		return nullptr;
	}
	return m_pool[m_index]->m_evloop;
}
