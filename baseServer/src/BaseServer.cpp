#include "BaseServer.h"
#include <thread>

BaseServer::BaseServer(const char* ip, short port)
{
	//初始化各主机属性，然后在run中给某些属性分配线程运行起来
	tcpServer = new TcpServer(port, 2);  //底层的TcpServer使用的是本地地址初始化，上层不用设置，这里就先丢弃，后续如果远程，那么可以改底层，
									//而且这里给出主机的即可，底层在初始化的时候，会转换为网络字节序
	mysqlClient = new TcpClient("127.0.0.1", 10000);  //这里是mysql的地址，可以写死，或者从配置文件读取
	lbClient = new UdpClient("127.0.0.1", 10001);  //给lb服务器发消息的，也可以写死，或者从配置文件读取
	uid_conn.clear();
	msgIdRecoder = new TypeIdentifier();
	//注册相关回调事件、
	//注意，类型和事件id要一一对应，可以重复，但是不会多记录
	//客户端登录的请求处理动作1
	addHandleFunc<TcpServer, NetConnection, baseService::LoginRequest, mysqlService::GetUserBaseRequest>(tcpServer, &BaseServer::handleLoginStep1,
		{ baseService::ID_LoginRequest, mysqlService::ID_GetUserBaseRequest }); //这竟然可以不用强转
	//客户端登录的请求处理动作2（mysql的基础信息响应）（注意这里请求组，喜欢朋友列表的消息对应关系不在这写了，如果写太长了，直接指明id就行）
	addHandleFunc<TcpClient, NetConnection, mysqlService::GetUserBaseResponse, lbService::ServerResonseToLb, baseService::LoginResponse>(mysqlClient, &BaseServer::handleLoginStep2,
		{ mysqlService::ID_GetUserBaseResponse, lbService::ID_ServerResonseToLb ,baseService::ID_LoginResponse });

	//mysql的组，好友，喜欢信息响应处理逻辑
	addHandleFunc<TcpClient, NetConnection, user::GetUserFavoriteResponse, baseService::LoginResponse>
		(mysqlClient, &BaseServer::handleFavoriteList, //mysqlClient 4
		{ mysqlService::ID_GetUserFavoriteResponse, baseService::ID_LoginResponse });

	addHandleFunc<TcpClient, NetConnection, user::GetFriendsResponse, baseService::LoginResponse>
		(mysqlClient, &BaseServer::handleFriendList,
			{ mysqlService::ID_GetFriendsResponse, baseService::ID_LoginResponse });

	addHandleFunc<TcpClient, NetConnection, user::GetUserGroupsResponse, baseService::LoginResponse>
		(mysqlClient, &BaseServer::handleGroupList,
			{ mysqlService::ID_GetUserGroupsResponse, baseService::ID_LoginResponse });

	//消息转发请求处理逻辑
	addHandleFunc<TcpServer, NetConnection, baseService::SendMsgRequest, lbService::RepostMsgRequest>
		(tcpServer, &BaseServer::handleSendMsgRequest,
			{ baseService::ID_SendMsgRequest, lbService::ID_RepostMsgRequest });
	
	//消息通知请求处理逻辑（lb服务器给目的服务器让其转发消息的）
	addHandleFunc<UdpClient, Udp, lbService::RepostMsgResponseTo, baseService::MsgNotify>
		(lbClient, &BaseServer::handleMsgNotify,
			{ lbService::ID_RepostMsgResponseTo, baseService::ID_MsgNotify });

	//注册请求处理逻辑
	addHandleFunc<TcpServer, NetConnection, baseService::RegisterRequest, mysqlService::InsertUserRequest, mysqlService::RegisterGroupRequest>
		(tcpServer, &BaseServer::handleRegister,
			{ baseService::ID_RegisterRequest, mysqlService::ID_InsertUserRequest, mysqlService::ID_RegisterGroupRequest });

	//注册用户结果返回逻辑，这个一定要有，这个函数其实只处理其中的3，删除和修改不归这个函数管理
	addHandleFunc<TcpClient, NetConnection, mysqlService::UpdateUserResponse, baseService::RegisterResponse>
		(mysqlClient, &BaseServer::handleRegisterUserResult,
			{ mysqlService::ID_UpdateUserResponse, baseService::ID_RegisterResponse });

	//下线通知
	addHandleFunc<TcpServer, NetConnection, baseService::LogoutRequest, mysqlService::ModifyUserRequest, lbService::StopServiceRequest>
		(tcpServer, &BaseServer::handleLogout,
			{ baseService::ID_LogoutRequest, mysqlService::ID_ModifyUserRequest, lbService::ID_StopServiceRequest });

	//添加请求处理逻辑
	addHandleFunc<TcpServer, NetConnection, baseService::AddRelationsRequest, mysqlService::InsertFriendRequest, mysqlService::AddUserToGroupRequest>
		(tcpServer, &BaseServer::handleAddRelations,
			{ baseService::ID_AddRelationsRequest, mysqlService::ID_InsertFriendRequest, mysqlService::ID_AddUserToGroupRequest });

	//添加喜欢请求
	addHandleFunc<TcpServer, NetConnection, baseService::AddFavoriteRequest, mysqlService::InsertFavoriteRequest>
		(tcpServer, &BaseServer::handleAddFavorite,
			{ baseService::ID_AddFavoriteRequest, mysqlService::ID_InsertFavoriteRequest });//不需要返回

	//消息通知请求处理逻辑（lb服务器给目的服务器让其转发消息的）
	addHandleFunc<UdpClient, Udp, lbService::RepostMsgResponseFrom>
		(lbClient, &BaseServer::handleRepostMsgResponse,
			{ lbService::ID_RepostMsgResponseFrom });

	//这个别忘记，mysql客户端需要主动连接
	mysqlClient->connectServer();
}

BaseServer::~BaseServer() {
}

void BaseServer::run() {
	std::thread startLbClient(&UdpClient::run, lbClient);
	std::thread startMysqlClient(&TcpClient::run, mysqlClient);
	std::thread startTcpServer(&TcpServer::run, tcpServer);
	//保证main阻塞在这，回收对应的线程，否则主线程一退出，就回收这三个线程
	startLbClient.join();
	startMysqlClient.join();
	startTcpServer.join();
}

