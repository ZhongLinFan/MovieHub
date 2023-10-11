#pragma once
#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Log.h"

//忘记析构了。。。

class Thread {
public:
	Thread(int index); //index主要是为了给线程起名字，index代表是第几个线程
	~Thread();
	void run();
	//线程工作的函数
	void working();
public:
	EventLoop* m_evloop;
	std::thread*  m_worker; //线程对象
	std::thread::id m_threadID;  //很不熟悉
	int m_seq;
	std::mutex m_mutex;
	std::condition_variable m_cond;
};


