#include "lbService.pb.h"
#include "Udp.h"

//请求一个基础服务器地址或者流媒体服务器地址
void GetServerIp(Udp* udp, int mode) {  //1代表请求基础ip，2代表请求流媒体ip
	lbService::GetServerRequest responseData;
	responseData.set_modid(mode);  //1代表请求基础ip，2代表请求流媒体ip
	responseData.set_id(9527);  //客户uid

	std::string responseSerial;
	responseData.SerializeToString(&responseSerial);
	//封装response到tcp响应里面去
	auto response = udp->m_response;
	response->m_msgid = (int)lbService::ID_GetServerRequest;
	Debug("请求的业务号为：%d", response->m_msgid);
	response->m_data = &responseSerial[0];
	response->m_msglen = responseSerial.size();
	response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
	//发送数据到对端
	udp->sendMsg(udp->m_recvAddr);
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

//查询一个用户的基础服务器地址

//通知其他ip到这里取数据


//start
//using hookCallBack = std::function<int(Udp* udp, void* userData)>;
int start(Udp* udp, void* userData) {
	Debug("正在发送数据");
	GetServerIp(udp, 1);
	GetServerIp(udp, 2);
}
int main() {
	//验证lbAgent是否能正常返回数据
	UdpClient client("127.0.0.1", 10001);
	client.addMsgRouter((int)lbService::ID_GetServerResponse, HandleServerIp, nullptr);
	client.setConnStart(start);
	client.run();
}