#if 0
//所有的消息都会到这里来，由这个分流出去，这个函数模板会进行多次注册（包括tic或者udp的）（为什么可以合并呢，因为不管是udp还是tcp他们的流程都是一样的）
template<class Conn_T, class Request_T, class Response_T>
void BaseServer::requestHandle(Conn_T* conn, void* userData)
{
	Debug("requestHandle开始执行");
	//解析data数据（接收数据已经交给底层tcpserver了）
	Request_T requestData;
	requestData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);

	//取出一条连接根据解析的数据进行查询,并解析数据,并将数据放到responseData里
	//路由的响应消息
	Response_T responseData;
	Dealing(conn, &requestData, &responseData);  //requestsql注意内部函数是requestsql而不是requestSql
}

//公共处理函数
template<class Conn_T, class Request_T, class Response_T>
void BaseServer::Dealing(Conn_T* clientConn, Request_T* requestData, Response_T* responseData)
{
	//根据解析出来的数据进行逻辑处理，填充responseData，然后发送数据给对端
	//所有的逻辑处理都走这个函数的重载
	int responseType = 0; //这个返回的消息类型在经过analyzeRequest会被设置为正确的值
	analyzeRequest(requestData, responseType, responseData);
	
	//这个packageAndResponse放在这并不妥当，注意和mysql服务器的区别，因为mysql服务器必须要等到结果才能进行下一步，
	//也就是他的逻辑处理过程肯定能组装好响应数据，所以可以顺序执行，但是这里的逻辑处理函数analyzeRequest就不一定了，
	//他可能需要等待mysql服务器的数据发过来，才能进行下一步的处理，这样的话，analyzeRequest其实只是在请求一个结果，然后等待触发接下来的动作（函数）
	//但是mysql代理服务器将packageAndResponse放在这主要是考虑取得的mysql连接尽可能快的释放，而不需要等到响应完数据才发送
	//那这个时候在这里responseData就不能定义在栈上了，需要定义在堆上，因为如果定义在栈上，那么这些结果执行完就释放掉了，analyzeRequest不一定，
	//但是为了统一，可以一起定义在堆上，等到响应完直接释放掉
	// 但是怎么传递requestData和responseData和responseType呢，而且这里需要回复两个数据，一个是客户端一个是lb服务器端
	// 关于回复两个包的问题，应该是这个连接是谁的就给谁回复，其他的回复应该放在逻辑处理里面
	// 关于上面三个值怎么传递的问题，首先responseType放在这是没有意义的，因为packageAndResponse已经不存在了，完全可以放到packageAndResponse里赋值
	// 那如果responseType都没有了 analyzeRequest这个还有意义吗，没有了把，不可能一个函数里只调用另一个函数把，所以可以直接在Dealing函数里执行一些流程
	// 那么Dealing就不应该是模板了，而应该是重载
	// 比如登录的时候，可以直接在这发包请求基础信息，那还是涉及到传递的问题
	// 其实来一个请求之后，对应的响应消息和响应类型也就知道了，所以模板函数的参数完全可以使用request来确定，而不需要response，
	// 那么在这里进行第一步操作完之后如果需要发送，可以在这里定义发送类型，和定义response变量和执行打包发送函数，如果还需要其他流程，
	// 那么可以直接使用void* data传递request变量，完美解决值传递的问题，所以这里和mysqlAgent的区别是只通过请求类型作为模板参数，不过核心思路是不变的
	// mysqlAgent那里需要在“Dealing”里执行一个请求和响应，这个相当于他的逻辑处理，然后由于其肯定是逻辑处理完就可以发送响应了，所以在这里可以直接packageAndResponse
	// 但是基础服务器，很大的一点是逻辑处理完可能还要处理可能还没有开始封装响应包，所以packageAndResponse放在这里不合适，需要放在逻辑处理函数里
	// 并且由于逻辑处理函数里可能还需要request的数据，所以还涉及到值传递的问题，为了尽可能的少传递值，所以可以省略掉响应的包，在合适的时机在创建更好
	// 但是好像request通过userData不能传递，首先这个值是刚开始注册的时候就有了地址，而不是和连接绑定在一起，而且不能频繁更改userData的值，因为假如有两个客户同时过来
	// 进行登录，（不管是不是在两个线程里）那么势必要连续执行两个DealingRequest，如果在执行DealingRequest的时候指定了userdata为requestdata
	// （如果不指定，后面就没机会告诉handleGetUserBaseResponse关于requestdata的信息了），那么后一个指定的值会覆盖前一个指定的值
	// 那么第一个连接请求的mysql数据就和第二个包进行比对了，显然是不正确的，这个时候其实handleGetUserBaseResponse更应该是去当前连接里找对应的request
	// 这样就不用担心覆盖数据的问题了，这就用到了之前实现的连接属性了，也就是让conn的
	// 但是好像没那么简单，因为好像在接收到mysql服务器发回来的数据之后，实际上是在mysqlclient的那个线程执行的，
	// 这个时候其实是需要切换到tcpserver对应的线程的（之前的conn发完一个mysql请求就休眠了，所以需要唤醒），并将对应的数据拷贝过去，并在那里进行比较
	// 那怎么唤醒指定的线程呢？那就需要handleGetUserBaseResponse有特殊的标记来记录对应的conn
	// 注意，mysql客户端实际上是只有一个conn，当这个客户端请求的时候，记录下对应的conn和请求的包的关系，然后唤醒mysqlh之后，唤醒的是mysql发送请求的包的那个线程
	// 所以这时应该找到响应的包和请求的包之间的关系，然后再找到conn，然后把响应的数据交给他，然后唤醒
	// 
	// 编程本质上就是信息流的传递过程，在设计编程时，应该考虑信息（内存）什么时候传递到另一个线程，什么时候该销毁这个信息，
	// 就好像编程是拿线把信息串起来，比如上面的客户端的request数据怎么保存，以及mysql的响应函数怎么唤醒指定的conn，就是寻找之间的信息交织点，
	// 然后一步一步找到对应的conn，唤醒，传递，好的程序不会有冗余的信息传递路经
	//packageAndResponse(clientConn, responseType, responseData);
}
#endif

//为了能通过函数名显著的区分其功能，将requestHandle放到DealingRequest里面， 之前都是DealingRequest放到requestHandle里面
//并将requestHandle改为parseRequest,，这相当于是对lbServer和mysql模式的一个改进，内部也进行了一些改进


