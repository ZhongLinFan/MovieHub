#pragma once
//#include "EventLoop.h"没用到为啥加
#include "Log.h"
#include "NetConnection.h"
#include "Message.h"  这个也没用到呀   //其他模块需要用到这个里面的方法，所以需要提供
//用到了TcpMsgRouter类，需要包含头文件，这是后来加的
#include "MsgRouter.h"

//其实这就相当于一个单独的TcpConncetion，为什么要设计成非阻塞形式呢，因为如果是阻塞的话，那么read，就会阻塞在那，如果我想要处理多个业务
//就必须让epoll告诉我，啥时候来消息了，所以这个客户端的机制（处理流程）和一个TcpConncetion是一样的，只不过回调的业务函数不一样

//那为啥不能用TcpConncetion类呢，确实很像，可以把他们的共同部分抽象出来，然后client和TcpConnection继承
//需要线程池吗？？需要吧，因为如果当阻塞在wait上时，其他线程可以处理其他业务
class TcpClient : public NetConnection{
public:
	//业务层需要一个无参的构造函数，这里提供一下
	TcpClient();
	TcpClient(const char* ip, short port);  //初始化客户端
	bool connectServer(); //连接服务器
	bool connected();  //非阻塞模式通过触发写回调判断是否建立连接的回调函数

	//注册路由函数
	bool addMsgRouter(int msgid, TcpMsgRouter::msgCallBack callBcak, void* userData = nullptr);

	//设置hook函数
	inline bool setConnStart(NetConnection::hookCallBack callBack, void* args=nullptr) {
		connOnStart = callBack;
		connOnStartArgs = args;
	}
	inline bool setConnLost(NetConnection::hookCallBack callBack, void* args=nullptr) {
		connOnLost = callBack;
		connOnLostArgs = args;
	}
	
	virtual bool destroy();
	//对外封装启动函数
	void run();
	//改变服务器地址，业务层需要
	void changeServer(const char* ip, short port);
	//避免冗余代码
	void setSockAddr(sockaddr_in* addr, const char* ip, short port);
public:
	//char* msg[] = { {"123456578909076588958695756097560978560978000679567094569456094569546054"}, {"435645"}, {"43"}, {"lalaalallalalalallalalalalalal"}, {"你好世界，我爱你"}, {"那为啥不能用TcpConncetion类呢，确实很像，可以把他们的共同部分抽象出来，然后client和TcpConnection继承"}, {"1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是1234734958340958的房间客人TV是"}, {"12893849865469075675894568"}, {"3"}, {"3422"}, {"说明建立连接成功，关闭写事件，添加读事件检测吗"} };
	//上面写是错误的，应该是 char* msg[] = {"ret", "erf", "fg"};即使这样也是不对的char* msg[20]...不知道为啥
	//TcpMsgRouter msgRouter; //客户端因为只有一个，可以不用静态，如果把他放到抽象类中，也是可以的,在父类声明一个指针，为了和服务器配套

	NetConnection::hookCallBack connOnStart;
	NetConnection::hookCallBack connOnLost;
	void* connOnStartArgs;
	void* connOnLostArgs;

public:
	bool m_connected;
};