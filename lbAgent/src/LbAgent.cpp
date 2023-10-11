#include "LbAgent.h"
#include <thread>
#include <functional>
#include <memory>

LbAgent::LbAgent(const char* udpIp, unsigned short udpPort, const char* mysqlAgentIp, unsigned short mysqlAgentPort, std::initializer_list<int> modidSet):serverMap(modidSet)
{
	udpServer = new UdpServer(udpIp, udpPort);
	tcpClient = new TcpClient(mysqlAgentIp, mysqlAgentPort);
	user_ServerMap.clear();
	//注册tcp来ID_GetRouterResponse消息时的回调函数
	tcpClient->addMsgRouter((int)mysqlService::ID_GetRouterResponse, //tcp2
		std::bind(&LbAgent::requestHandle<NetConnection, mysqlService::GetRouterResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//注册客户端获取服务器请求的回调函数
	udpServer->addMsgRouter((int)lbService::ID_GetServerRequest,  //udp1
		std::bind(&LbAgent::requestHandle<Udp, lbService::GetServerRequest>, this, std::placeholders::_1, std::placeholders::_2), nullptr); //udp最后一个参数没有默认，后期可以改一下
	//注册业务服务器返回的客户端的登录情况函数
	udpServer->addMsgRouter((int)lbService::ID_ServerResonseToLb,  //udp3
		std::bind(&LbAgent::requestHandle<Udp, lbService::ServerResonseToLb>, this, std::placeholders::_1, std::placeholders::_2), nullptr);

	//注册业务服务器端转发消息请求的函数
	udpServer->addMsgRouter((int)lbService::ID_RepostMsgRequest,  //udp4
		std::bind(&LbAgent::requestHandle<Udp, lbService::RepostMsgRequest>, this, std::placeholders::_1, std::placeholders::_2), nullptr);
	//注册获取群组在线成员的回调函数
	tcpClient->addMsgRouter((int)mysqlService::ID_GetGroupMemberResponse, //tcp24
		std::bind(&LbAgent::requestHandle<NetConnection, user::GetGroupMemberResponse>, this, std::placeholders::_1, std::placeholders::_2));

	//注册停止服务的回调函数
	udpServer->addMsgRouter((int)lbService::ID_StopServiceRequest, //udp7
		std::bind(&LbAgent::requestHandle<Udp, lbService::StopServiceRequest>, this, std::placeholders::_1, std::placeholders::_2), nullptr);
	//tcp连接mysql服务器
	tcpClient->connectServer();
}

LbAgent::~LbAgent()
{
}

void LbAgent::run()
{
	//线程2启动udpserver
	//std::thread startUdp(&LbAgent::udpServer::run, udpServer);
	// std::thread startUdp(udpServer->run, udpServer);
	// std::thread startUdp(&LbAgent::udpServer->run, udpServer);上面这些全都不对，第一个参数应该是一个类的成员函数，第二个应该是个对象
	std::thread startUdp(&UdpServer::run, udpServer);
	//线程3启动tcpserver
	std::thread startTcp(&TcpClient::run, tcpClient);

	//这里先休眠5秒，保证tcpClient->connectServer();能够连接成功
	//否则会有很小概率导致一直发包不成功，这个bug已在base服务器的bug里记录了
	//一定要在tcpClient之后休眠，否则没用的。。。。
	std::this_thread::sleep_for(std::chrono::seconds(5));

	//当前是主线程在执行，主线程还有其他任务在做，所以不能使用join阻塞，等待上面两个函数执行完
	//所以让主线程和子线程分离，就是detach函数
	//线程1启动定时任务
	std::thread planUpdate(&LbAgent::planUpdateRouter, this);
	planUpdate.join();
	startUdp.join();
	startTcp.join();
	//但是目前没有给主线程安排任务，主线程执行完main之后就退出了，所以最终有三个线程
	//但是好像如果主线程不阻塞，程序直接退出去了
}

//将原来的凌乱的输入输出逻辑按照mysql的那种模板处理了，如果想看之前的逻辑，可以看上一版的代码
//所有的消息都会到这里来，由这个分流出去，这个函数模板会进行多次注册（包括tcp或者udp的）（为什么可以合并呢，因为不管是udp还是tcp他们的解析和发送流程都是一样的）
//感觉这样处理并不是很好，因为有大量的函数时同名的，我感觉应该更应该广泛的用注册机制，注册函数用的id我们不用管，然后在注册函数内部，调用这个解析函数
//解析消息体，因为msgid需要用到，这里其实并没有发挥很大价值
template<class Conn_T, class Request_T>
void LbAgent::requestHandle(Conn_T* conn, void* userData)
{
	Debug("requestHandle开始执行");
	//解析data数据（接收数据已经交给底层tcpserver了）
	//可以考虑使用智能指针,后面怎么释放就不用管了
	std::shared_ptr<Request_T> requestData(new Request_T);
	requestData->ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);

	//进行对应的逻辑处理
	DealingRequest(conn, requestData);
}


void LbAgent::monitor()
{
	//这里监视routerMap的情况即可，其他两个的都在这里面
#if 0
	for (auto key_set : tempRouterMap) {
		for (auto host : key_set.second) {
			if (host.serviceCondition > 5) {
				ServerAddr tmp;
				tmp.ip = host.ip;
				tmp.port = host.port;
				host.serviceCondition = 99999;
				key_set.second.erase(host);
				key_set.second.insert(tmp);
			}
		}
	}
#endif
}

void LbAgent::planUpdateRouter()
{
	//tcp执行的定时任务
	//tcp给服务器发送ID_GetRouteRequest 的data为GetRouterRequest的消息
	while (true) {
		Debug("正在请求第一张表");
		mysqlService::GetRouterRequest responseData;
		responseData.set_modid(1);  //1代表基础服务器
		tcpSendMsg<mysqlService::GetRouterRequest>(tcpClient, (int)mysqlService::ID_GetRouterRequest, &responseData);
		//第二张表
		Debug("正在请求第二张表");
		responseData.set_modid(2);  //1代表基础服务器
		tcpSendMsg<mysqlService::GetRouterRequest>(tcpClient, (int)mysqlService::ID_GetRouterRequest, &responseData);
		//发送完之后就可以退出了，然后等待消息到来触发函数
		//为了保证性能，可以让这个函数一定周期执行以下，也就是休眠一段时间唤醒检测一下
		std::this_thread::sleep_for(std::chrono::seconds(10)); //10秒更新一次
	}
}

//接收mysql的router信息处理函数
void LbAgent::DealingRequest(NetConnection* tcpClient, std::shared_ptr<mysqlService::GetRouterResponse> responseData)
{
	//这个并不是什么高效率的模式，后续可以增加一个只收集截至时间戳的路由，然后进行直接添加就行，而不是还要先建立一张map等等
	//判断两个map中的set是否相等，如果都相等，那么直接退出，如果不相等，那么进行修改
#if 1
	//打印收到的数据的结果
	std::cout << "===========" << std::endl;
	std::cout << "当前收到：" << std::endl;
	std::cout << "modid:" << responseData->modid() << std::endl;
	for (int i = 0; i < responseData->host_size(); i++) {
		std::cout << "ip:" << responseData->host(i).ip()
			<< "\tport:" << responseData->host(i).port() << std::endl;
	}
	std::cout << "===========" << std::endl;
	std::cout << "更新前Map：" << std::endl;
	serverMap.printServerMapInfo();
	std::cout << "===========" << std::endl;
#endif
	int modid = responseData->modid();  //当前处理的set
	std::cout << "responseData->host_size():" << responseData->host_size() << std::endl;
	for (int i = 0; i < responseData->host_size(); i++) {
		ServerAddr serverAddr;
		serverAddr.m_ip = responseData->host(i).ip();
		serverAddr.m_port = responseData->host(i).port();
		serverAddr.m_serverModid = modid;
		serverMap.insertServer(serverAddr);
	}
	serverMap.submit(modid);
#if 1
	//打印更新后的routerMap的结果
	std::cout << "===========" << std::endl;
	std::cout << "更新后Map：" << std::endl;
	serverMap.printServerMapInfo();
	std::cout << "===========" << std::endl;
#endif
}

//处理udp的server地址获取请求（流媒体或者基础服务器）
void LbAgent::DealingRequest(Udp* udpServer, std::shared_ptr<lbService::GetServerRequest> requestData)
{
	int requestType = requestData->modid();
	Debug("requestType:%d", requestType);
	switch (requestType) {
	case 1:
		returnBaseServer(requestData->uid());
		break;
	case 2:
		enterMediaRoom(requestData->uid(), requestData->fid());
		break;
	}
}

void LbAgent::enterMediaRoom(int uid, int toFid)  //注意原来的fid不用传过来，因为uid对应的value里存的有当前的正在看的fid，就是user_ServerMap[uid].m_fid
{
	//先查看当前用户之前在看的电影，判断是否合法，如果合法，（需要先删除之前在看的一些信息（注意是serverMap中的信息，而不是user_ServerMap，因为user_ServerMap在最后更新，不对，应该是分配成功再删除之前的服务信息
	//），如果对方暂停怎么办？那也会一直传的）才会进行下面的操作，注意user_ServerMap在下面进行更新
	//如果不为0，那么就需要清理掉serverMap相关的信息（user_ServerMap的值不需要清理，最后部分会直接替换掉）
	Debug("正在为%d获取fid为%d的房间", uid, toFid);
	int fromFid = user_ServerMap[uid].m_fid;
	if (fromFid == toFid) {
		return;  //如果请求的fid相等，那么不处理
	}
	//清理原来的旧信息
	if (fromFid != 0) {
		//清理原来服务器的三个信息
		//更新流媒体server的人数（modid2） 需要放在这个if里面，因为万一客户端没看电影呢
		auto targetServerIt = user_ServerMap.find(uid);
		if (targetServerIt == user_ServerMap.end()) {
			return; //如果找不到，那就是有问题的，说明根本没登录上
		}
		//先取出当前用户的udp服务器地址
		ServerAddr& mediaServer = (*targetServerIt).second.m_mediaServer;  //注意前面的*,那只是个迭代器，需要得到元素本身才可以使用second
		ServerCondition* oldServerCondition = serverMap.getServer(&mediaServer);
		Debug("oldServerCondition:[%s:%d], condition:%d", oldServerCondition->m_addr.m_ip.data(), oldServerCondition->m_addr.m_port, oldServerCondition->getCondition());
		if (oldServerCondition != nullptr) {  //这里为啥加if，在stop那个地方已经说明了
			oldServerCondition->m_clientNums -= 1; //注意这里不能先update，如果先更新，后面就找不到m_mediaRoomMap的地址了，只要不update，就可以找到
			//需要从观看名单中删除掉当前的uid
			std::unordered_set<int>* clientSet = serverMap.getMediaServerClient(fromFid, mediaServer);  //clientSet和oldServerCondition其实是一个键值对，所以最后更新的是oldServerCondition
			clientSet->erase(uid);
			serverMap.updateServer(oldServerCondition);  //不需要提供uid和fid
		}
	}
	//然后分配服务器
	//如果是uid，fid这种方式获取server（或者说是如果想要得到当前房间的一个服务器地址），那肯定是为了得到当前房间的一个服务器地址，所以这个时候，直接在内部就可以更新了，就是添加人员之类的
	//不过感觉serverMap对应的类再提供两个成员函数，一个是添加人员，一个是删除人员，这样就比较灵活了，其实内部不内部添加都一样
	bool isNewServer = false;
	ServerCondition* targetServerCondition = serverMap.getServer(uid, toFid, isNewServer);
	targetServerCondition->m_clientNums += 1;
	std::unordered_set<int>* clientSet = serverMap.getMediaServerClient(toFid, targetServerCondition->m_addr);
	clientSet->insert(uid);
	//分配完服务器，再更新服务器
	serverMap.updateServer(targetServerCondition);

	//if和else分支已经更新了serverMap中的情况（包括总的服务器负载排序，当前房间流媒体排序，以及当前服务器服务的客户端集合），所以只需要更新user_ServerMap的情况
	//user_ServerMap的更新在这进行即可，如果分配成功才会进行更新，如果全都超负载，并且不可分配，那就获取不到地址
	//如果获取了有效的地址
	if (targetServerCondition != nullptr){
		user_ServerMap[uid].m_mediaServer = targetServerCondition->m_addr;  //注意这个是udp地址
		user_ServerMap[uid].m_fid = toFid;
		//其他三个属性和获取fid与否无关，是固定的，不用管
 
		//回复数据 只有得到有效地址才会发送数据
		getServerResponse(2, uid, &targetServerCondition->m_addr);
		//如果这个服务器人数只有1，说明是刚分配的，或者说是目前在空转的服务器（就是可能在播，但是房间里一个人都没有）
		if (isNewServer) {
			//给某个流媒体服务器发送一个播放某个电影的消息，其他的分支都不需要通知流媒体播放，其他的时候都是用户单独和流媒体通信了
			Debug("正在请求流媒体服务器开启房间播放视频");
			mediaService::AddRoomRequest addRoomRequest;
			addRoomRequest.set_fid(toFid);
			sockaddr_in toServerAddr;
			toServerAddr.sin_family = AF_INET;
			toServerAddr.sin_port = htons(targetServerCondition->m_addr.m_port);
			toServerAddr.sin_addr.s_addr = inet_addr(targetServerCondition->m_addr.m_ip.data());
			udpSendMsg<mediaService::AddRoomRequest>(mediaService::ID_AddRoomRequest, &addRoomRequest, &toServerAddr);
		}
	}
}

void LbAgent::returnBaseServer(int uid)
{
	//依赖客户端的ip，udpServer里有一个属性可以存，和一组ip（routerMap里有）
	//获取一个chat服务器（指针）
	Debug("开始执行returnBaseServer");
	//先验证当前账号是否已经分配baseServer
	//不用一开始就返回流媒体服务器，只需要一个基础服务器，所以这里不需要vector
	ServerAddr server;
	//更新routerMap选定服务器的人数
	auto targetServer = serverMap.getServer(1); //routerMap[1]是聊天模块对应的set,第一个是当前匹配规则最优的选择
	//注意上面的=类中是默认重载了的，所以上面的可以targetServer是一个独立的ServerAddr了
	Debug("当前用户%d是否已在线：%d",uid, user_ServerMap.find(uid) != user_ServerMap.end());
	if (user_ServerMap.find(uid) == user_ServerMap.end()) {  //当前user没有这个用户记录，我才给你uid
		//对这个target人数+1，然后重新加入到routerMap中去，然后将这个加入到user_ServerMap中
		targetServer->m_clientNums += 1;
		serverMap.updateServer(targetServer);
		
		//将这个服务器的地址加到对应的user_ServerMap上去
		server = targetServer->m_addr;
		user_ServerMap[uid] = ServerInfo(server);		//这里第二个参数先使用无参构造，等同步之后再更新第二个参数的udp
		//将这个用户的状态进行更新，不能在这更新，因为还没有登录验证，这个更新应该是基础服务器做的
	}
#if 0
	//将这个targetServer直接加入到对应的set中去，插入失败flag为0
	std::cout << "插入host.ip：" << targetServer.ip << "host.port：" << targetServer.port << "host.serviceCondition：" << targetServer.serviceCondition << std::endl;
	std::pair<std::set<ServerAddr, HostServiceConditionCmp>::iterator, bool> flags = routerMap[1].insert(targetServer);
	std::cout << "插入结果flags：" << flags.second << std::endl;
#endif
	Debug("返回基础服务器IP之后，routerMap情况：");
	serverMap.printServerMapInfo();  //这里不加;可能会报Debug的错误
	//回复数据
	getServerResponse(1, uid, &server);
}

void LbAgent::getServerResponse(int selectedModid, int uid, ServerAddr* server)
{
	lbService::GetServerResponse responseData;
	responseData.set_modid(selectedModid);  //流媒体代表基础服务器
	responseData.set_uid(uid);  //给谁的或者是哪个文件的
	//设置ip
	//在写流媒体服务器的时候，add_host这个进行repeat，并不合适，先留着吧，后面最后工作完善的时候用不着再改协议，因为当时想着流媒体房间内聊天也需要一个服务器的，
	//但是这个看来完全用基础服务器就行，所以完全不需要一个单独的服务器进行
	Debug("返回的地址详情：[%s:%d]", server->m_ip.data(), server->m_port);
	lbService::HostInfo* host = responseData.add_host();
	host->set_ip(server->m_ip);
	host->set_port(server->m_port);
	//发送数据
	udpSendMsg<lbService::GetServerResponse>((int)lbService::ID_GetServerResponse, &responseData, &udpServer->m_recvAddr);
}

//收到基础服务器的登录反馈（登上登不上，这边的map需要同步）
void LbAgent::DealingRequest(Udp* udpServer, std::shared_ptr<lbService::ServerResonseToLb> acceptResult)
{
	Debug("同步前当前user_server map情况：");
	for (auto user_server : user_ServerMap) {
		auto serverInfo = user_server.second;  //(server的tcp信息)  //注意for循环得到的是元素本身了
		std::cout << "uid:" << user_server.first << "\tbaseServer_ip:" << serverInfo.m_baseServer.m_ip << "\tbaseServer_port:" << serverInfo.m_baseServer.m_port
			<< "\tmediaServer_ip:" << serverInfo.m_mediaServer.m_ip << "\tmediaServer_port:" << serverInfo.m_mediaServer.m_port << "\tfid:"
			<< "\tm_baseUdpConn_ip:" << serverInfo.m_baseUdpConn.sin_addr.s_addr << "\tmediaServer_port:" << serverInfo.m_baseUdpConn.sin_port << "\tfid:" << serverInfo.m_fid << std::endl;
	}
	//同步状态
	int origin = acceptResult->originid();
	int finally = acceptResult->finalid();
	int modid = acceptResult->modid();   //模块
	if (modid == 1) {  // 基础服务器的响应
		if (acceptResult->result() == -1) {
			Debug("uid:%d登录失败", origin);
			//routermap的人数减去1
			//删掉当前客户端的user_ServerMap服务信息
			assert(handleStopService(origin));
			//user_ServerMap.erase(origin);  不能简单的只删除这个，人数信息也要在routerMap里更新
			Debug("同步后user_server map情况：");
			for (auto user_server : user_ServerMap) {
				auto serverInfo = user_server.second;  //(server的tcp信息)  //注意for循环得到的是元素本身了
				std::cout << "uid:" << user_server.first << "\tbaseServer_ip:" << serverInfo.m_baseServer.m_ip << "\tbaseServer_port:" << serverInfo.m_baseServer.m_port
					<< "\tmediaServer_ip:" << serverInfo.m_mediaServer.m_ip << "\tmediaServer_port:" << serverInfo.m_mediaServer.m_port << "\tfid:"
					<< "\tm_baseUdpConn_ip:" << serverInfo.m_baseUdpConn.sin_addr.s_addr << "\tmediaServer_port:" << serverInfo.m_baseUdpConn.sin_port << "\tfid:" << serverInfo.m_fid << std::endl;
			}
		}
		else {
			//登录成功，需要在这里记录对应的lb服务器udp地址，否则转发消息没法进行
			Debug("uid:%d登录成功", origin);
			assert(acceptResult->result() == 1);
			int targetUid = origin;
			if (origin != finally) {
				targetUid = finally;
				assert(user_ServerMap.find(finally) == user_ServerMap.end());  //断言这个新的id不存在
				user_ServerMap[finally] = user_ServerMap[origin];
				//记录udp地址
				user_ServerMap.erase(origin);
			}
			user_ServerMap[targetUid].m_baseUdpConn = udpServer->m_recvAddr;
			Debug("同步后user_server map情况：");
			for (auto user_server : user_ServerMap) {
				auto serverInfo = user_server.second;  //(server的tcp信息)  //注意for循环得到的是元素本身了
				std::cout << "uid:" << user_server.first << "\tbaseServer_ip:" << serverInfo.m_baseServer.m_ip << "\tbaseServer_port:" << serverInfo.m_baseServer.m_port
					<< "\tmediaServer_ip:" << serverInfo.m_mediaServer.m_ip << "\tmediaServer_port:" << serverInfo.m_mediaServer.m_port << "\tfid:"
					<< "\tm_baseUdpConn_ip:" << serverInfo.m_baseUdpConn.sin_addr.s_addr << "\tmediaServer_port:" << serverInfo.m_baseUdpConn.sin_port << "\tfid:" << serverInfo.m_fid << std::endl;
			}
		}
	}
	else if (modid == 2) {  // 流媒体模块
		//流媒体服务器不需要登录吧。。。。。。
	}
	else {
		//模块错误
	}
}

//处理消息转发逻辑（私聊或者群组）
void LbAgent::DealingRequest(Udp* udpServer, std::shared_ptr<lbService::RepostMsgRequest> requestData)
{
	int requestType = requestData->modid();
	switch (requestType) {
	case 1:
		//私聊转发
		privateTalk(requestData);
		break;
	case 2:
		//群聊转发
		//请求某个群的在线群组
		getGroupMemberList(requestData);  //他的请求响应之后就可以查找了
		//requestData怎么传递给groupTalk，因为groupTalk需要requestData和mysql返回的数据
		//可以给mysql添加一个连接属性，然后指向一个vector，然后将这个request的地址压进去，或者给groupTalk指向一个全局的vector，然后指向这个request
		//或者定义一个属性map，key是事件id，值是vector，vector类型就是key对应的message，完美
		//（感觉baseServer也可以参考这一点，baserver设计的是使用std::map<int, NetConnection*> uid_conn，然后找到conn的动态属性）
		//这个设计真的很有亮点

		//压入请求转发的请求指针
		msgCatch.record(requestData);
		//下面这样比较冗余
		//msgCatch.record(int(ID_XXX),requestData);
		break;
	}
}

void LbAgent::privateTalk(std::shared_ptr<lbService::RepostMsgRequest> requestData)
{
	Debug("私聊转发正在执行");
	auto targetServerIt = user_ServerMap.find(requestData->toid());
	//先取出源服务器的地址，取不取其实无所谓了，再增加了地址队列之后
	sockaddr_in fromAddr = udpServer->m_recvAddr;
	//然后设置发给源服务器的信息，先设置为不在线的状态
	lbService::RepostMsgResponseFrom responseFrom;
	responseFrom.set_result(-1);
	responseFrom.set_fromid(requestData->fromid());
	responseFrom.set_toid(requestData->toid());
	if (targetServerIt != user_ServerMap.end()) {
		auto& targetServer = targetServerIt->second;  //取出对组，一定要注意 []和find之后的取值方式不一样 （目前是取出结构体了）
		//当前目标在线，把消息转发到对应的服务器上，直接调用打包发送函数
		//注意如果先转发消息，那么应该保存原来服务器的ip地址，否则会丢失原来服务器的ip地址
		//将hostinfo转为sockaddr_in并且设置到udp的地址里
		sockaddr_in toAddr = targetServer.m_baseUdpConn; //取出基础服务器的udp地址
		Debug("需要server[ip:%s, port:%d]帮助转发消息", targetServer.m_baseServer.m_ip.data(), targetServer.m_baseServer.m_port);
		Debug("对应的udp网络地址为：server[ip:%d, port:%d]", toAddr.sin_addr.s_addr, toAddr.sin_port);
		//发送数据给目的服务器让其转发消息
		lbService::RepostMsgResponseTo responseData;
		responseData.set_modid(requestData->modid());
		responseData.set_fromid(requestData->fromid());
		responseData.add_msg(requestData->msg());
		responseData.add_toid(requestData->toid());
		udpSendMsg<lbService::RepostMsgResponseTo>((int)lbService::ID_RepostMsgResponseTo, &responseData, &toAddr);
		//能连续发两个消息给不同的服务器吗？
		//这里可以设置给from的消息内容
		responseFrom.set_result(1);
	}
	//不管在不在线，这里都已经准备好了回复源服务器的消息
	udpSendMsg<lbService::RepostMsgResponseFrom>((int)lbService::ID_RepostMsgResponseFrom, &responseFrom, &fromAddr);
}

//群聊转发处理
//这个需要同时分析两个响应message（一个是mysql的在线人员，一个是请求的转发消息）
//getGroupMemberList的响应函数
//conn用不上
void LbAgent::DealingRequest(NetConnection* conn, std::shared_ptr<user::GetGroupMemberResponse> memberData)
{
	Debug("群聊转发正在执行");
	//注意这里负载均衡服务器在接到一个请求之后，可能会发多个包给多台服务器
	//先去mysql服务器读取这个表的成员
	//方式1不行，这样写的话，将成员分门别类的传递给不同的服务器太复杂，需要拉取组成员信息，然后和这个在线成员名单对比，选出在线名单（这个需要成员人数*log在线人数）（此时可以打包好并转发给各服务器）
	//方式2如果是将在上线与否状态写入到数据库，那么直接筛选出来成员的在线人数（这个时间复杂度和上面的拉取名单复杂度一样）*log在线人数，因为本质上这边的查找ip也是一个红黑树
	//groupTalk执行时说明已经获取到两个包，并且都已经解析成功，这时可以先取出来在map里的requestData
	//注意ServerAddr里面并没重载<，只重载了==，这个时候如果使用ServerAddr做key（比如之前的hostSet也提供了比较函数），需要提供比较函数
	std::unordered_map<sockaddr_in, lbService::RepostMsgResponseTo, GetSockAddrHashCode, SockAddrIsEqual>responses;  //这里不需要排序
	//遍历在线组成员，将当前在线组成员分好服务器
	Debug("在线群组%d成员信息：", memberData->gid());
	for (int i = 0; i < memberData->member_size(); i++) {
		Debug("uid：%d，姓名：%s", memberData->member(i).uid(), memberData->member(i).name().data());
	}
	//这个targetRequest是为了保存处理好的消息准备删除的模板
	std::shared_ptr<lbService::RepostMsgRequest> targetRequest;
	for (int i = 0; i < memberData->member_size(); i++) {
		//某个在线的uid
		int personUid = memberData->member(i).uid();
		//获取uid对应的服务器地址
		assert(user_ServerMap.find(personUid)!= user_ServerMap.end());

		auto& targetServer = user_ServerMap[personUid];  //注意和上面的find得到的结果不一样，导致下面的tcpServer、udpServer和上面的targetServerIt取的不一样
		auto& tcpServer = targetServer.m_baseServer;
		auto& udpServer = targetServer.m_baseUdpConn;
		Debug("uid：%d，姓名：%s所在的主机：[%s:%d]", memberData->member(i).uid(), memberData->member(i).name().data(), tcpServer.m_ip.data(), tcpServer.m_port);
		//将responses对应的host种添加一个uid
		//先判断当前RepostMsgResponseTo是否存在，如果不存在，就先初始化一个
		if (responses.find(udpServer) == responses.end()) {
			//创建对应的回复response，并通过map索引
			lbService::RepostMsgResponseTo response;  //不能直接写道下面的=右边
			responses[udpServer] = response;
			//初始化一下其他项
			responses[udpServer].set_modid(2); //代表是群聊
			responses[udpServer].set_fromid(memberData->uid()); //谁发的
			responses[udpServer].set_gid(memberData->gid()); //发给哪个组
			//取出RepostMsgRequest的消息
			auto targetMsgList = msgCatch.getMsgList(targetRequest);
			Debug("targetMsgList的大小：", targetMsgList->size());
			for (auto request : *msgCatch.getMsgList(targetRequest)) {
				//找出所有的当前uid转发给gid的消息请求，可能来了一个请求之后，紧接着又来一个请求，这个时候就不需要反复查询mysql库了
				//注意如果uid转发给另一个gid就必须要查表了
				if (request->modid() == 2 && request->fromid() == memberData->uid() && request->toid() == memberData->gid()) {
					targetRequest = request;  //这样赋值可以吗
					//如果是modid相同，发消息的id相同，送到的群id也相同，那么说明是同一用户的多条消息
					responses[udpServer].add_msg(request->msg());
					//将当前的request从vector中删除掉（将对应的智能指针移除掉）
					//注意vector中的erase会导致后续的迭代器都会失效。。。。
					//然后发现这样动态的很频繁的增加删除用list更合适，而不是vector
					//一定要注意！！！！！范围的for不适合边遍历边删除，因为会一开始就获得一个迭代器，后面就认为结构不变了，那么你删除就改变了原来的结构
					//可以搭配remove_if和erase进行删除
				}
			}
		}
		//将当前的uid作为目标id添加到当前的服务器上
		//不用转发给自己，所以简单过滤一下
		//需要利用这个发给自己作为群聊发送消息成功的确认机制
		responses[udpServer].add_toid(personUid);
	}
	//遍历完成，那么删除这个list中所有的当前客户给当前组发的所有信息，注意这里最好使用互斥量锁一下
	//这里会将targetRequest这里所有的相同的uid和gid的都给删除完
	msgCatch.remove(targetRequest);
	//到这里说明所有的包都准备好了，发给对应的服务器群
	Debug("目标服务器个数：%d", responses.size());
	for (auto& response : responses) {
		//取出key，转为对应的sockaddr_in
		sockaddr_in addr = response.first;
		Debug("需要server[ip:%s, port:%d]帮助转发消息", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		Debug("对应的udp网络地址为：server[ip:%d, port:%d]", addr.sin_addr.s_addr, addr.sin_port);
		udpSendMsg((int)lbService::ID_RepostMsgResponseTo, &response.second, &addr);
	}
	//方式3如果是上线时把状态和对应的服务器ip地址写到mysql，那么查询过来的数据就是目标数据，不过我还是打算采用方式2
}

//停止服务请求（基础服务器给lb服务器的消息）
//这个是在写baseServer的时候，发现需要增加一个这样的功能，而且发现在处理登录同步的时候也需要对登录失败的情况执行下面的操作，之前并没有进行，并且没有在测试文件中测试，所以需要对这个测试一下
void LbAgent::DealingRequest(Udp* udpServer, std::shared_ptr<lbService::StopServiceRequest> stopServiceData) {
	handleStopService(stopServiceData->uid());
}

bool LbAgent::handleStopService(int uid) {
	int targetUid = uid;
	auto targetServerIt = user_ServerMap.find(targetUid);
	if (targetServerIt != user_ServerMap.end()) {
		//先取出对应的serverAddr
		ServerAddr baseServer = targetServerIt->second.m_baseServer;  //一定要注意前面的时->
		ServerAddr mediaServer = targetServerIt->second.m_mediaServer;
		//取出正在看的fid
		int fid = targetServerIt->second.m_fid;

		//删除uid和server的对应关系,注意lb服务器这里只需要维护user_ServerMap的关系，而serverMap应该提供一个接口函数来维护他自身内部的关系
		user_ServerMap.erase(targetUid);
		//基础服务器的负载情况更新
		auto baseServerCondition = serverMap.getServer(&baseServer);
		//基础服务器的人数减去1
		baseServerCondition->m_clientNums -= 1;
		serverMap.updateServer(baseServerCondition);

		//更新m_mediaRoomMap的情况
		if (fid != 0) {   //当前在看的fid
			//更新流媒体server的人数（modid2） 需要放在这个if里面，因为万一客户端没看电影呢
			ServerCondition* mediaServerCondition = serverMap.getServer(&mediaServer);
			if (mediaServerCondition != nullptr) {  //注意这里必须要加上if，因为如果mysql那里删除了正在使用的服务器，那么serverMap中的m_serviceConditionMap会严格和数据库的保持一致，
													//所以，user_ServerMap中存储的可能是已经被废弃的服务器地址，这个时候去总的服务器负载排行上查找就会查找不到，所以这里默认如果查找不到就是被舍弃的，不处理就行了
				mediaServerCondition->m_clientNums -= 1; //注意这里不能先update，如果先更新，后面就找不到m_mediaRoomMap的地址了，只要不update，就可以找到
				serverMap.updateServer(mediaServerCondition);
			}
		}
		return true;
	}
	return false;
}

void LbAgent::getGroupMemberList(std::shared_ptr<lbService::RepostMsgRequest> requestData)
{
	user::GetGroupMemberRequest GetGroupMemberData;
	GetGroupMemberData.set_uid(requestData->fromid());  //查询的人的uid，可以后面用来判断当前uid是否有资格查
	GetGroupMemberData.set_gid(requestData->toid());  //查询群号
	GetGroupMemberData.set_condition("state=1");  //查询状态在线的 
	tcpSendMsg<user::GetGroupMemberRequest>(tcpClient, (int)mysqlService::ID_GetGroupMemberRequest, &GetGroupMemberData);
}

//公共打包函数
template<class Conn_T, class Response_T>
void LbAgent::packageMsg(Conn_T* conn, int responseID, Response_T* responseData)
{
	Debug("requestHandle开始回复数据");
	//responseData的结果序列化
	std::string responseSerial;  //注意我让conn的response的data指针指向responseSerial是完全没问题的，因为sendMsg函数整个过程，responseData都是有效的，因为这个函数还没执行完
	responseData->SerializeToString(&responseSerial);
	//封装response
	auto response = conn->m_response;
	response->m_msgid = responseID;
	response->m_data = &responseSerial[0];
	response->m_msglen = responseSerial.size();
	response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
}

//由于UdpReply可能需要设置地址，而conn没有m_addr，所以需要对packageAndResponse进行一层包装
template<class Response_T>
void LbAgent::udpSendMsg(int responseID, Response_T* responseData, sockaddr_in* to)
{
	//如果to为nullptr，那么就默认发送给发过来的消息的客户端，如果不为空，那么就发给指定的客户端
	//设置发送地址
	//可以不用设置，因为一个包一个包的接收，那么当前地址在处理这个包之前就不会丢失
	//不对必须要设置，因为如果同时来两个包，（虽然这两个包他们是顺序处理的）但是后一个包的地址已经覆盖了前一个包的地址
	// 前一个包回复给了最后一个接收包的地址，所以每个包请求必须自带地址
	//不对，因为读到rbuff，每次只能读一个包，所以读完之后，立马处理了（不管这个包有几个请求，这个包都会处理完），然后再去readtoBuf，相当于再去处理下一个客户端
	// 那么就可以不设置读字符串，当然了，如果是消息转发，肯定还要设置的
	packageMsg<UdpServer, Response_T>(udpServer, responseID, responseData);
	udpServer->sendMsg(*to);
}

//由于Udp底层更改了地址的逻辑，导致sendMsg不得不有一个地址参数，所以不得不给TCP弄一个函数，将原来的打包发送函数改为打包函数
//公共发送函数
template<class Response_T>
void LbAgent::tcpSendMsg(NetConnection* conn, int responseID, Response_T* responseData)
{
	packageMsg(conn, responseID, responseData);
	//发送数据到对端
	conn->sendMsg();
}