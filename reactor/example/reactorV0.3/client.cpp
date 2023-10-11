#include "TcpClient.h"
#include <string.h>

void randomReturn(NetConnection* conn, void* userData) {
    auto response = conn->m_response;
    response->m_msgid = 1;
    int randomIndex = rand() % 20; //不能将rand() % 11写到下面两个，每次的值都不一样的
    response->m_data = conn->msg[randomIndex];
    response->m_msglen = strlen(conn->msg[randomIndex]) + 1 + MSG_HEAD_LEN;  //MSG_HEAD_LEN不能少，否则会最终停在m_wbuffer->writeToBuffer(m_response->m_data, m_response->m_msglen - MSG_HEAD_LEN) <= 0，直接结束（返回值小于0），因为那里减去MSG_HEAD_LEN了
    std::cout << "m_msgid:" << response->m_msgid << "m_msglen:" << response->m_msglen << std::endl;
    response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
    conn->sendMsg();
}

void start(NetConnection* conn, void* userData) {
    std::cout << "hello" << std::endl;
}

void lost(NetConnection* conn, void* userData) {
    std::cout << "bye bye" << std::endl;
}

int main() {
	auto client = new TcpClient("127.0.0.1", 10000);
    client->addMsgRouter(1, randomReturn);
    client->setConnStart(start);
    client->setConnLost(lost);
	client->connectServer();
	client->m_evloop->run();
}