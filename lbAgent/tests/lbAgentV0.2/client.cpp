#include "lbService.pb.h"
#include "mysqlService.pb.h"
#include "Udp.h"
#include "TcpClient.h"
#include <thread>

//默认是测试返回一个基础ip的
#define SYSAcceptResultTest   //如果这个打开就增加了处理基础服务器的反馈响应测试
#define RepostMsgRequestTest   //如果这个打开就增加了基础服务器消息转发请求的测试

void simulateGetBaseServer(Udp* host, int uid, void* userData);
void simulateRepostMsgRequest(Udp* udp, int uid, void* userData);

//公共打包函数
template<class Conn_T, class Response_T>
void packageMsg(Conn_T* conn, int responseID, Response_T* responseData)
{
	std::string responseSerial;  
	responseData->SerializeToString(&responseSerial);
	//封装response
	auto response = conn->m_response;
	response->m_msgid = responseID;
	response->m_data = &responseSerial[0];
	response->m_msglen = responseSerial.size();
	response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
}

//由于UdpReply可能需要设置地址，而conn没有m_addr，所以需要对packageAndResponse进行一层包装
template<class Response_T>
void udpSendMsg(Udp* udp, int responseID, Response_T* responseData, sockaddr_in* to)
{
	packageMsg<Udp, Response_T>(udp, responseID, responseData);
	udp->sendMsg(*to);
}

//由于Udp底层更改了地址的逻辑，导致sendMsg不得不有一个地址参数，所以不得不给TCP弄一个函数，将原来的打包发送函数改为打包函数
//公共发送函数
template<class Response_T>
void tcpSendMsg(NetConnection* conn, int responseID, Response_T* responseData)
{
	packageMsg(conn, responseID, responseData);
	//发送数据到对端
	conn->sendMsg();
}

//请求一个基础服务器地址或者流媒体服务器地址
void GetServerIp(Udp* udp, int mode, int uid) {  //1代表请求基础ip，2代表请求流媒体ip
	lbService::GetServerRequest responseData;
	responseData.set_modid(mode);  //1代表请求基础ip，2代表请求流媒体ip
	responseData.set_id(uid);  //客户uid
	udpSendMsg<lbService::GetServerRequest>(udp, (int)lbService::ID_GetServerRequest, &responseData, &udp->m_recvAddr);
	Debug("GetServerIp执行到这");
}


//得到基地址之后的处理动作//得到流媒体服务器地址的处理动作
//using msgCallBack = std::function<void(Udp* host, void* userData)>;
void HandleServerIp(Udp* host, void* userData) {
	lbService::GetServerResponse responseData;
	responseData.ParseFromArray(host->m_request->m_data, host->m_request->m_msglen);

	int responseType = responseData.modid();
	Debug("requestType:%d", responseType);
	switch (responseType) {
	case 1:  //得到基地址
		std::cout << "基地址:" << std::endl;
		for (int i = 0; i < responseData.host_size(); i++) {
			std::cout << "ip:" << responseData.host(i).ip()
				<< "\tport:" << responseData.host(i).port() << std::endl;
		}
#ifdef SYSAcceptResultTest
		simulateGetBaseServer(host, responseData.uid(), userData);
#endif // !SYSAcceptResultTest
		break;
	case 2:// //得到流媒体地址
		std::cout << "流媒体地址:" << std::endl;
		for (int i = 0; i < responseData.host_size(); i++) {
			std::cout << "ip:" << responseData.host(i).ip()
				<< "\tport:" << responseData.host(i).port() << std::endl;
		}
		break;
	}
}

//模拟服务器端发送登录情况的包
void simulateGetBaseServer(Udp* udp, int uid, void* userData) {
	//注意，在模拟各个模块之前，需要重启lb服务器，因为这个lb服务器可能已经包含这个9527这个用户了
	lbService::ServerResonseToLb responseDataToLb;  //给lb服务器回复的消息
	//先假设是错误的，然后如果正确，再更改
	responseDataToLb.set_modid(1);
	responseDataToLb.set_originid(uid);
	responseDataToLb.set_finalid(uid);
	responseDataToLb.set_result(-1);
#if 0
	//模拟登录失败的情况
	//这里不用任何处理
#elif 1
	//模拟登录成功的情况
	responseDataToLb.set_result(1);
#elif 0
	//模拟注册成功的情况
	responseDataToLb.set_finalid(6);
	responseDataToLb.set_result(1);
#else
	//模拟注册失败的情况
	responseDataToLb.set_finalid(6);
	responseDataToLb.set_result(-1);
#endif
	udpSendMsg<lbService::ServerResonseToLb>(udp, (int)lbService::ID_ServerResonseToLb, &responseDataToLb, &udp->m_recvAddr);
	//在这里调用是没有问题的，因为即使两个包黏在一起，在处理第一个包的时候就已经更新了user_serverMap了，在处理第二个包的时候就正常处理了
	
#ifdef RepostMsgRequestTest
	simulateRepostMsgRequest(udp, uid, userData);
#endif
}

