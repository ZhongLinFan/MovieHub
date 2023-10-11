#include "LbAgent.h"

int main() {
	LbAgent lbAgent("127.0.0.1", 10001, "127.0.0.1", 10000, {1,2});  //前面时udp的IP地址，后面的是连接数据库的ip地址
	lbAgent.run();
	//主线程执行完之后就退出了
}