#include "baseService.pb.h"
#include "lbService.pb.h"
#include "Udp.h"
#include "TcpClient.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>


std::condition_variable loginSucess;
std::mutex m_mutex;
int LoginOrRegister = 0;
int uid = 0;

void registerUser(NetConnection* conn);

/*
	测试项目：
	1、多个相同的uid登录失败 √
	2、一个客户端使用未注册的uid登录tcp服务器，观察BaseServer执行handleLoginStep1是否符合预期（给mysql服务器正常发请求包，并且得到回复包） √
	3、一个客户端使用未注册的uid登录tcp服务器，观察BaseServer执行handleLoginStep2是否符合预期（lb服务器同步，符合预期；
	强制客户端下线符合预期，base服务器同步情况符合预期，重复登录同一个id，3个服务器依然可以正常工作）√
	注意，有一个小问题，客户端收不到base服务器踢下线之前的最后一个包，可能tcp底层需要有一个强制发送按钮，否则客户端收不到
	或者收不到也没关系，底层在客户端自己清理相关节点的时候不要exit就行(看日志好像收到了，但是执行exit的时候就退出了)
	4、一个客户端使用已经注册的uid登录tcp服务器，密码错误，观察BaseServer执行handleLoginStep2是否符合预期（效果和3一样）√
	5、一个客户端使用已经注册的uid登录tcp服务器，密码正确，观察BaseServer执行handleLoginStep2是否符合预期（mysql状态修改成功，lb服务器同步成功，base服务器可以收到
	请求的组，喜欢，朋友列表）不过client收不到基础服务器的登录成功通知，没有收到消息。。。。
	tmd，发现baseServer把消息发给mysql的conn了，已在bug记录中记下了。。。。然后改完之后，3正常，5正常了，但是会增加baseServer崩溃的风险，已记录TODO
	6、3个用户同时登录成功，然后第四个用户登录同一个id，第五个用户登录未注册的uid，观察BaseServer执行handleLoginStep2是否符合预期
	（3个用户同时登录成功，2个在一个服务器上，另一个在另一个服务器上，可以正常实现；一个用户登录成功，另一个用户（未注册）登录失败，可以正常实现）
	(第4个用户登录时无法获取id，第5个用户登录失败，被踢下线符合预期)
	7、三个用户登陆成功，获取到正确的组列表，喜欢列表，朋友列表，观察BaseServer执行handleLoginStep2是否符合预期（符合预期客户端可以正常接收）
	//注意到这里之前的一个客户端登录失败，导致基础服务器断言失败，退出还没解决
	//这个出现了极小概率的bug，到后面有时间再解决吧，已记录在bug21了
	8、一个用户给另一个用户（朋友）发送消息（考虑在同一个服务器和不同的服务器），可以成功接收，观察BaseServer执行handleLoginStep2是否符合预期（符合预期，都能正确接收消息）
	//注意，如果不是朋友，但是知道对方的uid，也是可以发的，只要对方在线（不管是否在同一个服务器上，都测了）,如果不在线会发送失败就丢弃了（但是客户端做了限制，只能给朋友发消息）
	9、一个用户给一个群发消息，看群组在线人员是否收到消息（可以正常通信）
	10、注册一个账号，看数据库是否正常，使用注册的账号进行私聊看是否正常（注册失败（mysql语句有问题）时各方反应正常，lb服务器人数会减少，客户端会下线）
	（注册成功时反应正常，lb服务器是登录状态，数据库查询正常，base服务器也没踢下线，并且返回的uid也正常）
	11、注册群组看各方是否反应正常（注册成功，数据库能显示查询到组，并且群主也正常，客户端能及时更新群组列表，注册失败时也正常，就是mysql会返回注册失败（执行语句失败））
	12、下线操作，看各方反应是否正常（下线成功，符合预期，mysql状态修改成功，客户端下线成功，lb服务器服务器信息更新成功，基础服务器）
	13、添加朋友操作，看各方反应是否正常
*/

