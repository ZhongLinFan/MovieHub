#pragma once
#include "MediaRoom.h"  //必须要放在这里，最上面，否则会报错
#include <string>
#include <list>
#include <thread>
#include "Udp.h"
#include "TcpClient.h"

class MediaServer {
public:
	MediaServer(std::string mediaServerIp, unsigned short mediaServerPort, std::string lbServerIp, unsigned short lbServerPort, 
		std::string mysqlServerIp, unsigned short mysqlServerPort);
	//启动函数
	void run();
	//创建房间的第一步
	void createRoom(Udp* lbClient, void* userData);
	//创建房间的第二步
	void createRoom2(NetConnection* mysqlClient, void* userData);
	//销毁房间
	void destroyRoom();
	//获取fid的路径的函数
	void getFidPath();
	//rtp需要访问sendMsg
public:
	//有重复代码的辅助函数
	// 注意模板函数在这里实现，可以保证未定义不会出现
	//由于UdpReply可能需要设置地址，而conn没有m_addr，所以需要对packageAndResponse进行一层包装
	//udp回复消息给某个主机void UdpReply(int msgid, std::string& data, sockaddr_in* to = nullptr);
	template<class Response_T>
	void udpSendMsg(int responseID, Response_T* responseData, sockaddr_in* to) {
		//如果to为nullptr，那么就默认发送给发过来的消息的客户端，如果不为空，那么就发给指定的客户端
		//设置发送地址
		//可以不用设置，因为一个包一个包的接收，那么当前地址在处理这个包之前就不会丢失
		//不对必须要设置，因为如果同时来两个包，（虽然这两个包他们是顺序处理的）但是后一个包的地址已经覆盖了前一个包的地址
		// 前一个包回复给了最后一个接收包的地址，所以每个包请求必须自带地址
		//不对，因为读到rbuff，每次只能读一个包，所以读完之后，立马处理了（不管这个包有几个请求，这个包都会处理完），然后再去readtoBuf，相当于再去处理下一个客户端
		// 那么就可以不设置读字符串，当然了，如果是消息转发，肯定还要设置的
		packageMsg<Udp, Response_T>(streamServer, responseID, responseData);
		streamServer->sendMsg(*to);
	}

	//由于Udp底层更改了地址的逻辑，导致sendMsg不得不有一个地址参数，所以不得不给TCP弄一个函数，将原来的打包发送函数改为打包函数
	//公共发送函数
	template<class Response_T>
	void tcpSendMsg(NetConnection* conn, int responseID, Response_T* responseData) {
		packageMsg(conn, responseID, responseData);
		//发送数据到对端
		conn->sendMsg();
	}

	//公共打包函数
	template<class Conn_T, class Response_T>
	void packageMsg(Conn_T* conn, int responseID, Response_T* responseData) {
		Debug("requestHandle开始回复数据");
		//responseData的结果序列化
		std::string responseSerial;  //注意我让conn的response的data指针指向responseSerial是完全没问题的，因为sendMsg函数整个过程，responseData都是有效的，因为这个函数还没执行完
		responseData->SerializeToString(&responseSerial);
		//封装response
		auto response = conn->m_response;
		response->m_msgid = responseID;
		response->m_data = &responseSerial[0];
		response->m_msglen = responseSerial.size();
		response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
	}
public:
	//负责发送实际数据,	//并且负责和lb服务器通信，因为如果再增加一个UdpClient，那么lb服务器如果想要和media服务器通信，
	//就要记住这个UdpClient的地址，显然很麻烦，现在直接这样通信吧
	UdpServer* streamServer;  
	//还需要增加一个mysql客户端，因为获取路径，以及客户端如果获取最新电影，都需要从mysql拉取
	TcpClient* mysqlClient;
	std::list<MediaRoom> room;					
};