#include <stdio.h>
#include <unistd.h>  //chdir
#include <arpa/inet.h>  //htons
#include <string.h>
#include "msg.pb.h"
#include "Udp.h"
//和msgid0关联

struct Qps {
    Qps() {
        lastTime = time(NULL);
        succCnt = 0;
    }
    long lastTime;
    int succCnt;
};

//qps测试
int sameReturn(Udp* server, void* userData) {
    Qps* qpsResult = (Qps*)userData;
    qps::msg requestData, responseData;
    //解析request的m_data包
    requestData.ParseFromArray(server->m_request->m_data, server->m_request->m_msglen);

    //组织response->m_data的数据，注意，这个只是个指针
    responseData.set_id(requestData.id());
    responseData.set_content(requestData.content());

    //服务端统计一秒所有的响应次数
    if (responseData.content() == "hello server!") {
        qpsResult->succCnt++;
    }
    int currentTime = time(NULL);
    if (currentTime - qpsResult->lastTime >= 60) {
        std::cout << "[cnt:" << qpsResult->succCnt / 60<< "]" << std::endl;
        qpsResult->lastTime = currentTime;
        qpsResult->succCnt = 0;
    }

    //将response序列化
    std::string responseSerial;
    responseData.SerializeToString(&responseSerial);
    //封装response
    auto response = server->m_response;
    response->m_msgid = 1;
    //response->m_data = responseSerial.data();  错误，这样返回的是const char* 不能赋给char*，可以使用下面的
    response->m_data = &responseSerial[0];
    response->m_msglen = responseSerial.size();
    response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
    server->sendMsg();
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
    Qps qpsResult;
    auto server = UdpServer("127.0.0.1", 10007); //epoll_create: Too many open files不能打开太多即使是epoll，否则会出现错误
    //不是172，如果地址填不对，会报不能分配
    server.addMsgRouter(1, sameReturn, (void* )&qpsResult);
    server.m_evLoop->run();
    return 0;
}