//处理登录请求--拉取个人信息请求
//这个Request_T是为了指出handleLogin处理的接收对象是什么类型，必须要给出，否则没法注册
//登录请求直接在内部给出就行
void BaseServer::handleLoginStep1(NetConnection* clientConn, void* userData)
{
	auto requestData = parseRequest<NetConnection, baseService::LoginRequest>(clientConn);   //这里的Request_T是在parseRequest返回时用的,requestData为智能指针

	//根据解析出来的数据进行逻辑处理，填充responseData，然后发送数据给对端
	//查询用户名和密码
	mysqlService::GetUserBaseRequest GetUserBaseData;
	GetUserBaseData.set_uid(requestData->uid());  //查询用户的基本个人信息
	//根据上面的分析，需要将clientConn和请求的包做一个绑定
	//或者这个时候根据请求的包生成一个随机数，让这个随机数和conn关联起来
	//或者干脆将地址clientConn传递过去，解析出来之后直接使用也可以
	//如果传递地址过去，这样耦合性就很强了（因为mysql服务器在设计的时候不需要考虑地址的关系，它只需要负责发回数据就可以了），
	//不太好，那就是只能发送请求之前记录这个请求和tcpSercer的conn的关系了
	//但是想了一下假如我这个mysqlClient发出了大量的查询喜欢列表的请求，那么baseServer该怎么区分，那是登录之后的事情了，可以通过uid区分，因为uid唯一和conn绑定
	//所以登录的问题就很棘手，因为登录的uid可能一样
	//登录的id到这里肯定不会相同的，因为lb服务器那里如果是登录请求，会判断当前uid是否在线，如果是就直接不返回ip，拒绝登录，所以这里可以不用理会uid重复的问题
	//所以这里可以直接将conn的连接属性暂时指定为这个requestData，然后使用map记录uid和conn的关系就完成了信息流的连接（请求数据库基础信息前后）
	//那么连map都可以不用的，当然了如果想要快速查找可以使用map
	// 我感觉这里还是要用map，这样可以快熟找到，要不然使用底层的m_Fd_TcpConn会比较麻烦，m_Fd_TcpConn这个就只是限制人数吧，
	Debug("uid_conn的大小：%d", uid_conn.size());
	Debug("当前用户uid情况：");
	for (auto & key_vlaue: uid_conn) {
		std::cout << key_vlaue.first << std::endl;
	}
	assert(uid_conn.find(requestData->uid()) == uid_conn.end());
	uid_conn[requestData->uid()] = clientConn;
	//requestData是智能指针，所以不能直接指向智能指针，因为他只在这个函数域有效，过了这个函数域就失效了
	auto loginRequest = new baseService::LoginRequest(*requestData.get());
	clientConn->m_dymaicParam = (void*)loginRequest;   //让当前连接记住这个客户端的请求
	//注意这里不是响应对端，而是给mysql发数据
	//这里不再需要指定发送类型了，直接指定服务器和发送消息体即可
	tcpSendMsg<mysqlService::GetUserBaseRequest>(mysqlClient, &GetUserBaseData);
}

//处理登录请求--拉取个人信息响应
//NetConnection* conn是mysql的，而上面的NetConnection* clientConn是tcpServer的
void BaseServer::handleLoginStep2(NetConnection* conn, void* userData) {

	auto userInfoData = parseRequest<NetConnection, mysqlService::GetUserBaseResponse>(conn);

	baseService::LoginRequest* requestData = (baseService::LoginRequest*)uid_conn[userInfoData->uid()]->m_dymaicParam;
	lbService::ServerResonseToLb responseDataToLb;  //给lb服务器回复的消息
	baseService::LoginResponse loginResData;  //不用new可以直接发送.给客户端回复的消息
	//先假设是错误的，然后如果正确，再更改
	responseDataToLb.set_modid(1);
	responseDataToLb.set_originid(requestData->uid());
	responseDataToLb.set_finalid(requestData->uid());
	responseDataToLb.set_result(-1);
	//先假设登录失败
	loginResData.set_uid(requestData->uid());
	loginResData.set_result(-1);
	if (userInfoData->isgot() == 1) {
		//获取当前user成功，进行比对
		if (userInfoData->uid() == requestData->uid() && userInfoData->passwd() == requestData->passwd()) {
			//比对成功，那么就代表登录成功，那么根据上面的分析，可以给当前的conn的连接属性绑定为当前的用户
			//注意绑定之前，看看m_dymaicParam是否需要释放
			delete requestData;
			uid_conn[userInfoData->uid()]->m_dymaicParam = new UserData(userInfoData->uid());  //注意这里转为void*不需要显示的转化，但是反过来却需要进行显示的转换
			//然后结构体内部也有一个动态指针，可以让LoginResponse存在那里，这样就保证了信息的连续性了
			//那么下面的各种列表请求，都可以通过uid找到目标conn了
			//组装LoginResponse，这个又需要多次请求mysql，多次组装
			//组装逻辑太复杂了，我直接使用oneof作为response，那么我就可以返回4次loginResponse，而不是一次LoginResponse
			//至此，就解决了所有的客户端登录问题了
			//但是发现了一个大问题，就是基础服务器的协议如果提供全面的话，和mysql服务器的协议大量的重复（特别是查询的服务，因为返回的东西很相似），把那些查询的
			//都放到user.proto里面
			//不能用oneof，因为如果用oneof服务器端将会有冗余的代码，拉取信息流应该有独立的函数（而不是在这个login函数里），一个是拉取信息流ID的，一个是loginID的
			//返回登录结果
			loginResData.set_result(1);
			//发消息给lb服务器告诉登录成功
			responseDataToLb.set_result(1);

			//获取喜欢列表，然后由响应函数返回结果
			//这里需要指定出来发送的类型，因为这不是默认发送的类型
			getUserInfoList<user::GetUserFavoriteRequest>(requestData->uid(), (int)mysqlService::ID_GetUserFavoriteRequest);
			//获得朋友列表，然后由响应函数返回结果
			getUserInfoList<user::GetFriendsRequest>(requestData->uid(), (int)mysqlService::ID_GetFriendsRequest);
			//获得组列表，然后由响应函数返回结果
			getUserInfoList<user::GetUserGroupsRequest>(requestData->uid(), (int)mysqlService::ID_GetUserGroupsRequest);
			//通过上面三个的响应函数组装完成LoginResponse，然后发送
			//但是这里又涉及到线程间信息传递的问题了，
			//并且还需要解决我怎么确定三个消息都组装到里面这个问题（这个问题可以通过UserInfo的size判断三个列表值是否都不为空进行判断）

			//同步数据库（改为在线）
			modifyUserStatus(requestData->uid(), 1);
		}
	}
	//不管有没有登录成功，该有的数据都准备好了，可以将数据发给两端了
	//返回结果给客户端
	auto targetPair = uid_conn.find(userInfoData->uid());
	assert(targetPair != uid_conn.end());
	auto targetConn = targetPair->second;
	tcpSendMsg<baseService::LoginResponse>(targetConn, &loginResData);
	//返回结果给负载均衡服务器告诉登录情况
	udpSendMsg<lbService::ServerResonseToLb>(&responseDataToLb, &lbClient->m_recvAddr);
	//如果登录失败，清理map，关掉当前连接
	if (loginResData.result() == -1) {
		//应该给关闭连接增加一个接口
		targetConn->m_evloop->addTask(targetConn->m_evloop, targetConn->m_evloop->packageTask(targetConn->m_channel, TaskType::DEL));
		uid_conn.erase(userInfoData->uid());
	}
}

