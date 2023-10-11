#include "mysqlService.pb.h"
#include "TcpClient.h"

//using hookCallBack = std::function<void(NetConnection* conn, void* userData)>;
//注意TcpClient也是一个NetConnection类型
void start(NetConnection* conn, void* userData) {

	//给mysql代理服务器发送一个(int)router::ID_GetRouteRequest类型的消息
	mysqlService::GetRouterRequest responseData;
	responseData.set_modid(1);  //0代表所有的服务器
	std::string responseSerial;  
	responseData.SerializeToString(&responseSerial);
	//封装response
	auto response = conn->m_response;
	response->m_msgid = (int)mysqlService::ID_GetRouterRequest;
	response->m_data = &responseSerial[0];
	response->m_msglen = responseSerial.size();
	response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
	Debug("当前的数据为:%d,%d,%s", response->m_msgid, response->m_msglen, response->m_data);
	//发送数据到对端
	conn->sendMsg();
}

//注册一个router::ID_GetRouterResponse类型的回调函数，用来处理mysql代理服务器响应的数据
//using msgCallBack = std::function<void(NetConnection* conn, void* userData)>;
void handleMysqlResponse(NetConnection* conn, void* userData) {
	std::cout << "负载均衡服务器已收到mysql服务器返回的响应" << std::endl;
	mysqlService::GetRouterResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "modid" << responseData.modid() << std::endl;
	for (int i = 0; i < responseData.host_size(); i++) {
		std::cout << "ip：" << responseData.host(i).ip() << "\tport：" << responseData.host(i).port() << std::endl;
	}
}

int main(int argc, void* arg[])
{
	TcpClient* client = new TcpClient("127.0.0.1", 10000);
	client->setConnStart(start);
	client->addMsgRouter((int)mysqlService::ID_GetRouterResponse, handleMysqlResponse);
	client->connectServer();
	client->m_evloop->run();
}
