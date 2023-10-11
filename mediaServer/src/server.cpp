#include "MediaServer.h"

int main(int argc, char* argv[]){
	if (argc < 3) {
		std::cout << "./mediaServer ip port" << std::endl;
		return -1;
	}
	//argv[1], argv[2]为当前服务器地址，中间两个是lb服务器的udp地址，最后两个是mysql的地址
	MediaServer server(argv[1], std::stoi(argv[2]), "127.0.0.1", 10001, "127.0.0.1", 10000);
	server.run();
}