#include <stdio.h>
#include <unistd.h>  //chdir
#include <arpa/inet.h>  //htons
#include "TcpServer.h"

int main(int argc, void* arg[])
{
    unsigned short port;
#if 0
    if (argc < 3) {
        printf("./a.out path port");
        return -1;
    }
    chdir(arg[1]);
    port = atoi(arg[2]);
#else
    //chdir("home/tony");注意这样写是错的！！！！一定要加上/
    chdir("/home/tony/");
    port = 10000;
#endif
    //port = htons(port);   // 不要在这进行端口转换呀！！！！！port = htons(port)，因为这都是在初始化lfd的时候一气呵成做得！！！！
    //转换了两边之后
    auto server = TcpServer(port,10); //epoll_create: Too many open files不能打开太多即使是epoll，否则会出现错误
	
    server.run();
    return 0;
}
