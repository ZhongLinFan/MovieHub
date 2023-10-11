#include "TcpClient.h"

int main() {
	auto client = new TcpClient("127.0.0.1", 10000);
	client->connectServer();
	client->m_evloop->run();
}