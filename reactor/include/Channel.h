#pragma once
#include <functional>

//这个模块用到的是using 包装器 强类型枚举，static_cast<int>
//using handle = void(*)(void* arg);


enum class FDEvent {
	Nothing = 0x00,  //为了写客户端增加的  //但是这个没用 ，一开始啥都没有根本添加不上去, 但是是在new的时候传递了这个值，先不添加，所以还是用到了这个值
	ReadEvent = 0x01,
	WriteEvent = 0x02,  //为什么这里用16进制而不是2进制，因为16进制跟专业一些，0x02和2是没有区别的，不过如果事件多了一些之后，比如再来一个事件吗，那么 是0x04
						//而不是0x03，但是用十进制表示就没那么直观了，并且我本意是为了使用一个位表示事件，如果赋值用二进制太麻烦，16进制刚好可以方便的表示二进制
};

class Channel {
public:
	//如果不加c++11进行编译，会报很奇怪的问题
	using handleFunc = std::function<int(void*)>;
	//初始化函数
	Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc,  void* args);  //这里我忘记改成FDEvent
	//修改fd的写事件（检测or不检测）（读事件肯定是不能去掉的）
	void writeEnable(bool mode);
	//为了客户端非阻塞，需要检测写事件，这里增加一个修改读事件的方法
	void readEnable(bool mode);
	//判断是否检测了写事件
	bool writeIsEnable();
	//判断是否检测了读事件，这个是为了在modifyChannel()时先验证事件的合理性，如果没有这两个事件其中一个，那么这个channel就不会产生任何事件
	bool readIsEnable();
	//释放channel
	~Channel();

	//增加一个修改写事件的回调函数的接口，因为客户端（非阻塞的情况下）在建立的过程中需要更改一下写事件的回调逻辑
	bool changeWriteCallBack(handleFunc writeFunc, void* args);
public:
	//文件描述符
	int m_fd;
	//事件
	int m_events;
	//读写回调函数
	handleFunc readCallBack;
	handleFunc writeCallBack;
	//销毁函数，这个是在从检测树上摘下，立马调用的
	handleFunc destroyCallBack;
	//读写回调函数的参数
	void* m_args;
};