//将喜欢列表，朋友列表，组列表响应封装为一个模板，因为逻辑都是一样的，就是找到对应的uid，然后发送给客户端,不过注册的时候还是要注册3个
//不能封装成模板，因为下面的if (typeid(Request_T) == typeid(user::GetUserFavoriteResponse))是编译期间就会生成的，会编译不通过后面的elseif
//可以使用编译期if，但是那要到c++17，我并不想支持那么高的版本
#if 0 
template<class Request_T>
void BaseServer::handleUserInfoList(NetConnection* mysqlClient, void* userData) {
	//userInfo是智能指针
	auto userInfo = parseRequest<NetConnection, Request_T>(mysqlClient);   //Request_T是在parseRequest返回时用的，注意客户端 基础服务器 mysql用的这些列表是同一个
	//封装响应
	baseService::LoginResponse response;
	//这还要判断类型。。。。。折腾了一天，加了消息类型记录类，修改了协议，结果发现还是要判断。。。。
	//这个太罗嗦了，一定要改进
	if (typeid(Request_T) == typeid(user::GetUserFavoriteResponse)) {
		//protobuf并没有提供简单的赋值函数，这样写是不对的：response.set_favoritelist(*userInfo.get());
		//可以使用GetUserFavoriteResponse重载的=，如果还不行，自己可以实现一个for
		//for (int i = 0; i < userInfo.file_size(); i++)
		//下面已经没有错了
		//response.favoritelist() = * userInfo.get();   错误编译不通过
		//response.favoritelist().Swap(userInfo.get());//这个swap方法很好，所以如果不知道怎么操作，一定要读头文件
		//但是智能指针返回的是const，编译不通过（Swap的参数不能是const）：将‘const user::GetUserFavoriteResponse’作为‘void user::GetUserFavoriteResponse::Swap(user::GetUserFavoriteResponse*)’
		//response.favoritelist() = *userInfo; //这个和* userInfo.get()等价，也会出现上面这个编译错误
		//response.favoritelist().CopyFrom(*userInfo);  //copyFrom返回的是void，并且也有const，但是这个也编译失败
		response.favoritelist()(*userInfo);  //拷贝构造
	}
	else if (typeid(Request_T) == typeid(user::GetFriendsResponse)) {
		response.friendlist() = *userInfo;
	}
	else if (typeid(Request_T) == typeid(user::GetUserGroupsResponse)) {
		response.grouplist() = *userInfo.get();
	}
	else {
		std::cout << "error" << std::endl;
		return;
	}
	//找到对应的conn准备发送
	auto clientPair = uid_conn.find(userInfo->uid());
	if (clientPair != uid_conn.end()) {
		//原封不动的发给对端
		//但是使用模板的话，发送id成了问题，因为外界没法传进来id，userInfo需要注册的时候就传进来数据，这显然需要给类添加一个属性
		//其实这个信息内部是可以知道的，但是用ifelse太啰嗦
		//怎么办，可以考虑将lbServeice的那个类型标志类给移植过来，然后稍加改造，这样注册的时候就不用转，直接注册的时候传递类型即可，就可以直接
		//避免事件id冲突了（因为可能支持不同的proto，这些id可能冲突）
		//但是发送的id还是要手动添加，也就是移植过来之后，还是不行的，发送的时候还是需要手动指出发送类型，这该怎么办，
		//可以这样操作：记录id的类，里面增加一个结构体，然后记录对应的默认响应函数id，如果需要回复不是默认的id，那么发消息的时候主动标明即可
		//可以是可以，但是返回的id到底是啥呢，客户端不可能包括mysql的协议的，那么你传递回去的id到底是啥，难道还要在基础服务器增加3个id，显然不合适
		//这该咋办，如果一次发三个请求，然后组合成一个请求，还需要消息缓存机制
		//修改协议体LoginResponse，原来的协议体太简单了，只有一个result，这个时候应该增加oneof，将所有的朋友，组，喜欢列表放进去
		//但是这又带来了另一个问题，就是如果添加，肯定也要更新，那咋办？不会在add朋友的响应下增加朋友的响应列表吧，
		//这势必会要求消息队列的缓存机制，因为相当于2个 请求才组织一个响应
		//那就是在这里result不由第一个请求执行的反馈执行，而是由第二个获取列表的响应执行？因为确实第一条仅代表执行了，执行效果咋样，不得而知
		//那这样的话登录反馈改咋办，因为登录的结果确实是查询了mysql并比较了的，只能增加一个变量标志当前是哪个消息体了，那oneof里装4个结果一个result，3个列表
		//所以修改协议吧
		//修改完之后，就可以确定返回类型了
		//tcpSendMsg<Request_T>(clientPair->second, &response);  //肯定不能发Request呀，直接编译失败，会推断错误
		tcpSendMsg<baseService::LoginResponse>(clientPair->second, &response);  //这里先不指定统一的id即可，因为考虑到朋友等列表响应，默认的id应该指定为类似刷新动作，但是因为没有，所以暂时指定为这个
	}
	//如果没有进入if，那么说明有问题
}
#endif