//模拟服务器转发消息请求的包
void simulateRepostMsgRequest(Udp* udp, int uid, void* userData) {
	//先往数据库修改登录状态，因为lbServer会获取组成员在线的人员名单
	//这个模拟过程有点繁琐，这里说明一下
	//1、首先需要模拟三个用户登录成功（这三个用户必须是同一个群聊的）√
	//2、然后将mysql的这三个用户的状态修改在线  √
	TcpClient* mysqlClient = (TcpClient*)userData;
	mysqlService::ModifyUserRequest ModifyUserData;
	ModifyUserData.set_uid(uid);
	mysqlService::ColmPair* colm;
	colm = ModifyUserData.add_colms();
	colm->set_colmname("`state`");
	colm->set_state(1);
	tcpSendMsg<mysqlService::ModifyUserRequest>(mysqlClient, (int)mysqlService::ID_ModifyUserRequest, &ModifyUserData);
	//3、然后准备2个基础服务器ip，（模拟一个群组中一个用户在一台，另外两个用户在另一台），并启动 √
	//4、需要确保三个用户在两台服务器上 √

	//5、然后模拟给lb服务器发送转发私聊消息请求的包  √
	//为了确保mysql服务器修改成功， 先等待1秒
	//这里只让一个人发包，而不是三个人都发包
	if (uid == 1) {
		lbService::RepostMsgRequest repostRequest; //先定义给lb服务器的转发请求
		repostRequest.set_modid(1);  //私聊
		repostRequest.set_fromid(1);  //用户1发的
		repostRequest.set_msg("你好");  //消息体
		repostRequest.set_toid(3);  //给3号的
		udpSendMsg<lbService::RepostMsgRequest>(udp, (int)lbService::ID_RepostMsgRequest, &repostRequest, &udp->m_recvAddr);
	}
	
	//6、这个时候只需要检查两台服务器有没有消息即可，如果能成功打印出消息，那说明lb服务器的群组转发消息功能正常
	//7、然后模拟给lb服务器发送转发群组消息请求的包
	//用户1给群组1发送的包
	if (uid == 1) {
		lbService::RepostMsgRequest repostRequest; //先定义给lb服务器的转发请求
		repostRequest.set_modid(2);  //群聊
		repostRequest.set_fromid(1);  //用户1发的
		repostRequest.set_msg("大家好");  //消息体
		repostRequest.set_toid(1);  //给1群发的
		udpSendMsg<lbService::RepostMsgRequest>(udp, (int)lbService::ID_RepostMsgRequest, &repostRequest, &udp->m_recvAddr);
	}
	//8、这个时候只需要检查其中一台服务器有没有消息即可，如果能成功打印出消息，那说明lb服务器的私聊转发消息功能正常

}

//转发消息请求响应 lb服务器要求当前服务器转发给客户端的消息
void HandleLbRepost(Udp* host, void* userData) {
	char* servername = (char*)userData;
	std::cout << "当前server：" << servername << std::endl;
	lbService::RepostMsgResponseTo requestData;
	requestData.ParseFromArray(host->m_request->m_data, host->m_request->m_msglen);
	std::cout << "当前要转发的消息类型：" << requestData.modid() << "消息来自：" << requestData.fromid() <<
		"目标组：" << requestData.gid() << std::endl;

	for (int i = 0; i < requestData.toid_size(); i++) {
		std::cout << "当前server要发的目标uid：" << requestData.toid(i) << std::endl;
	}
	for (int i = 0; i < requestData.msg_size(); i++) {
		std::cout << "当前server要发目标消息：" << requestData.msg(i) << std::endl;
	}
}


//start
//using hookCallBack = std::function<int(Udp* udp, void* userData)>;
int start(Udp* udp, void* userData) {
	GetServerIp(udp, 1, 1); //用户1登录
	GetServerIp(udp, 1, 2); //用户2登录
	GetServerIp(udp, 1, 3); //用户3登录
	//GetServerIp(udp, 2, 1);
}
int main() {
	//连接mysql的tcpclient
	TcpClient* mysqlClient = new TcpClient("127.0.0.1", 10000);  //连接msyql服务器
	mysqlClient->connectServer();
	//std::thread mc(&TcpClient::EventLoop::run, mysqlClient);  //mysqlClient->m_evloop->run();  错误
	std::thread mc(&TcpClient::run, mysqlClient);  //mysqlClient->run();
	
	//这两台必须要放到client前面，因为client一启动立马一连串的响应，然而相应玩之后，这两个服务器的业务函数还没启动。。。。
	//两台假装基础服务器的udp,用来接收lb服务器的转发的消息
	UdpServer baseServer1("127.0.0.1", 10010);
	UdpServer baseServer2("127.0.0.1", 10011);
	//注册接收转发响应的函数
	std::string server1Name("baseServer1");
	std::string server2Name("baseServer2");
	baseServer1.addMsgRouter((int)lbService::ID_RepostMsgResponseTo, HandleLbRepost, &server1Name[0]);
	baseServer2.addMsgRouter((int)lbService::ID_RepostMsgResponseTo, HandleLbRepost, &server2Name[0]);

	std::thread bs1(&UdpServer::run, &baseServer1);
	std::thread bs2(&UdpServer::run, &baseServer2);


	//验证lbAgent是否能正常返回数据
	UdpClient client("127.0.0.1", 10001);  //给lb服务器发消息
	std::string clinetName("client");
	client.addMsgRouter((int)lbService::ID_GetServerResponse, HandleServerIp, mysqlClient);
	client.addMsgRouter((int)lbService::ID_RepostMsgResponseTo, HandleLbRepost, &clinetName[0]);
	client.setConnStart(start);
	//当前客户端放到一个线程里工作，下面几个也类似
	std::thread realClient(&UdpClient::run, &client);
	
	//必须要有join函数或者detach，否则主线程执行完就结束了，其他的线程也结束了，所有的函数都不会执行（除了start函数外，他也只负责写到写缓冲区里）
	mc.join();
	realClient.join();
	bs1.join();
	bs2.join();
}