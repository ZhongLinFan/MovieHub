#include <stdio.h>
#include <unistd.h>  //chdir
#include <arpa/inet.h>  //htons
#include <string.h>
#include "msg.pb.h"
#include "TcpServer.h"
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
void sameReturn(NetConnection* conn, void* userData) {
    Qps* qpsResult = (Qps*)userData;
    qps::msg requestData, responseData;
    //解析request的m_data包
    requestData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);

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
    auto response = conn->m_response;
    response->m_msgid = 1;
    //response->m_data = responseSerial.data();  错误，这样返回的是const char* 不能赋给char*，可以使用下面的
    response->m_data = &responseSerial[0];
    response->m_msglen = responseSerial.size();
    response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
    conn->sendMsg();
}


//普通任务测试 // 给当前线程的某一个fd发送消息//这是服务器执行的
//注意一定要注意，这个是服务器的子线程执行的，所以还是服务器执行的，不要认为是client执行这个任务，client都不知道服务器的这个函数地址，怎么执行
//一定要注意
void readomReturn(EventLoop* loop, void* userData) {
    Debug("readomReturn正在执行");
    TcpConnection* conn = (TcpConnection*)userData;
    //因为这里有evloop，所以也可以在这里随机选择发送的客户端
    auto response = conn->m_response;
    response->m_msgid = 200;
    int randomIndex = rand() % 20; //不能将rand() % 11写到下面两个，每次的值都不一样的
    response->m_data = conn->msg[randomIndex];
    response->m_msglen = strlen(conn->msg[randomIndex]);  //MSG_HEAD_LEN不能少，否则会最终停在m_wbuffer->writeToBuffer(m_response->m_data, m_response->m_msglen - MSG_HEAD_LEN) <= 0，直接结束（返回值小于0），因为那里减去MSG_HEAD_LEN了
    response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
    conn->sendMsg();  //一个客户端上限，服务器给当前上线的客户端的线程池中的某一个线程发消息
}

//新客户端创建成功之后，随机给某个客户端（服务器这边的子线程的某一个fd）发送一个任务readomReturn函数，让这个线程给对应的客户端回复消息
void start(NetConnection* conn, void* userData) {
    //注意这是服务器的某一个子线程的某个fd执行的，一定要分清楚
    TcpServer* server = (TcpServer*)userData;
    //随机选择一个线程
    int randNum = rand() % 5;
    auto thread = server->m_threadPool[randNum];
    //随机选择一个服务器这边的客户端，给这个客户端发一个任务，
    //让他给对端的客户端进程发送消息，相当于是服务器给客户端响应消息
    auto onlines = conn->m_evloop->m_onlineFd;
    auto it = onlines.begin();
    for (; it != onlines.end(); it++) {
        if (*it == conn->m_channel->m_fd)continue;
        Debug("当前选择的事件循环id为:%d", TcpServer::m_Fd_TcpConn[*it]->m_evloop->m_seq);
        //这里的readomReturn是在类内部执行的时候直接赋值this的，所以这里不需要传递，这个很重要
        TcpServer::m_Fd_TcpConn[*it]->m_evloop->addTask(TcpServer::m_Fd_TcpConn[*it]->m_evloop, TcpServer::m_Fd_TcpConn[*it]->m_evloop->packageTask(readomReturn, TcpServer::m_Fd_TcpConn[*it]));
        return;
    }
    std::cout << "抱歉，当前选择的线程无其他客户端在线" << std::endl; 
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
    auto server = TcpServer(port,5); //epoll_create: Too many open files不能打开太多即使是epoll，否则会出现错误

    //server.addMsgRouter(1, sameReturn, (void* &)qpsResult); //错误
   // server.addMsgRouter(1, sameReturn, (void* )&qpsResult);
    server.setConnStart(start, &server);  //一定要传递地址呀
    server.run();
    return 0;
}