//喜欢列表响应
void BaseServer::handleFavoriteList(NetConnection* mysqlClient, void* userData) {
	//userInfo是智能指针
	auto userInfo = parseRequest<NetConnection, user::GetUserFavoriteResponse>(mysqlClient);   //Request_T是在parseRequest返回时用的，注意客户端 基础服务器 mysql用的这些列表是同一个
	//封装响应
	baseService::LoginResponse response;
	response.set_uid(userInfo->uid());
	//这还要判断类型。。。。。折腾了一天，加了消息类型记录类，修改了协议，结果发现还是要判断。。。。
	//这个太罗嗦了，一定要改进
	
	//protobuf并没有提供简单的赋值函数，这样写是不对的：response.set_favoritelist(*userInfo.get());
	//可以使用GetUserFavoriteResponse重载的=，如果还不行，自己可以实现一个for
	//for (int i = 0; i < userInfo.file_size(); i++)
	//下面已经没有错了
	//response.favoritelist() = * userInfo.get();   错误编译不通过
	//response.favoritelist().Swap(userInfo.get());//这个swap方法很好，所以如果不知道怎么操作，一定要读头文件
	//但是智能指针返回的是const，编译不通过（Swap的参数不能是const）：将‘const user::GetUserFavoriteResponse’作为‘void user::GetUserFavoriteResponse::Swap(user::GetUserFavoriteResponse*)’
	//response.favoritelist() = *userInfo; //这个和* userInfo.get()等价，也会出现上面这个编译错误
	//response.favoritelist().CopyFrom(*userInfo);  //copyFrom返回的是void，并且也有const，但是这个也编译失败
	//response.favoritelist()(*userInfo);   //为什么上面一直不能编译通过呢，是因为favoritelist返回的是不能更改的。。。。。
	//而且上面编译不通过，可以使用has_xxx这样就可以编译通过了,肯定不行，mysql发的都是GetUserFavoriteResponse之类的，里面根本没有has_
	//response.mutable_favoritelist()->CopyFrom(*userInfo);//竟然能编译通过。。。。。。。。。。。不可思议，还可以使用下面这种。。。。我感觉交换更好，但是不知道对智能指针有没有影响
	auto clientPair = uid_conn.find(userInfo->uid());
	response.mutable_favoritelist()->Swap(userInfo.get());   //先查找再交换，因为一交换，原来的数据就没了，就找不到发不了
	if (clientPair != uid_conn.end()) {
		Debug("喜欢列表开始发给客户端");
		tcpSendMsg<baseService::LoginResponse>(clientPair->second, &response); 
	}
	//如果没有进入if，那么说明有问题
}

//朋友列表响应 不能和上面合并，因为if的原因（已在上面和bug中记录），可以考虑更新协议，采用any，这样可能可以合并
void BaseServer::handleFriendList(NetConnection* mysqlClient, void* userData) {
	auto userInfo = parseRequest<NetConnection, user::GetFriendsResponse>(mysqlClient);
	baseService::LoginResponse response;
	response.set_uid(userInfo->uid());
	auto clientPair = uid_conn.find(userInfo->uid());
	response.mutable_friendlist()->Swap(userInfo.get());
	if (clientPair != uid_conn.end()) {
		Debug("朋友列表开始发给客户端");
		tcpSendMsg<baseService::LoginResponse>(clientPair->second, &response);
	}
}

//组列表响应
void BaseServer::handleGroupList(NetConnection* mysqlClient, void* userData) {
	Debug("正在执行组列表响应");
	auto userInfo = parseRequest<NetConnection, user::GetUserGroupsResponse>(mysqlClient);
	baseService::LoginResponse response;
	response.set_uid(userInfo->uid());
	auto clientPair = uid_conn.find(userInfo->uid());
	response.mutable_grouplist()->Swap(userInfo.get());
	if (clientPair != uid_conn.end()) {
		Debug("组列表开始发给客户端");
		tcpSendMsg<baseService::LoginResponse>(clientPair->second, &response);
	}
}

//喜欢列表请求，朋友列表请求，组列表请求
template<class Request_T>
void BaseServer::getUserInfoList(int uid, int requestId) {
	Request_T GetUserInfoData;
	GetUserInfoData.set_uid(uid);
	//这里需要指定出来发送的类型，因为这不是默认发送的类型
	tcpSendMsg<Request_T>(mysqlClient, &GetUserInfoData, requestId);
}

//处理消息发送请求
void BaseServer::handleSendMsgRequest(NetConnection* conn, void* userData)
{
	auto sendMsgRequest = parseRequest<NetConnection, baseService::SendMsgRequest>(conn);//Request_T是在parseRequest返回时用的，注意客户端 基础服务器 mysql用的这些列表是同一个
	lbService::RepostMsgRequest repostRequest; //先定义给lb服务器的转发请求
	auto userInfo = (UserData*)conn->m_dymaicParam;
	//返回给客户端的消息，如果是私聊需要判断对端有没有消息接收成功（如果在当前服务器直接返回成功，如果在其他服务器需要lb服务器通知，才能返回成功），
	//如果是群聊，直接返回发送成功，因为不用关心有没有有多少人在线
	baseService::SendMsgResponse sendMsgResponse;
	sendMsgResponse.set_fromid(sendMsgRequest->toid());
	sendMsgResponse.set_result(-1);
	//先写单对单的把
	if (sendMsgRequest->modid() == 1) {  //是私聊
		//先在这个服务器找找`
		auto target = uid_conn.find(sendMsgRequest->toid());
		if (target != uid_conn.end()) {
			//封装一个MsgNotify发给某个conn
			baseService::MsgNotify msgNotify;
			msgNotify.set_modid(1);
			msgNotify.set_fromid(userInfo->m_uid);
			msgNotify.set_msg(sendMsgRequest->msg());
			msgNotify.set_toid(sendMsgRequest->toid());
			tcpSendMsg<baseService::MsgNotify>(target->second, &msgNotify, baseService::ID_MsgNotify);//需要指定发送ID，因为默认的发送id是回复给原始的客户端发送情况的
			//在写客户端的时候需要知道到底有没有发送成功，所以需要回复客户端
			sendMsgResponse.set_result(1);
			tcpSendMsg<baseService::SendMsgResponse>(conn, &sendMsgResponse, baseService::ID_SendMsgResponse);//需要指定发送ID，因为默认的发送id是回复给原始的客户端发送情况的
			return;
		}
		else {
			//封装一个RepostMsgRequest发给lb
			//但是发现MsgNotify和RepostMsgRequest的格式一样，这个重复（因为发现基础服务器和另外两种服务器的协议有大量冲突的）的问题该怎么解决
			//其实仔细分析了一下，客户端在请求列表之类的时候是需要一起请求的，而在删除增加之类的，需要服务器这边帮忙请求一次，所以并没有那么强的重复度
			//但是我现在没有打算增加刷新功能，而是再增加删除等的时候服务器端主动帮忙刷新一下
			//值得提醒的一点是mysql本身是没有用到ID_XXXResponse的，都是request的
			repostRequest.set_modid(1);
		}
	}
	else if(sendMsgRequest->modid() == 2){  //群聊
		//直接把消息转发给lb，因为
		repostRequest.set_modid(2);
		sendMsgResponse.set_result(1);
		tcpSendMsg<baseService::SendMsgResponse>(conn, &sendMsgResponse, baseService::ID_SendMsgResponse);
	}
	//注意，群组和私聊的格式差不多，这里的共同部分就放在这里一起了
	repostRequest.set_fromid(userInfo->m_uid); //conn的动态属性绑定的用户
	repostRequest.set_msg(sendMsgRequest->msg());
	repostRequest.set_toid(sendMsgRequest->toid());
	//到了这里不管是群发消息还是私聊转发到其他服务器，发给lb服务器的已经准备好了
	udpSendMsg<lbService::RepostMsgRequest>(&repostRequest, &lbClient->m_recvAddr, (int)lbService::ID_RepostMsgRequest);
}

