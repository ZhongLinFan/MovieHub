#pragma once
#include <stdio.h>
#include<stdarg.h>  //不是这个头文件，在c++种会报错
#include <iostream> //c++种对线程id的打印使用cout才能简单打印，否则很困难
#include <mutex>
#include <thread>
#include <arpa/inet.h> //sockaddr和sockaddrin的结构体

#define DEBUG 1
/*
	参考网页：
	https://blog.csdn.net/dldw8816/article/details/86519575/ do{}while(0)
	百度关键词##arg  LOG(type, fmt, args...)  （日志系统 c语言 宏函数）（日志 do whille(0)）
	https://blog.csdn.net/qq_32938605/article/details/98179228	使用(fmt,##args) 自定义日志
	https://blog.csdn.net/weixin_41903639/article/details/124389662  宏定义中的##作用
	https://blog.csdn.net/u014436243/article/details/107386167  C使用宏定义封装printf实现日志功能  多级日志
*/
#if DEBUG
	
#define LOG(type, fmt, args...)	\
	do{\
		std::mutex mutex;\
		mutex.lock();\
		std::cout<<"threadID:"<<std::this_thread::get_id(); \
		printf("%s:%s->%s->line:%d=[",type,  __FILE__, __FUNCTION__, __LINE__);\
		printf(fmt, ##args);\
		printf("]\n");\
		mutex.unlock();\
	} while (0)
#define Debug(fmt, args...) LOG("DEBUG",fmt, ##args)
#define Error(fmt, args...) do{LOG("ERROR",fmt, ##args);exit(0);}while(0)
#else
#define LOG(fmt, args...)
#define Debug(fmt, args...)
#define Error(fmt, args...)
#endif
