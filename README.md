# MovieHub
一个提供在线观影需求的服务器。
# 0. 注意：并没有严格遵守.cpp文件实现函数逻辑，有些函数实现甚至类都是在.h中实现的。
# 1. 在线影院服务器信息交互示意图如下：
![image](https://github.com/ZhongLinFan/MovieHub/blob/main/images/%E5%9C%A8%E7%BA%BF%E5%BD%B1%E9%99%A2%E6%9C%8D%E5%8A%A1%E5%99%A8%E4%BF%A1%E6%81%AF%E4%BA%A4%E4%BA%92%E7%A4%BA%E6%84%8F%E5%9B%BE.PNG)
# 2. reactor多反应堆架构示意图（以TcpServer为例）
![image](https://github.com/ZhongLinFan/MovieHub/blob/main/images/Reactor%E5%A4%9A%E5%8F%8D%E5%BA%94%E5%A0%86%E6%9E%B6%E6%9E%84%E7%A4%BA%E6%84%8F%E5%9B%BE.png)
# 3. reactor多反应堆信息交互示意图（以TcpServer为例）
TODO
其流程大致经过如下步骤：
+ mainLoop运行在主线程中，专门负责接收到来的cfd，如果未达到负载上限，会生成一个TcpConnection，并选择一个子线程中的EvLoop，将一个添加的任务交给EvLoop处理，若子线程在睡眠，那么进行唤醒操作，最终这个子线程负责将这个Channel添加到Select/Poll/Epoll上
+ （若子线程个数为0，那么原本子线程的任务也会由主线程完成）
+ 当客户端发送数据给服务器之后，会唤醒对应子线程的Select/Poll/Epoll的TcpConnection对应的读回调函数，然后确定是读事件，并由对应的读回调函数处理到来的数据（读取数据到readBuf（这个时候可能会涉及到buf的自适应扩容操作），解析出msgid类型的msg，然后交给消息路由进行处理（若未定义该类型数据，就会默认处理），最终交由上层的某个已注册的函数进行逻辑处理）；
+ 当服务器需要发送数据时，需要让EvLoop处于写事件打开的状态（打开写事件的方式有：添加一个打开写事件的任务/直接调用打开写事件的函数/或者借用前面发送时打开的写状态），这时依然会唤醒Select/Poll/Epoll的TcpConnection对应的写回调函数，会由写回调函数将已经准备好的writeBuf里的数据写到对端，然后再适时的关闭写事件（直接关闭或者间接关闭）。
+ 当客户端或者服务器检测到对端断开时，服务端会清理相关的资源（包括channelMap的更新、onlineFd、Fd_TcpConn的更新以及TcpConnection的释放，其中TcpConnection会释放对应的buf，channel相关的资源），客户端会直接退出程序。
+ Log系统可以简单的进行调试。
+ qps测试程序见./reactor/example/qps
# 4. 负载均衡服务器架构示意图
TODO

# 5. mysql代理服务器架构示意图
TODO
# 6. 基础服务器架构示意图
TODO
# 7. 流媒体服务器架构示意图
TODO