void BaseServer::handleRepostMsgResponse(Udp* lbClient, void* userData) {
	auto repostMsgResponse = parseRequest<Udp, lbService::RepostMsgResponseFrom>(lbClient);
	baseService::SendMsgResponse sendMsgResponse;
	sendMsgResponse.set_result(repostMsgResponse->result());
	sendMsgResponse.set_fromid(repostMsgResponse->toid());  //哪个朋友给的确认收到
	//找到对应的conn
	auto target = uid_conn.find(repostMsgResponse->fromid());
	assert(target != uid_conn.end());
	tcpSendMsg<baseService::SendMsgResponse>(target->second, &sendMsgResponse, baseService::ID_SendMsgResponse);
}

//处理消息通知请求
//lb服务器在通知当前服务器处理消息转发时，需要调用该函数，将对应的消息发送到对应的客户端上
void BaseServer::handleMsgNotify(Udp* lbClient, void* userData)
{
	Debug("正在处理lb服务器要求转发的消息");
	auto msgNotifyRequest = parseRequest<Udp, lbService::RepostMsgResponseTo>(lbClient);   //Request_T是在parseRequest返回时用的，注意客户端 基础服务器 mysql用的这些列表是同一个
	//注意这里可能是私聊转发，也可能是群转发
	//注意这里负载均衡服务器在接到一个请求之后，可能会发多个包给多台服务器
	baseService::MsgNotify msgNotify;
	msgNotify.set_modid(msgNotifyRequest->modid());
	msgNotify.set_fromid(msgNotifyRequest->fromid());
	//msgNotify的toid如果是群发，那么存的是gid，如果是私聊，存的是目标uid（私发不存也没啥，因为已经发到客户端了）
	if (msgNotify.modid() == 2) {
		msgNotify.set_toid(msgNotifyRequest->gid());  //即使是私聊也可以，因为私聊的话是0，当然不放心可以对这个语句加个if
	}
	for (int i = 0; i < msgNotifyRequest->toid_size(); i++) {
		int toId = msgNotifyRequest->toid(i);
		auto targetClient = uid_conn[toId];
		if (msgNotify.modid() == 1) {
			msgNotify.set_toid(toId);
		}
		for (int i = 0; i < msgNotifyRequest->msg_size(); i++) {
			msgNotify.set_msg(msgNotifyRequest->msg(i));   //注意这里的赋值方式
			//msgNotify.msg() = msgNotifyRequest->msg(i); 错的
			tcpSendMsg<baseService::MsgNotify>(targetClient, &msgNotify);
		}
	}
}

//处理注册账号请求
void BaseServer::handleRegister(NetConnection* conn, void* userData)
{
	//注册时不应该客户端生成随机uid，有可能不同的客户端uid一样，这个工作应该是lb服务器生成，也不对，那lb服务器的呢
	//还是应该客户端提供，如果注册的时候提供的uid，刚好lb服务器那里有人在登录，那么就返回注册失败，客户端再重新发送一次注册请求即可
	auto registerRequest = parseRequest<NetConnection, baseService::RegisterRequest>(conn);  //baseService::RegisterRequest

	if (registerRequest->modid() == 1) { //1代表请求注册用户
		conn->m_dymaicParam = new UserData(registerRequest->id());  //关于这个的属性释放都是底层在做的,注意注册群的时候并不需要new
		//注册一个用户
		mysqlService::InsertUserRequest insertUserData;
		insertUserData.set_uid(registerRequest->id());  //这个uid模拟随机生成的一个id，或者是生成uid的工作交给mysql，这里就先使用临时的id，如果是这里随机生成，
		//那么后续不用修改uid_conn，但是不能保证id一定可以插入成功了，看了一下mysql的注册逻辑，发现这里只能随机生成一个值，因为mysql那里不考虑你的uid
		//而且mysql那里默认注册就登陆，我又改了成不登录了
		uid_conn[registerRequest->id()] = conn; //注意这个id后面响应完还要用,要找到conn
		insertUserData.set_name(registerRequest->name());
		insertUserData.set_passwd(registerRequest->passwd()); 
		tcpSendMsg<mysqlService::InsertUserRequest>(mysqlClient, &insertUserData);
	}
	else if(registerRequest->modid() == 2) {
		auto userInfo = (UserData*)conn->m_dymaicParam;
		//2代表请求注册群组
		//就插入组数据，并且将当前组的成员更新为1个，也就是申请者的）
		mysqlService::RegisterGroupRequest registerGroupData;
		registerGroupData.set_uid(userInfo->m_uid);  //群主uid
		registerGroupData.set_name(registerRequest->name());
		registerGroupData.set_summary(registerRequest->summary());
		tcpSendMsg<mysqlService::RegisterGroupRequest>(mysqlClient, &registerGroupData);
		//查询结果，注意不应该数据库主动返回，因为数据库代理服务器只提供增删查改，不负责业务上面的东西，所以不能添加完成之后，让mysql返回一个uid，不过操作函数
		//本身带有返回值，可以初步判断插入是否成功
		//用户注册就不用刷新了，因为有个回调函数帮助返回结果
		//获得组列表，然后由响应函数返回结果
		//这样真的可以吗，因为这相当于一个包包含两个请求，应该需要等待返回result吧，可能是个隐患，但是现在先不管了，先这样写吧
		user::GetUserGroupsRequest GetUserGroupsData;
		GetUserGroupsData.set_uid(userInfo->m_uid);
		tcpSendMsg<user::GetUserGroupsRequest>(mysqlClient, &GetUserGroupsData, (int)mysqlService::ID_GetUserGroupsRequest);
	}
	else {
		//错误
		std::cout << "error" << std::endl;
	}
}

