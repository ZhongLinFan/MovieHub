#include <string.h>
#include "msg.pb.h"
#include "Log.h"
#include "Udp.h"

struct Qps {
    Qps() {
        lastTime = time(NULL);
        succCnt = 0;
    }
    long lastTime;
    int succCnt;
};

int sameReturn(Udp* client, void* userData) {
    Qps* qpsResult = (Qps*)userData;

    qps::msg requestData, responseData;
    //解析request的m_data包
    requestData.ParseFromArray(client->m_request->m_data, client->m_request->m_msglen);

    //组织response->m_data的数据，注意，这个只是个指针
    responseData.set_id(requestData.id());
    responseData.set_content(requestData.content());
    if (responseData.content() == "hello server!") {
        qpsResult->succCnt++;
    }
    long currentTime = time(NULL);
    if (currentTime - qpsResult->lastTime >= 1) {
        std::cout << "[cnt:" << qpsResult->succCnt << "]" << std::endl;
        qpsResult->succCnt = 0;
        qpsResult->lastTime = currentTime;
    }
    //将response序列化
    std::string responseSerial;
    responseData.SerializeToString(&responseSerial);
    //封装response
    auto response = client->m_response;
    response->m_msgid = client->m_request->m_msgid;
    response->m_data = &responseSerial[0];
    response->m_msglen = responseSerial.size();
    response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
    client->sendMsg();
}

int start(Udp* client, void* userData) {
	//主动发送消息给客户端
    qps::msg requestData;
    requestData.set_id(1000);
    requestData.set_content("hello server!");

    std::string responseSerial;
    requestData.SerializeToString(&responseSerial);
    auto response = client->m_response;
    response->m_msgid = 1;
    response->m_msglen = responseSerial.size();
    response->m_data = &responseSerial[0];

    response->m_state = HandleState::Done;
    client->sendMsg();
}

int main() {
    Qps qpsResult;
    Debug("qpsResult的地址：%x", &qpsResult);
	auto client = new UdpClient("127.0.0.1", 10007);
    client->addMsgRouter(1, sameReturn, (void*)&qpsResult);
    client->setConnStart(start);
    start(client, nullptr);
    Debug("qpsResult的地址：%x", (void*)&qpsResult);
	client->m_evLoop->run();
}