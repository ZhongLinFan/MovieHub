# MovieHub
一个提供在线观影需求的服务器。
# 0. 注意：并没有严格遵守.cpp文件实现函数逻辑，有些函数实现、甚至类都是在.h中实现的。
# 1. 在线影院服务器信息交互示意图如下：
![image](https://github.com/ZhongLinFan/MovieHub/blob/main/images/%E5%9C%A8%E7%BA%BF%E5%BD%B1%E9%99%A2%E6%9C%8D%E5%8A%A1%E5%99%A8%E4%BF%A1%E6%81%AF%E4%BA%A4%E4%BA%92%E7%A4%BA%E6%84%8F%E5%9B%BE.PNG)
# 2. reactor多反应堆架构示意图（以TcpServer为例）
![image](https://github.com/ZhongLinFan/MovieHub/blob/main/images/Reactor%E5%A4%9A%E5%8F%8D%E5%BA%94%E5%A0%86%E6%9E%B6%E6%9E%84%E7%A4%BA%E6%84%8F%E5%9B%BE.png)
# 3. reactor多反应堆信息交互示意图（以TcpServer为例）
TODO  
其流程大致经过如下步骤：
+  TcpServer启动之后，会进行一系列的初始化工作，包括buffPool，mainLoop，lfd，threadPool等；
+  mainLoop运行在主线程中，专门负责接收到来的cfd，如果未达到负载上限，会生成一个TcpConnection（会初始化Channel，并且会绑定对应的回调函数等一系列，如果有hook函数，会执行hook函数），并选择一个子线程中的EvLoop，将一个添加任务交给EvLoop的任务队列，若子线程在睡眠，那么进行唤醒操作，最终这个子线程负责处理这个任务，也就是将这个Channel添加到Select/Poll/Epoll上；
+ （若子线程个数为0，那么原本子线程的任务会由主线程（mainLoop）完成）
+ **（reactor的实现就是围绕着Channel的读/写/删除回调函数进行的，下面3个点分别解释了调用的时机和调用的大致逻辑）**
+ 当客户端发送数据给服务器之后，会唤醒对应子线程的Select/Poll/Epoll，确定是读事件，TcpConnection对应的读回调函数处理到来的数据（读取数据到readBuf（这个时候可能会涉及到buf的自适应扩容操作），解析出msgid类型的msg，然后交给消息路由进行处理（若未定义该类型数据，就会默认处理），最终交由上层的某个已注册的函数进行逻辑处理）；
+ 当服务器需要发送数据时（在准备数据时可能依然需要buf的扩容操作），需要让EvLoop处于写事件打开的状态（打开写事件的方式有：添加一个打开写事件的任务/直接调用打开写事件的函数/或者借用前面发送时打开的写状态），这时依然会唤醒Select/Poll/Epoll的TcpConnection对应的写回调函数，会由写回调函数将已经准备好的writeBuf里的数据写到对端，然后再适时的关闭写事件（直接关闭或者间接关闭）。
+ 当客户端或者服务器检测到对端断开时，服务端会调用相关的清理回调函数，清理相关的资源（包括channelMap的更新、onlineFd、Fd_TcpConn的更新以及TcpConnection的释放，其中TcpConnection会释放对应的buf，channel相关的资源），客户端会直接退出程序。
+ Log系统可以简单的进行调试。
+ qps测试程序见./reactor/example/qps
# 4. 负载均衡服务器架构示意图
TODO  
其包含 定时更新服务器集群ip信息、 基础/流媒体服务器地址获取请求、房间与服务器集群的关系维护、群/私聊消息转发处理、停止服务处理5个功能模块。
+ **serverMap的成员关系设计比较复杂（可能设计的并不合理）。**
+ 0.1. 设计当前结构的背景是在解决baseServer/主要bug.txt中第16个bug进行的，当时面临的情况是按照负载情况排序，使用ip和port查询时会出现找不到的情况，（当时对map的理解并不够深刻，没有意识到底层的红黑树结构是按照负载定的，ip和port则是乱序的，那么按照ip和port查询自然是不可能找到的。），而当时我的诉求是使用负载排序，能够使用ip和port ServerCondition快速的定位到目标。然后想到0.2的思路：
+ 0.2. 首先是std::map<int, serverSet> m_serviceConditionMap，key值为modid值，比如1代表基础服务器，serverSet为std::multiset<ServerCondition*, ServiceConditionCmp>，ServerCondition作为key是为了能够按照server的负载情况进行排序，那么getServer的时候，就会简单很多。不过仅仅这样是不够的，我还需要通过ip找到ServerCondition（比如更新ip的时候，首先就需要通过mysql传过来的ip值定位到ServerCondition），但是遍历serverSet是我比较排斥的，所以我又设计了std::unordered_map<ServerAddr, ServerCondition*, GetHashCode> m_serverMap，这样就可以很快速的定位到ServerCondition，不过ServerCondition需要有很明确的逻辑所属关系，否则释放的时候容易重复释放或者泄漏。我这里认为ServerCondition属于serverSet，m_serverMap只有使用权。
+  0.3. **0.2这个设计可以满足我前面的需求，但是删除serverSet的时候会将相同负载的服务器全都删除掉了（baseServer/主要bug.txt中第16个bug），这显然是不对的，然后就想到设计一个每台服务器任意时刻负载情况一定不同的思路（可能也有其他思路）这样就保证删除的时候肯定能定位到想要的目标，最终想到的是使用一个int值，低16位存储初始评价值，高16位存储运行之后的负载情况，可以将低16位看作小数点之后的当前服务器的唯一id。那么serverSet就没有必要使用multiset，而是改用set即可。这种设计就解决了我的基本需求，当然可能不是最简单的（比如使用equal_range，然后定位到想定位的ServerCondition）。**
  -----------------------------------------------------------------------------------------------
+ 1. 定时更新服务器集群ip信息：一个子线程定时的给mysql服务器发送获取路由表更新的请求，并且在得到消息之后更新到serverMap中的serviceConditionMap（更新操作主要依赖于两个函数，**insertServer和submit**）
+ 2. 基础/流媒体服务器地址获取请求：服务器收到客户端的基础服务器ip地址请求之后，会调用serverMap中的getServer（针对基础服务器的重载版本1），然后更新serverMap（updateServer），并返回相应的数据；当收到请求流媒体服务器的地址之后，会调用getServer（针对流媒体服务器的重载版本2，这里处理的逻辑较为复杂，）。
+ 3. 房间与服务器集群关系维护：当收到请求播放某个电影资源的请求之后，会首先查看是否有当前房间，如果有，并且当前房间（std::unordered_map<int, std::map<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp>> m_mediaRoomMap）的map中有运转良好的服务器，那么分配出去，然后更新负载情况，这个时候需要更新两个map，一个是m_mediaRoomMap还有一个是m_serverMap；如果当前房间服务器map全都满负荷或者是根本没有当前房间的存在，会从m_serverMap获得一个新的服务器放入当前房间，并且更新两个map。如果当前房间某台服务器人数为0，会删除该房间的空载服务器，并且更新，如果当前房间没有服务器，会清除该房间。
+ 4. 群/私聊消息转发处理：如果是私聊，会查询在线用户表，然后找到对应的服务器地址，封装一个消息转发到对应的基础服务器，由对应的基础服务器转发到对应的客户端；如果是群聊，会先向mysql服务器请求当前群聊的在线用户名单，然后查询在线用户表依次发送过去。
+ 5. 停止服务处理：当登录失败或者客户端主动下线时，基础服务器会向负载均衡服务器发送一个停止为某个用户服务的请求，负载均衡服务器负责清理相关信息（在线用户表，对应的基础服务器负载更新，对应的流媒体房间的用户信息更新）。
# 5. mysql代理服务器架构示意图
TODO
# 6. 基础服务器架构示意图
TODO
# 7. 流媒体服务器架构示意图
TODO
# 8. 数据库表关系图
TODO
