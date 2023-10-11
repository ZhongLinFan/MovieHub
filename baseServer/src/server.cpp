#pragma once
#include "BaseServer.h"

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cout << "./BaseServer ip port" << std::endl;
		return -1;
	}
	BaseServer server = BaseServer(argv[1], atoi(argv[2]));
	server.run();
	return 0;
}