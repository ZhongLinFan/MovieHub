#pragma once
#include "Thread.h"
#include "EventLoop.h"
#include "Log.h"

class ThreadPool {
public:
	//线程池初始化
	ThreadPool(EventLoop* mainLoop, int threadNums);
	//启动线程池
	void run();
	//从线程池中的某个子线程取出evloop模型， 因为启动之后肯定要往子线程的evloop放任务
	EventLoop* takeEvLoop();
	EventLoop* takeEvLoop(int index); //根据编号取出对应的evLoop
public:
	bool m_isStart;
	int m_index;
	int m_threadNums;
	Thread** m_pool; //里面存放的是线程结构体的地址,
	EventLoop* m_mainLoop; //主线程的事件循环
};
