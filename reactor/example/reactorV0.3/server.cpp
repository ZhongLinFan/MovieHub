#include <stdio.h>
#include <unistd.h>  //chdir
#include <arpa/inet.h>  //htons
#include "TcpServer.h"
#include <string.h>


//using msgCallBack = std::function<void(NetConnection* conn, void* userData)>;
//根据这个写几个回调业务，然后在main种注册一下
//和msgid 1关联
void randomReturn(NetConnection* conn, void* userData) {
    auto response = conn->m_response;
    response->m_msgid = conn->m_request->m_msgid+1;
    int randomIndex = rand() % 20; //不能将rand() % 11写到下面两个，每次的值都不一样的
    response->m_data = conn->msg[randomIndex];
    response->m_msglen = strlen(conn->msg[randomIndex]) + 1 + MSG_HEAD_LEN;  //MSG_HEAD_LEN不能少，否则会最终停在m_wbuffer->writeToBuffer(m_response->m_data, m_response->m_msglen - MSG_HEAD_LEN) <= 0，直接结束（返回值小于0），因为那里减去MSG_HEAD_LEN了
    std::cout << "m_msgid:" << response->m_msgid << "m_msglen:" << response->m_msglen << std::endl;
    response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
    conn->sendMsg();
}

void start(NetConnection* conn, void* userData) {
    //网络字节序需要转为主机字节序，ip地址在转为ip地址的时候，则可以直接转为字符串
    printf("玩家已上线，玩家信息：fd:%d,ip:%s,port:%d\n", conn->m_channel->m_fd,
        inet_ntoa(conn->m_sOrcAddr->sin_addr), ntohs(conn->m_sOrcAddr->sin_port));  //注意，inet_ntoa(m_clientAddr->sin_addr)必须这样写，写成inet_ntoa(m_clientAddr->sin_addr.s_addr时错误的)
}

void lost(NetConnection* conn, void* userData) {
    //网络字节序需要转为主机字节序，ip地址在转为ip地址的时候，则可以直接转为字符串
    printf("玩家已下线，玩家信息：fd:%d,ip:%s,port:%d\n", conn->m_channel->m_fd,
        inet_ntoa(conn->m_sOrcAddr->sin_addr), ntohs(conn->m_sOrcAddr->sin_port));  //注意，inet_ntoa(m_clientAddr->sin_addr)必须这样写，写成inet_ntoa(m_clientAddr->sin_addr.s_addr时错误的)
}

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

    server.addMsgRouter(1, randomReturn);
    server.setConnStart(start);
    server.setConnLost(lost);
    server.run();
    return 0;
}