//注册响应，一定要有这个的，因为客户端最终需要获得一个uid，就相当于一个qq号
//返回给客户端注册用户的情况（注意，群组的不处理，因为群组的刷新一下就行了）
void BaseServer::handleRegisterUserResult(NetConnection* conn, void* userData)
{
	//这里需要三个动作，一是更新当前服务器的状态，主要是uid_conn，另一个是是反馈给lb服务器的，还有就是反馈给客户端的
	Debug("正在返回注册用户结果");
	auto registerResponse = parseRequest<NetConnection, mysqlService::UpdateUserResponse>(conn);  //mysqlService::UpdateUserResponse* registerResponse

	//注意，这个UpdateUserResponse包含3个操作，我们只处理updateKind为3的，其他的忽略
	if (registerResponse->updatekind() == 3) {
		//更新uid_conn, 但是mysql后端没有保存随机的id，需要更改一下cpp文件，和协议，不过不用改message，只需要修改返回的result，让他注册的时候返回注册的id，uid还是随机id即可
		auto targetClient = uid_conn[registerResponse->uid()];
		auto userInfo = (UserData*)targetClient->m_dymaicParam;

		baseService::RegisterResponse response;
		lbService::ServerResonseToLb responseToLb;
		responseToLb.set_modid(1);
		responseToLb.set_originid(registerResponse->uid());
		responseToLb.set_finalid(registerResponse->result());
			
		response.set_modid(1);
		//如果mysql注册失败，result会和随机的uid相同，所以这里做个判断
		if (registerResponse->uid() == registerResponse->result()) {
			Debug("注册失败");
			responseToLb.set_result(-1);
			response.set_result(-1);
			//还需要删除当前连接的连接属性,这应该归reactor底层做，所以这里不处理
			//停止服务
			targetClient->m_evloop->addTask(targetClient->m_evloop, targetClient->m_evloop->packageTask(targetClient->m_channel, TaskType::DEL));
		}
		else {
			Debug("注册成功");
			Debug("返回的uid：%d", registerResponse->result());
			responseToLb.set_result(1);
			response.set_result(1);
			//更新一下连接属性最新的uid
			userInfo->m_uid = registerResponse->result();
			//为什么要添加新的，删除旧的，因为uid变了，key是uid，肯定要删除，value也要更新一下（就是上面的连接属性的值）
			uid_conn[registerResponse->result()] = targetClient;  //注册成功需要插入新的，删除随机的（这个动作在下面）
		}
		uid_conn.erase(registerResponse->uid());   //这里包括注册失败，以及注册成功的，都要删除掉随机的

		response.set_id(registerResponse->result());
		tcpSendMsg<baseService::RegisterResponse>(targetClient, &response);
		//如果注册成功，应该返回给客户端一些top电影列表
		//或者获取一个随机电影列表
		//反馈给lb服务器注册结果，如果注册成功，那就登录成功（更新为最新的uid），如果注册失败，直接踢下线，lb那边也删除掉
		udpSendMsg<lbService::ServerResonseToLb>(&responseToLb, &lbClient->m_recvAddr);
		return;
	}
	std::cout << "该操作不为插入，已忽略该包" << std::endl;
}

//处理添加朋友，组操作
void BaseServer::handleAddRelations(NetConnection* conn, void* userData)
{
	auto addRequest = parseRequest<NetConnection, baseService::AddRelationsRequest>(conn);  //baseService::AddRelationsRequest* addRequest

	auto userInfo = (UserData*)conn->m_dymaicParam;
	if (addRequest->modid() == 1) { //1代表请求添加用户
		//添加朋友关系数据
		mysqlService::InsertFriendRequest insertFriendData;
		insertFriendData.set_uid(userInfo->m_uid);
		insertFriendData.set_friend_id(addRequest->id());
		tcpSendMsg<mysqlService::InsertFriendRequest>(mysqlClient, &insertFriendData);
		//添加完之后，这里帮助请求一下列表
		//获得朋友列表，然后由响应函数返回结果
		user::GetFriendsRequest getFriendsData;
		getFriendsData.set_uid(userInfo->m_uid);
		tcpSendMsg<user::GetFriendsRequest>(mysqlClient, &getFriendsData, (int)mysqlService::ID_GetFriendsRequest);
	}
	else if (addRequest->modid() == 2) {//2代表请求添加群组
		//添加群组成员关系
		mysqlService::AddUserToGroupRequest addUserToGroupData;
		addUserToGroupData.set_uid(userInfo->m_uid);
		addUserToGroupData.set_gid(addRequest->id());
		tcpSendMsg<mysqlService::AddUserToGroupRequest>(mysqlClient, &addUserToGroupData);
		//获得组列表，然后由响应函数返回结果
		user::GetUserGroupsRequest getUserGroupsData;
		getUserGroupsData.set_uid(userInfo->m_uid);
		tcpSendMsg<user::GetUserGroupsRequest>(mysqlClient, &getUserGroupsData, (int)mysqlService::ID_GetUserGroupsRequest);
	}
	else {
		//错误
		std::cout << "请求错误" << std::endl;
	}
}

//其实还应该增加一个下线同步机制，这就需要增加一些协议
void BaseServer::handleLogout(NetConnection* conn, void* userData)
{
	auto logoutRequest = parseRequest<NetConnection, baseService::LogoutRequest>(conn);  //baseService::LogoutRequest
	auto userInfo = (UserData*)conn->m_dymaicParam;
	if (userInfo->m_uid == logoutRequest->uid()) {
		//需要比对一下，如果是当前uid，那么退出登录
		//首先需要修改mysql的状态
		modifyUserStatus(userInfo->m_uid, 0);
		//然后发给lb服务器删除当前uid的服务器信息
		lbService::StopServiceRequest stopServerRequest;
		stopServerRequest.set_uid(userInfo->m_uid);
		udpSendMsg<lbService::StopServiceRequest>(&stopServerRequest, &lbClient->m_recvAddr);
		//清理当前uid_conn的map
		uid_conn.erase(userInfo->m_uid);
		//关闭连接
		conn->m_evloop->addTask(conn->m_evloop, conn->m_evloop->packageTask(conn->m_channel, TaskType::DEL));
	}
}

//还应该添加喜欢操作
void BaseServer::handleAddFavorite(NetConnection* conn, void* userData)
{
	auto addRequest = parseRequest<NetConnection, baseService::AddFavoriteRequest>(conn);  //baseService::AddFavoriteRequest* addRequest

	auto userInfo = (UserData*)conn->m_dymaicParam;
	//添加favorite
	mysqlService::InsertFavoriteRequest insertFavoriteData;
	insertFavoriteData.set_uid(userInfo->m_uid);
	insertFavoriteData.set_fid(addRequest->fid());
	tcpSendMsg<mysqlService::InsertFavoriteRequest>(mysqlClient, &insertFavoriteData);
	//查询favorite
	getUserInfoList<user::GetUserFavoriteRequest>(userInfo->m_uid, (int)mysqlService::ID_GetUserFavoriteRequest);
}