//公共打包函数
template<class Conn_T, class Response_T>
void packageMsg(Conn_T* conn, int responseID, Response_T* responseData)
{
	std::string responseSerial;
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
void udpSendMsg(Udp* udp, int responseID, Response_T* responseData, sockaddr_in* to)
{
	packageMsg<Udp, Response_T>(udp, responseID, responseData);
	udp->sendMsg(*to);
}

//由于Udp底层更改了地址的逻辑，导致sendMsg不得不有一个地址参数，所以不得不给TCP弄一个函数，将原来的打包发送函数改为打包函数
//公共发送函数
template<class Response_T>
void tcpSendMsg(NetConnection* conn, int responseID, Response_T* responseData)
{
	packageMsg(conn, responseID, responseData);
	//发送数据到对端
	conn->sendMsg();
}

int startMedia(Udp* udp, void* userData) {  //返回值为void会报错
	//一启动就给lb服务器发一个要基础地址的包
	lbService::GetServerRequest responseData;
	responseData.set_modid(1);  //1代表请求基础ip，2代表请求流媒体ip

	int modid = 0;
	std::cout << "注册用户（1）or 登录（2）" << std::endl;
	std::cin >> modid;
	if (modid == 1) {
		std::cout << "请随机输入一个uid" << std::endl;
	}
	else if (modid == 2) {
		std::cout << "请输入uid" << std::endl;
	}
	else {
		std::cout << "未知业务，已退出" << std::endl;
		return -1;
	}
	LoginOrRegister = modid;
	std::cin >> uid;
	responseData.set_id(uid);  //客户uid
	udpSendMsg<lbService::GetServerRequest>(udp, (int)lbService::ID_GetServerRequest, &responseData, &udp->m_recvAddr);
}


//得到基础服务器之后的响应
void HandleServerIp(Udp* host, void* userData) {
	lbService::GetServerResponse responseData;
	responseData.ParseFromArray(host->m_request->m_data, host->m_request->m_msglen);

	int responseType = responseData.modid();
	Debug("requestType:%d", responseType);
	switch (responseType) {
	case 1:  //得到基地址
		if (responseData.host_size() == 0) {
			std::cout << "获取服务器失败" << std::endl;
			return;
		}
		std::cout << "基地址:" << std::endl;
		for (int i = 0; i < responseData.host_size(); i++) {
			std::cout << "ip:" << responseData.host(i).ip()
				<< "\tport:" << responseData.host(i).port() << std::endl;
		}
		break;
	case 2:// //得到流媒体地址
		std::cout << "流媒体地址:" << std::endl;
		for (int i = 0; i < responseData.host_size(); i++) {
			std::cout << "ip:" << responseData.host(i).ip()
				<< "\tport:" << responseData.host(i).port() << std::endl;
		}
		break;
	}
#if 0
	//创建基础客户端
	auto baseClient = (std::shared_ptr<TcpClient>*)userData;
	//不能在这创建智能指针，因为函数结束就释放掉智能指针了，可以在主函数那里创建一个空的智能指针，然后udp传进来。。。。
	baseClient->reset(new TcpClient(responseData.host(0).ip().data(), responseData.host(0).port())); 
	(*baseClient)->connectServer();  //注意这个放在下面还是上面都是一样的，自己可以分析一下
	std::thread baseClientRunner(&TcpClient::run, baseClient->get());

	baseClientRunner.join();  //不行主线程会阻塞在这，而不是mediaClient.run();这里了。。。。。。还是用类吧
#endif
	auto baseClient = (TcpClient*)userData;
	//修改baseClient的ip
	baseClient->changeServer(responseData.host(0).ip().data(), responseData.host(0).port());
	//连接server
	baseClient->connectServer();  //不对，必须要先连接，再运行，否则connectServer添加完任务，baseClient还是一直阻塞在wait上

	//这里不能直接发送请求，因为connectServer执行完还没有确定连接成功
	std::this_thread::sleep_for(std::chrono::seconds(5)); //5秒之后注册或者登录

	//注册
	if (LoginOrRegister == 1) {
		registerUser(baseClient);
	}
	//发送登录请求
	else {
		std::string passwd;
		std::cout << "请输入密码：" << std::endl;
		std::cin >> passwd;
		Debug("客户端开始登录");
		baseService::LoginRequest loginRequese;
		loginRequese.set_uid(responseData.uid());
		loginRequese.set_passwd(passwd);
		tcpSendMsg<baseService::LoginRequest>(baseClient, (int)baseService::ID_LoginRequest, &loginRequese);
	}
	
}

//得到登录响应之后的动作
void handleSuccessLogin(NetConnection* conn, void* userData) {
	baseService::LoginResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "当前用户：" << responseData.uid() << std::endl;
	if (responseData.has_result()) {
		std::cout << "登录结果：" << responseData.result() << std::endl;
	}
	else if (responseData.has_favoritelist()) {
		std::cout << "喜欢电影列表：" << std::endl;
		for (int i = 0; i < responseData.favoritelist().file_size(); i++) {
			std::cout << "fid：" << responseData.favoritelist().file(i).fid() << "\tname：" << responseData.favoritelist().file(i).name() << std::endl;
		}
	}
	else if (responseData.has_friendlist()) {
		std::cout << "朋友列表：" << std::endl;
		for (int i = 0; i < responseData.friendlist().friend__size(); i++) { //注意friend是关键字，所以protobuf会在后面加_
			std::cout << "uid：" << responseData.friendlist().friend_(i).uid() << "\tname：" << responseData.friendlist().friend_(i).name() << std::endl;
		}
	}
	else if (responseData.has_grouplist()) {
		std::cout << "群组列表：" << std::endl;
		for (int i = 0; i < responseData.grouplist().group_size(); i++) {
			std::cout << "gid：" << responseData.grouplist().group(i).gid() << "\tname：" << responseData.grouplist().group(i).name() << std::endl;
		}
	}
	else {
		std::cout << "未知列表" << std::endl;
	}
	loginSucess.notify_all();
}

void Talk(NetConnection* conn) {
	int modid = 0;
	std::cout << "私聊（1）还是群聊（2）" << std::endl;
	std::cin >> modid;
	int id = 0;
	std::cout << "请输入聊天对象（uid或者gid）：" << std::endl;
	std::cin >> id;
	if (id <= 0) {
		std::cout << "id不合法，已退出" << std::endl;
		return;
	}
	else {
		std::string msg;
		std::cout << "请输入聊天信息：" << std::endl;
		std::cin >> msg;
		baseService::SendMsgRequest msgRequest;
		msgRequest.set_modid(modid);
		msgRequest.set_msg(msg);
		msgRequest.set_toid(id);
		tcpSendMsg(conn, (int)baseService::ID_SendMsgRequest, &msgRequest);
	}
}

void handleReciviedMsg(NetConnection* conn, void* userData) {
	baseService::MsgNotify responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "消息来自：" << responseData.fromid() << std::endl;
	std::cout << "消息内容：" << responseData.msg() << std::endl;
		if (responseData.modid() == 1) {
			//1对1聊天
			std::cout << "消息需要发送给：" << responseData.toid() << std::endl;
		}
		else if (responseData.modid() == 2){
			//群聊
			std::cout << "哪个群：" << responseData.toid() << std::endl;
		}
		else {
			std::cout << "未知消息类型" << std::endl;
		
		}
}

void registerUser(NetConnection* conn) {
	baseService::RegisterRequest registerData;
	registerData.set_id(uid);
	registerData.set_modid(1);
	std::string name;
	std::cout << "请输入用户名称" << std::endl;
	std::cin >> name;
	registerData.set_name(name);
	std::cout << "请输入用户密码" << std::endl;
	std::string passwd;
	std::cin >> passwd;
	registerData.set_passwd(passwd);
	registerData.set_summary("可爱的夏天");
	tcpSendMsg(conn, (int)baseService::ID_RegisterRequest, &registerData);
}

void registerGroup(NetConnection* conn) {
	baseService::RegisterRequest registerData;
	registerData.set_id(uid);
	registerData.set_modid(2);  //群聊
	std::string name;
	std::cout << "请输入用户名称" << std::endl;
	std::cin >> name;
	registerData.set_name(name);
	registerData.set_summary("可爱的夏天");
	tcpSendMsg(conn, (int)baseService::ID_RegisterRequest, &registerData);
}

void logout(NetConnection* conn) {
	baseService::LogoutRequest logoutData;
	logoutData.set_uid(uid);
	tcpSendMsg(conn, (int)baseService::ID_LogoutRequest, &logoutData);
}

void addFriend(NetConnection* conn) {
	baseService::AddRelationsRequest addFriendData;
	addFriendData.set_modid(1);  //添加用户
	std::cout << "输入要添加的用户uid：" << std::endl;
	int friendUid = 0;
	std::cin >> friendUid;
	if (friendUid <= 0) {
		std::cout << "uid输入错误，已退出" << std::endl;
		return;
	}
	addFriendData.set_id(friendUid);
	tcpSendMsg(conn, (int)baseService::ID_AddRelationsRequest, &addFriendData);
}

void addGroup(NetConnection* conn) {
	baseService::AddRelationsRequest addGroupData;
	addGroupData.set_modid(2);  //添加组
	std::cout << "输入要添加的群id：" << std::endl;
	int groupGid = 0;
	std::cin >> groupGid;
	if (groupGid <= 0) {
		std::cout << "gid输入错误，已退出" << std::endl;
		return;
	}
	addGroupData.set_id(groupGid);
	tcpSendMsg(conn, (int)baseService::ID_AddRelationsRequest, &addGroupData);
}

void addFavorite(NetConnection* conn) {
	//喜欢请求
	std::cout << "输入要喜欢的电影id：" << std::endl;
	int fid = 0;
	std::cin >> fid;
	if (fid <= 0) {
		std::cout << "fid输入错误，已退出" << std::endl;
		return;
	}
	baseService::AddFavoriteRequest favoriteData;
	favoriteData.set_fid(fid);
	tcpSendMsg(conn, (int)baseService::ID_AddFavoriteRequest, &favoriteData);
}

class Client {
public:
	Client() {
		mediaClient = new UdpClient("127.0.0.1", 10001);  //这个是从配置文件读取，不需要外界传递
		baseClient = new TcpClient();
		mediaClient->setConnStart(&startMedia);
		//mediaClient->addMsgRouter((int)lbService::ID_GetServerResponse, HandleServerIp, mediaClient);  //错误！！！！会在底层connect的时候加锁失败
		mediaClient->addMsgRouter((int)lbService::ID_GetServerResponse, HandleServerIp, baseClient); 
		baseClient->addMsgRouter((int)baseService::ID_LoginResponse, handleSuccessLogin);
		baseClient->addMsgRouter((int)baseService::ID_MsgNotify, handleReciviedMsg);
	}
	void run() {
		std::thread baseClientRunner(&TcpClient::run, baseClient);
		std::thread mediaClientRunner(&UdpClient::run, mediaClient);
		std::unique_lock<std::mutex> lock(m_mutex);
		loginSucess.wait(lock);
		int option = 0;
		while (1) {
			std::this_thread::sleep_for(std::chrono::seconds(2));
			std::cout << "请选择功能：聊天（1）,注册群（2），添加好友（3）,添加群组（4），添加喜欢（5），下线（6），挂机（7）" << std::endl;
			std::cin >> option;
			switch (option) {
				case 1:
					Talk(baseClient);
					break;
				case 2:
					registerGroup(baseClient);
					break;
				case 3:
					addFriend(baseClient);
					break;
				case 4:
					addGroup(baseClient);
					break;
				case 5:
					addFavorite(baseClient);
					break;
				case 6:
					logout(baseClient);
					break;
				default:
					std::cout << "未知功能，请重新选择" << std::endl;
			}
		}
		baseClientRunner.join();
		mediaClientRunner.join();
	}

public:
	TcpClient* baseClient;
	UdpClient* mediaClient;
	
};

int main() {
#if 0
	//不能在子函数创建智能指针，因为智能指针在子函数结束就释放了
	std::shared_ptr<TcpClient> baseClient(nullptr);
	//udp运行在一个线程,udp启动的时候会给lb服务器发一个获取基础服务器的请求，
	UdpClient mediaClient = UdpClient("127.0.0.1", 10001);
	mediaClient.setConnStart(&startmedia);
	mediaClient.addMsgRouter((int)lbService::ID_GetServerResponse, HandleServerIp, &baseClient);
	mediaClient.run();
	
	//然后收到请求后帮助tcp连接，然tcp在启动的时候执行登录请求，然后观察客户端收到什么消息
	//tcp客户端在这创建吧，如果在响应函数，需要智能指针创建，否则执行完函数就失效，这里创建的话，内部应该增加一个修改端口的函数，算了还是在外面创建吧

	//tcp运行在一个线程
#endif
	Client client = Client();
	client.run();
	return 0;
}