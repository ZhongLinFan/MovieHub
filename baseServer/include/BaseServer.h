#pragma once
#include "baseService.pb.h"
#include "mysqlService.pb.h"
#include "lbService.pb.h"
#include "TcpServer.h"
#include "TcpClient.h"
#include "Udp.h"
#include <typeindex>

//conn属性的连接属性指向的结构体，当前连接对应的用户信息
struct UserData
{
public:
	UserData(int uid) {
		m_uid = uid;
	}
	int m_uid;
};

class TypeIdentifier {
public:
	TypeIdentifier() {
		m_typeIdentifier.clear();  //甚至这一步都不需要做
	}

	//获得RecvId
	template <typename type>
	int getMsgId() {
		//注意type_index对象是基于typeid(T)返回的type_info引用，然后进行匿名构造产生的type_index对象，然后find会进行比较（type_index里面有==）
		//当然也可以显示的type_index（typeid(T)）也行
		//但是type_index是个匿名对象，这就牵扯到m_typeIdentifier[]时插入的是副本，还是这个值本身了
		//不用担心，找到了这个https://noerror.net/zh/cpp/container/unordered_map/operator_at.html，里面有T& operator[]( const Key& key );
		//和T& operator[]( Key&& key );
		auto it = m_typeIdentifier.find(typeid(type));
		if (it != m_typeIdentifier.end()) {
			return it->second;  //迭代器要使用->，这都忘记了。。。
		}
		return -1; //不存在就返回-1
	}

	//设置标志值
	//不过有些函数在收到消息后是不需要发送消息的，所以sendId可以为一个特定值-1，代表sendId不存在
	template <typename type>
	bool setIdentifier(int msgId) {
		auto it = m_typeIdentifier.find(typeid(type));
		if (it != m_typeIdentifier.end()) {
			std::cout << "已经为该类型设置标志" << std::endl;
			return false;
		}
		//将该类型和对应关系记录下来
		m_typeIdentifier[typeid(type)] = msgId;  //注意typeid(T)
		std::cout <<"已记录，"<< "msgId：" << m_typeIdentifier[typeid(type)] << std::endl;
		//为该类型注册函数
		//这个注册逻辑只能在外面操作，因为你根本不知道函数是谁，难道还要传进来？
		return true;
	}
private:
	//注意，unordered_map本身是需要提供hash函数和operator==来工作的，但是std::type_index这个类本身有operator==和hash_code函数，所以可以显示指定
	//这个是参考了网上的unordered_map的使用案例（两种方式指定那两个函数）和cppreference.com里关于type_index的使用总结出来的
	//注意这里为啥使用unordered_map，因为查询时o(1)而map是o(logn)，还有就是之前已经使用过map了，这里想体验一下这个函数
	//type_index是对type_info的一层包装，而typeid返回的是type_info的对象
	std::unordered_map<std::type_index, int> m_typeIdentifier;
};

class BaseServer {
public:
	BaseServer(const char* ip, short port);
	~BaseServer();
	//启动函数
	void run();
	//处理登录请求步骤1
	void handleLoginStep1(NetConnection* clientConn, void* userData);
	//处理登录请求步骤2
	void handleLoginStep2(NetConnection* conn, void* userData);
	//喜欢列表响应
	void handleFavoriteList(NetConnection* mysqlClient, void* userData);
	//朋友列表响应
	void handleFriendList(NetConnection* mysqlClient, void* userData);
	//组列表响应
	void handleGroupList(NetConnection* mysqlClient, void* userData);
	//喜欢列表请求，朋友列表请求，组列表请求
	template<class Request_T>
	void getUserInfoList(int uid, int requestId);
	//处理消息发送请求
	void handleSendMsgRequest(NetConnection* conn, void* userData);
	//处理消息发送响应
	void handleRepostMsgResponse(Udp* lbClient, void* userData);
	//处理消息通知请求
	void handleMsgNotify(Udp* lbClient, void* userData);
	//处理注册账号请求
	void handleRegister(NetConnection* conn, void* userData);
	//注册响应，一定要有这个的，因为客户端最终需要获得一个uid，就相当于一个qq号
	void handleRegisterUserResult(NetConnection* conn, void* userData);
	//处理添加朋友，组操作
	void handleAddRelations(NetConnection* conn, void* userData);
	// 其实还应该增加一个下线同步机制，这就需要增加一些协议
	void handleLogout(NetConnection* conn, void* userData);
	//还应该添加喜欢操作
	void handleAddFavorite(NetConnection* conn, void* userData);
private:
	//公共修改用户状态函数
	void modifyUserStatus(int uid, int status);
	// 公共解析函数
	template<class Conn_T, class Request_T>
	std::shared_ptr<Request_T> parseRequest(Conn_T* conn);
	//公共打包函数
	template<class Conn_T, class Response_T>
	bool packageMsg(Conn_T* conn, Response_T* responseData, int responseID);
	//udp发送函数
	template<class Response_T>
	void udpSendMsg(Response_T* responseData, sockaddr_in* to, int responseID=-1);
	//tcp发送函数
	template<class Response_T>
	void tcpSendMsg(NetConnection* conn, Response_T* responseData, int responseID=-1);
	//注册记录函数模板
	template<class Server_T, class Conn_T, class... Args>
	bool addHandleFunc(Server_T* server, void(BaseServer::* func)(Conn_T*, void*), std::initializer_list<int> msgIdList);
	//下面两个都是注册记录模板辅助函数
	template<class MsgType>
	bool recordMsgType(int index, std::initializer_list<int> msgIdList);
	template<class Server_T, class Conn_T, class MsgType>
	bool recordMsgType(Server_T* server, void(BaseServer::* func)(Conn_T*, void*), int index, std::initializer_list<int> msgIdList);
private:
	TcpServer* tcpServer;  //主服务器，客户端连接的主服务器
	TcpClient* mysqlClient;  //负责和mysql连接的服务器
	//TcpClient* lbClient;  //负责和负载均衡服务器连接，交互数据
	//改成udp和负载均衡服务器通信把，因为如果这里用tcp，那么负载均衡服务器也要改，用udp就可以直接用了，后面为了保证数据，有时间再改成tcp通信
	UdpClient* lbClient;  //和负载均衡服务器通信
	std::map<int, NetConnection*> uid_conn;
	TypeIdentifier* msgIdRecoder;
};