//公共修改用户状态函数
void BaseServer::modifyUserStatus(int uid, int status) {
	mysqlService::ModifyUserRequest ModifyUserData;
	ModifyUserData.set_uid(uid);
	mysqlService::ColmPair* colm;
	colm = ModifyUserData.add_colms();
	colm->set_colmname("`state`");
	colm->set_state(status);
	tcpSendMsg<mysqlService::ModifyUserRequest>(mysqlClient, &ModifyUserData);
}

// 公共解析函数
//requestData是传入传出参数，但如果是传入传出参数，这样的话parseRequest只有一行代码了。。。。
//设计为返回值吧
template<class Conn_T, class Request_T>
std::shared_ptr<Request_T> BaseServer::parseRequest(Conn_T * conn)
{
	Debug("requestHandle开始执行");
	//解析data数据（接收数据已经交给底层tcpserver了）
	//std::shared_ptr<Request_T>(new Request_T) requestData; //错误
	std::shared_ptr<Request_T> requestData (new Request_T);
	requestData->ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	return requestData;  //这里的返回过程值得深究，第一个是以对象的返回类型探讨返回时发生了什么，第二个是作为智能指针有没有其他方法较好的返回
	//这里有两篇文章可以仔细看看 https://www.zhihu.com/question/455599343/answer/1848364211和https://www.veaxen.com/%E5%A4%8D%E6%9D%82%E7%9A%84c%EF%BC%8C%E5%BD%93%E5%87%BD%E6%95%B0%E8%BF%94%E5%9B%9E%E5%AF%B9%E8%B1%A1%E5%88%B0%E5%BA%95%E5%8F%91%E7%94%9F%E4%BA%86%E4%BB%80%E4%B9%88%EF%BC%9F.html
	//以及https://blog.csdn.net/conjimmy/article/details/7474278#:~:text=%E5%BD%93%E5%87%BD%E6%95%B0%E8%BF%94%E5%9B%9E%E5%AF%B9%E8%B1%A1%E6%97%B6%EF%BC%88return,a%EF%BC%89%EF%BC%8C%E4%BC%9A%E8%B0%83%E7%94%A8%E6%8B%B7%E8%B4%9D%E6%9E%84%E9%80%A0%E5%87%BD%E6%95%B0%E7%94%9F%E6%88%90%E4%B8%80%E4%B8%AA%E5%AF%B9%E8%B1%A1%E7%BB%99%E5%A4%96%E9%83%A8%E7%94%A8%EF%BC%8C%E5%90%8C%E6%97%B6%E9%80%80%E5%87%BA%E6%97%B6%E6%8A%8A%E5%86%85%E9%83%A8%E5%AF%B9%E8%B1%A1%E6%9E%90%E6%9E%84
}


//由于udp已经进行了更新（导致tcp和udp的sendMsg调用不一致，所以需要提供两个发送接口）
//公共打包函数
template<class Conn_T, class Response_T>
bool BaseServer::packageMsg(Conn_T* conn, Response_T* responseData, int responseID)
{
	Debug("requestHandle开始回复数据");
	//responseData的结果序列化
	std::string responseSerial;  //注意我让conn的response的data指针指向responseSerial是完全没问题的，因为sendMsg函数整个过程，responseData都是有效的，因为这个函数还没执行完
	responseData->SerializeToString(&responseSerial);
	//封装response
	auto response = conn->m_response;
	if (responseID == -1) {
		response->m_msgid = msgIdRecoder->getMsgId<Response_T>();
		if (response->m_msgid == -1)return false;
	}
	else {
		response->m_msgid = responseID;
	}
	response->m_data = &responseSerial[0];
	response->m_msglen = responseSerial.size();
	response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
	return true;
}


//由于UdpReply可能需要设置地址，而conn没有m_addr，所以需要对packageAndResponse进行一层包装
template<class Response_T>
void BaseServer::udpSendMsg(Response_T* responseData, sockaddr_in* to, int responseID)
{
	if (packageMsg<Udp, Response_T>(lbClient, responseData, responseID)) {
		lbClient->sendMsg(*to);
	}
	else {
		std::cout << "msgId为-1，已拒绝发送该包" << std::endl;
	}
}

//由于Udp底层更改了地址的逻辑，导致sendMsg不得不有一个地址参数，所以不得不给TCP弄一个函数，将原来的打包发送函数改为打包函数
//公共发送函数
template<class Response_T>
void BaseServer::tcpSendMsg(NetConnection* conn, Response_T* responseData, int responseID)
{
	if (packageMsg<NetConnection, Response_T>(conn, responseData, responseID)) {
		Debug("tcpSendMsg已经打包数据完成，准备执行发送函数");
		conn->sendMsg();
	}
	else {
		std::cout << "msgId为-1，已拒绝发送该包" << std::endl;
	}
}

//辅助addHandleFunc的
template<class MsgType>
bool BaseServer::recordMsgType(int index, std::initializer_list<int> msgIdList) {
	msgIdRecoder->setIdentifier<MsgType>(*(msgIdList.begin() + index));
}

template<class Server_T, class Conn_T, class MsgType>
bool BaseServer::recordMsgType(Server_T* server, void(BaseServer::* func)(Conn_T*, void*), int index, std::initializer_list<int> msgIdList) {
	recordMsgType<MsgType>(index, msgIdList);
	server->addMsgRouter(msgIdRecoder->getMsgId<MsgType>(),
		std::bind(func, this, std::placeholders::_1, std::placeholders::_2));
}

//构造函数中，那两个注册和记录的密密麻麻的太费事了，这里直接封装一个类模板吧
//C++11支持默认类型参数，这个很重要
//不管是Tcp还是udp注册类型都是void(*func)(NetConnection*, void*)
//关于下面的实现改进见主要bug12
template<class Server_T, class Conn_T, class... Args>
bool BaseServer::addHandleFunc(Server_T* server, void(BaseServer::* func)(Conn_T*, void*), std::initializer_list<int> msgIdList) {
	int index = 0;
	int array[] = {(index==0 ? recordMsgType<Server_T,Conn_T, Args>(server,func, index++, msgIdList): recordMsgType<Args>(index++, msgIdList),0)...};
	return true;
}
