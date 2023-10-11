#pragma once
//#include "EventLoop.h"
//可以用命名空间，但是如果是命名空间，那么Task中就需要两个联合体，而如果是类只需要一个联合体
//不行即使是类也需要一个联合体
//最终还是设计成这样把，因为发现udp读包以及这个evloop是针对channel设计的，整个运行状态都是基于channel
//所以udp还是单独弄一个项目吧，嵌套在这里面不合适
//过了几天之后发现udp完全可以用channel，channel就是对fd的一个封装，只不过message那块需要微调即可，所以最终还是决定将udp封装在这里

enum class TaskType :int {
	ADD = 0x01,
	DEL = 0x02,
	MODI = 0x04,
	NORMAL = 0x08
};

class EventLoop;  //这里用到了他的指针，而evloop用到了实体，所以这里可以前向声明一下，而不需要上面的include
using taskCallBack = std::function<void(EventLoop* evLoop, void* args)>;
struct Task {
	Task() {}
	~Task() {}
	TaskType type;
	union {
		Channel* channel;  //关于channel的任务
		struct {
			//void (*taskCallBack)(EventLoop* evLoop, void* args);
			taskCallBack call;  //普通任务
			void* args;
		}body;
	};
};