#pragma once
#include "mysqlService.pb.h"
#include "lbService.pb.h"
#include "mediaService.pb.h"
#include "user.pb.h"  //不包含在lbServer中，而是并行使用，可能架构不太理想，后期有机会优化一些协议（因为我如果使用mysql的协议，有的会用到user::）
#include "Udp.h"
#include "TcpClient.h"
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include <typeinfo>
#include <typeindex>
#include <algorithm> //求差集
//注意，在测试baseServer的时候（baseServer的bug16），碰到服务器排序这里还是有问题改正服务器排序问题时，发现改正的地方很多，并且下面的注释太多了，
//索性把这个文件注改成txt文件，重新再弄一个文件，这个新文件的注释该删的直接删掉了，废弃的代码也删掉了，如果想看，可以看前两个版本的txt文件
#include <utility> //对组


class ServerAddr {
public:
	ServerAddr() {
		std::cout << "无参构造ServerAddr" << std::endl;
		m_ip = "";
		m_port = 0;
		m_serverModid = 0;
	}
	ServerAddr(std::string ip, short port, int serverModid) {
		m_ip = ip;
		m_port = port;
		m_serverModid = serverModid;
	}
	ServerAddr(const ServerAddr& obj) {
		 std::cout << "正在执行ServerAddr拷贝构造函数" << std::endl;
		 m_ip = obj.m_ip;
		 m_port = obj.m_port;
		 m_serverModid = obj.m_serverModid;
	}
	bool operator==(const ServerAddr& server)const {  //必须要有const，否则会报错
		return server.m_ip == m_ip && server.m_port == m_port && server.m_serverModid == m_serverModid;  //为啥加最后一个看流媒体服务器的设计思路文档
	}
public:
	std::string m_ip;
	short m_port;
	int m_serverModid;
};

//设计思路看流媒体服务器的设计思路文档
struct ServerInfo {
	ServerAddr m_baseServer;
	ServerAddr m_mediaServer;
	int m_fid;  //正在看的电影
	sockaddr_in m_mediaClient; //客户端的udp地址
	sockaddr_in m_baseUdpConn; //baseServer的udp地址，负责和udp通信的
	ServerInfo() {};  //可能是在使用map的时候，需要这个无参构造先初始化一个空对象
	ServerInfo(ServerAddr& baseServer) {
		m_baseServer = baseServer;
		m_baseUdpConn = sockaddr_in();
		m_fid = 0;  //设置为空
		m_mediaClient = sockaddr_in();  //设置为空
		m_mediaServer = ServerAddr();  //设置为空
	}
};

struct ServerCondition; //需要前置声明一下
class ServerMap;  //友元
//给mutliset用的
//需要放在ServerCondition上面
class ServiceConditionCmp {
public:
	bool operator()(const ServerCondition* left, const ServerCondition* right) {//const ServerCondition* &会报错
		return left > right;
	}
};

struct ServerCondition {
	friend  ServerMap;  //右元类，注意，这里不能用友元函数，因为友元函数要求在他之前需要创建该类，而ServerMap又使用了ServerCondition，导致两者互相使用，所以这里就
						//直接使用友元类了
public:
	ServerCondition(ServerAddr& addr, const std::set<ServerCondition*, ServiceConditionCmp>& routerSet) :m_addr(addr){
		m_weight["m_clientNums"] = 1;
		m_clientNums = 0;
		m_initalCondition = getInitialCondition(routerSet);
		m_condition = m_initalCondition;
		refreshCondition();
	}
	ServerCondition(const ServerCondition& obj) {  //必须要加const，否则无法转换会报错
		//m_nums = obj.m_nums;
		//obj.nums = nums;不对的
		std::cout << "拷贝构造ServerCondition" << std::endl;
	}
	//注意，移动构造前后依然是两个对象，只不过如果原来的对象里有堆内存的空间，使用移动构造，可以使用指针赋值的方式转移堆内存空间，前后依然要创建两个对象
	//所以类对象中都是栈上的话，没有区别
	ServerCondition(ServerCondition&& obj) {   //如果不提供这个，那么std::move没用，依然会调用拷贝构造，如果提供了，需要提供operator=，因为如果提供移动，那么operator=会被标记为delete
		//m_nums = obj.m_nums;
		std::cout << "移动构造ServerCondition" << std::endl;
	}
	//注意这里不能显示提供赋值构造函数，这个系统默认提供，后面在保存旧的ServerCondition时候需要用到，还有一点是vector，还有自己的类还有map都是重载了=的，所以这里不用提供
	~ServerCondition() {
		std::cout << "正在析构ServerCondition" << "nums:" << m_clientNums << std::endl;
	}
	unsigned int getCondition() {
		return m_condition;
	}
	//为了能够控制是否选择该server，提供一个设置和检查满负荷的装置，满负荷的标志是高16位全为1，如果是满负荷上层不应该选择这个了（但不是不能选），如果都是满负荷，上层也可以选择
	bool isFullLoad() {
		unsigned short curCondition = static_cast<unsigned short>(m_condition >> 16);
		if (curCondition == 65535) {
			std::cout << "已满负荷" << std::endl;
			return true;
		}
		return false;
	}
private:
	void setFullLoad() {
		m_condition = 65535 << 16;
		m_condition |= m_initalCondition;
	}
	//设置比较函数（重载<）（这时候应该着重比较真正的因素）
	bool operator<(const ServerCondition& obj) { //排序和查找都要用到
		return m_condition < obj.m_condition;
	}

	//外界不能调用这个函数，这个函数的调用者只能是ServerMap中的updateServer
	//这里refresh之前判断人数是否大于某个值，如果大于更新位满负荷状态
	void refreshCondition() {
		if (m_clientNums >= 2) {
			setFullLoad();
		}
		else {
			m_condition = static_cast<unsigned int>(m_clientNums * m_weight["m_clientNums"]) << 16;
			std::cout << "暂时性condition" << m_condition << "m_clientNums * m_weight[m_clientNums]"
				<< m_clientNums * m_weight["m_clientNums"] << std::endl;
			m_condition |= m_initalCondition;
		}
	}
	//获取初始评价
	int getInitialCondition(const std::set<ServerCondition*, ServiceConditionCmp>& routerSet) {
		//产生第7为到第14位的随机小数，刚好支持百万级别的（后面8位非常不容易重复）
		//第15位一直为0，因为为了比较，保险一点，保证有终止点
		//上面不再采用，采用5位表示10^5保证最大为10000台的服务器工作
		//应该是2^16次方
		//怎么确保初始评价唯一，只能一个一个遍历，然后取出低16位比较、
		auto it = routerSet.begin();
		int tempCondition = 0;
		do {
			std::random_device rd; // Non-determinstic seed source
			std::default_random_engine rng{ rd() }; // Create random number generator
			//std::uniform_real_distribution<int> u(0,65535);
			std::uniform_int_distribution<int> u(0, 65535);
			tempCondition = u(rd);
			for (; it != routerSet.end(); it++) {  //这个不能用范围for
				if ((*it)->m_initalCondition == tempCondition) {  //(*it)->m_initalCondition 这个地方一定要注意
					it = routerSet.begin();
					break;
				}
			}
		} while (it != routerSet.end());   //别忘记分号
		Debug("产生的随机初始condition：%d", tempCondition);
		return tempCondition;
	}
public:
	//由于之前的测试，这里都采用unsigned int，而不再采用之前的double，可以看之前的测试
	//unsigned int一共有10位，高5位给m_condition，低5位给m_initalCondition，然后两者相加得到排序评价，好像有可能相等
	//按位？反正按位也是平分的，然后产生完移动一半即可
	ServerAddr m_addr;   //注意这里的初始化方式和lbagent中serverMap的初始化方式一样的
	unsigned short m_initalCondition; //两个字节 （0-65535）
	double realCondition;  //这个是真实地评价
	int m_clientNums;  //当前服务客户端的个数
	std::vector<int> m_playingFid; //目前只是针对流媒体正在播放的，这个设计在这对于基础服务器似乎有点不合理 ，为什么是vector，因为服务器播放在一段时间内是稳定的，而查找是频繁的
	std::unordered_map<std::string, float> m_weight;  //应该从配置文件中读取
private:
	//不能让外界直接得到
	unsigned int m_condition;  //（0-128）？（排序评价）（高16位怎么安排都行，低16位给初始评价）
};

//给ServerMap中的unordered_map用的
class GetHashCode {
public:
	std::size_t operator()(const ServerAddr& server) const { //必须要有const，否则会报错
		//生成ip的hash值
		std::size_t h1 = std::hash<std::string>()(server.m_ip);
		//生成port的hash值
		std::size_t h2 = std::hash<short>()(server.m_port);
		//生成token的hash值
		//std::size_t h3 = std::hash<int>()(server.m_token);//不能使用随机数，否则你根本找不到你的数据
		//使用异或运算降低碰撞(相同为为0，相异为1)
		return h1 ^ h2;
	}
};

//给cpp文件转发群组时的unordered_map 用的
class GetSockAddrHashCode {
public:
	std::size_t operator()(const sockaddr_in& server) const { //必须要有const，否则会报错
		//生成port的hash值
		std::size_t h1 = std::hash<unsigned short>()(server.sin_port);
		//生成地址族的hash值
		std::size_t h2 = std::hash<unsigned short>()(server.sin_family);
		//生成ip的hash值
		std::size_t h3 = std::hash<unsigned int>()(server.sin_addr.s_addr);
		return h1 ^ h2^ h3;
	}
};

//给cpp文件转发群组时的unordered_map 用的，相等比较
class SockAddrIsEqual {
public:
	bool operator()(const sockaddr_in& left, const sockaddr_in& right) const { 
		return left.sin_port == right.sin_port && left.sin_addr.s_addr == right.sin_addr.s_addr;
	}
};

class ServerMap {
public:
	using serverSet = std::set<ServerCondition*, ServiceConditionCmp>;
	ServerMap(std::initializer_list<int> modidSet) {    //不能使用vector，必须要使用initializer_list
		std::cout << "有参构造" << std::endl;
		for (auto modid: modidSet) {
			m_serviceConditionMap[modid];
		}
	}
	ServerMap(ServerMap& obj) {
		std::cout << "拷贝构造" << std::endl;
	}
	//新添加的server，这个是添加到temp中的
	bool insertServer(ServerAddr& serverAddr) {
		std::cout << "serverAddr:" << serverAddr.m_ip << '\t' << serverAddr.m_port << '\t' << "所属模块" << serverAddr.m_serverModid << std::endl;
		auto& targetSet = m_serviceConditionMapTemp[serverAddr.m_serverModid];  //如果不存在会自己创建，所以不用判断    这里必须要是引用，否则是副本，插入不成功
		auto serverPair = m_serverMap.find(serverAddr);
		if (serverPair != m_serverMap.end()) {
			std::cout << "相同server，不创建对象插入" << std::endl;
			targetSet.insert(serverPair->second);
			return true;
		}
		auto serverCondition = new ServerCondition(serverAddr, targetSet);
		targetSet.insert(serverCondition);
		m_serverMap.emplace(serverAddr, serverCondition);  //insert会出错   //注意如果是已有server，那么会先插入到在了temp中（不创建对象），如果是未知的server，
		//（只可能是新增的，删除掉的根本不可能在这呀。。）那么，就开辟一个空间，然后插入到temp中，并且更新m_serverMap的索引，这个时候还不会分配这个ip，因为分配ip
		//都是从m_serviceConditionMap这个地方出去的，所以可以这样进行。当外界构建好temp之后，提交submit，就可以处理减少的了，如果外界不执行submit就会出现根本更新不了
		std::cout << "插入新server成功" << std::endl;
		for (auto& server : targetSet) {
			std::cout << "ip:" << server->m_addr.m_ip << "\tport:" << server->m_addr.m_port
				<< "\t在线人数:" << server->m_clientNums << "当前condition" << server->getCondition() << "\t所属模块:" << server->m_addr.m_serverModid << std::endl;
		}
		return true;
	}
	//提供一个submit函数，用于同步两个map,提交这个之前默认m_serviceConditionMapTemp这个已经构建好了（插入完数据之后都需要执行这个函数）
	bool submit(int serverModid) {
		assert(m_serviceConditionMap.find(serverModid) != m_serviceConditionMap.end());
		assert(!m_serviceConditionMapTemp.empty());
		//下面都需要引用，否则操作的都是副本，后面交换就交换的是副本，就会更新不成功
		auto& orignSet = m_serviceConditionMap[serverModid];  //比如这里有1，2，3，4
		auto& updatedSet = m_serviceConditionMapTemp[serverModid]; //这里有3，4，6，7，那么这个函数的任务就是释放掉1，2的内存，清除掉m_serverMap中的1，2对应关系
		//然后交换两个set（注意为什么需要这样处理，在添加完server之后，updatedSet指向了所有的服务器信息，这已经是构建好并且排好序的set，
		//orignSet其实不关心了，后面直接clear，而m_serverMap保存了原来的但是现在依然存在的server信息，以及新增的server地址，
		//还有原来有的但是现在不存在的server地址，这个时候只需要处理第三项即可，因为m_serverMap不关心你在哪个set，它只需要知道对应的地址即可）
		
		//但是这好像有隐患吧，如果释放掉原先的，那么user_Server等还保存着节点地址呀。。。没有隐患，外界保存的都是ServerAddr，要想通过ServerAddr获取ServerCondition，会有一个函数
		//所以如果getServer(ServerAddr)之后，为空，那就代表这个服务器已经不能用了，获取指定服务器的场景目前只有stopService那里需要用到，所以外界如果获取为空，直接不处理就行
		//只是当前这个服务器自身还在服务着当前对象，等所有客户都退出之后，就正式退出了
		//找出差集也就是1，2（注意是谁对谁的差集）
		serverSet diffSet;  //如果一点并集都没用，那么说明全部更新了，之前的server全都不要了 
		std::set_difference(orignSet.begin(), orignSet.end(), updatedSet.begin(), updatedSet.end(), 
			std::inserter(diffSet, diffSet.begin()));  //注意这里需要使用空间适配器，可以在b站搜索inserter
		//找到之后，析构掉即可
		for (auto& serverPtr : diffSet) {
			ServerCondition* server = serverPtr;
			//找到m_serverMap对应的key，并清空
			ServerAddr serverAddr(server->m_addr);
			m_serverMap.erase(serverAddr);
			//删除掉房间内的服务器索引,这个查看对应的设计文档，下面的注释也有，必须要删除，否则就会段错误，因为我已经把实体空间释放了，你不能再使用了
			if (server->m_addr.m_serverModid == 2) {
				for (auto& i : server->m_playingFid) {
					assert(m_mediaRoomMap.find(i) != m_mediaRoomMap.end() && m_mediaRoomMap[i].find(server) != m_mediaRoomMap[i].end());
					auto clientSetPtr = m_mediaRoomMap[i][server];
					delete clientSetPtr; //因为m_mediaRoomMap中每个服务器都对应一个客户端集合，这个是new出来的，并且这个是不能通过stop那个为0的检测出来，所以删除的这个需要在这进行
					m_mediaRoomMap[i].erase(server);  //删除当前房间的这台服务器
				}
			}
			delete server;
		}
		//交换两个set
		orignSet.swap(updatedSet);
		//清空temp的
		updatedSet.clear();
	}

	//更新服务器排序状态，外界已经修改了对应的ServerCondition的内容了，这里就是更新一下,注意传进来的ServerCondition是之前创建的对象，这里仅仅是修改
	//注意这里必须先删除掉原来的，再插入，也就是修改之前先删除，否则的原来的地址就找不到了
	//上面的说法是错误的，可以不删除，还没执行refresh
	//这个是针对m_serviceConditionMap
	//注意这个更新不只是更新m_serviceConditionMap，因为m_mediaRoomMap这里也有他的一个相当于引用，可以视为是一体的，所以这里也应该负责更新m_mediaRoomMap的排序情况
	bool updateServer(ServerCondition* serverCondition) {
		Debug("当前更新的set的serverModid：%d", serverCondition->m_addr.m_serverModid);
		int serverModid = serverCondition->m_addr.m_serverModid;
		assert(m_serviceConditionMap.find(serverModid) != m_serviceConditionMap.end());
		auto targetSet = m_serviceConditionMap[serverModid];
		Debug("当前要找的server对应的condition值：%d", serverCondition->getCondition());
		auto targetServerPtr = targetSet.find(serverCondition);
		if (targetServerPtr != targetSet.end()) {
			//有这个元素，先插入，再删除掉原来的
			//找到routerSet中该元素的原始地址
			ServerCondition* server = *targetServerPtr;  //注意这里的右边，没有*会出错
			//先保存一份旧的值
			//ServerCondition oldServer = *server;  //这里用到了赋值运算符,这是栈上的，不需要手动释放
			//可以更新了，必须要在erase后面更新。  不过如果保存了旧值，那就无所谓了，想在哪更新，就在哪更新。。不对，保存了旧值，
			//如果refreshCondition了之后也查不到了。。。。。。因为m_mediaRoomMap的那个关键值也被更新了

			Debug("即将执行updateServer中refreshCondition");
			//取出原始地址，并删除
			targetSet.erase(server);

			//如果当前服务器的服务的电影不为0，那么更新各个房间的排序情况，上层取得ServerCondition之后，需要更新一下，但是上层有哪些操作？基础服务很简单，就是增加删除人数（后期可能还包含其他负载），
			//但是，这个最终都是对m_serviceConditionMap排一下序即可，流媒体服务器则不同，流媒体房间不光光是更新某个服务器（某个房间的人员id）的服务人数（可能还有其他负载情况，这种情况和上面的是一样的）
			//，还需要添加服务器到某个房间，或者是从某个房间删除掉（这其实是需要外界进行的吧，也就是外界获取server之后，外界需要做的动作是把得到的server加入某个房间，然后给当前服务器的fid添加一下即可），
			// （或者是获取server之后，删除掉当前用户，）
			// 基本上就这些，那这个函数应该干啥，他应该只负责更新两个map的排序情况
			//不对，思考了一下，后面还需要提供一个供lb服务器的getServer重载，这个是专门针对m_mediaRoomMap提供的，而之前的那两个是都能使用的，因为外界肯定需要获取流媒体服务器，所以必须要再提供一个getServer
			//到时候那个getServer会分3种情况往外界返回服务器地址，如果有当前房间，且负载良好，然后执行update即可，如果没有当前房间，那么需要这个专门的getServer需要调用getServer,去获取总的服务器里面的地址
			//这个时候获取完之后，需要new一个std::unordered_set<int>*，然后往里添加一些元素，并且更新获取到的server负载，然后update，如果是删除，那么需要获取指定的server,然后查找到对应的房间，
			//删除掉对应的uid，然后update，（这个时候包含排序和清理掉可能存在的多余的fid，不太对，如果这个uid是当前房间的最后一个元素，那么如果交给update之后，由于ServerCondition已经清理掉了这个为0的房间的标记
			//所以update时根本感知不到这个空房间，所以需要在外界进行更改），最终可以采取这样的策略：get之后，如果是删除，那么update可以感知到，上层不要清理掉空房间，仅仅是清理掉人数，然后交给update去处理
			//如果是添加，那么外界必须要new一个new一个std::unordered_set<int>*，然后放到对应的房间里，交给update去更新，如果是后期反馈包更新，那就很好办了，直接取出对应的server，然后更新即可。也就是外界
			//需要提供房间更新的所有条目，但是房间的释放与否，服务器即使在当前房间空转，也不需要外界操心，由updateServer统一管理
			//（根据上面的分析）必须要在refresh之前删除掉旧的，不要企图保存旧的值，然后依靠旧的值查找，这样不可能找到原来的值了
			if (server->m_addr.m_serverModid == 2) {
				if (!server->m_playingFid.empty()) { //当前server，正在播放一些fid
					//std::vector<std::iterator<std::map<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp>>> serversIt; //编译错误
					std::vector<std::map<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp>::iterator> serversIt;
					//不光要记录当前server在各个房间中的it，还要记录这个it所在的房间，因为后面如果只保留it，根本不知道这个it在哪个房间内
					//std::vector<int> roomId; 可以不用记录，因为和m_playingFid是一一对应的，直接访问m_playingFid即可
					for (auto& i : server->m_playingFid) {  //遍历这些房间，先记录这些房间中的server对应的指针，因为如果不记录，后面没法记录了，因为我必须在后面的循环中删除旧的和添加新的，所以必须要先记录旧指针
						assert(m_mediaRoomMap.find(i) != m_mediaRoomMap.end() && m_mediaRoomMap[i].find(server) != m_mediaRoomMap[i].end());
						serversIt.push_back(m_mediaRoomMap[i].find(server));  //暂时保存这个对象的指针，注意是迭代器，不能使用[]
					}
					//上面已经记录了每个房间的当前server的指针，所以可以放心的更新了
					server->refreshCondition();
					//这个for里有删除的，会不会导致serverIt迭代器失效？不会的，cppreference里面说map的迭代器增删之后不会失效
					auto serverIt = serversIt.begin();  //必须定义在外面，只有相同的元素才可以使用逗号表达式定义多个
					auto playingIt = server->m_playingFid.begin();  //标记m_playingFid中当前roomIt的下标位置，注意这里必须使用 server->m_playingFid，因为我要删除的元素对象就是他
					for (; serverIt != serversIt.end(); serverIt++) { //serverIt是一个pair，即<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp>>这种类型
						//记录当前服务器服务的客户端集合地址，注意serverIt是vector的迭代器，(*serverIt)才是元素本身，但是这个元素是迭代器，所以还要(*(*serverIt))才是pair，已记录在流媒体相关的bug中
						std::unordered_set<int>* clientSetPtr = (*(*serverIt)).second; 
						ServerCondition* curServer = (*(*serverIt)).first;
						int fid = *playingIt;
						m_mediaRoomMap[fid].erase(curServer);  //删除当前房间的这台服务器  注意erase可以使用key也可以使用迭代器
						
						if (clientSetPtr->size() != 0) {  //如果更新之后，当前房间不为0，那就直接插入新值即可
							m_mediaRoomMap[fid].emplace(curServer, clientSetPtr);  //插入更新后的值，这样就完成了当前服务器在所有房间的负载排序
							playingIt++;  //注意这两个it的加加的方式，如果删除就是下面的方式如果不删除，就加加
						}
						else { //当前房间的这台server正在空转，肯定是减少，已经减少到没有了，这个时候需要这里进行房间的清理了（不过还可以采取其他策略）
							std::cout << "当前服务器正在空转，目前的策略是从该房间清除掉空转的服务器" << std::endl;
							playingIt = curServer->m_playingFid.erase(playingIt);		//从当前服务器删除正在播放的标记fid
							std::cout << "1" << std::endl;
							//删除了uid之后如果服务客户端变为了0，那么就从fid这个房间删除这个服务器了，这个时候不用管理m_serviceConditionMap和m_serverMap，前一个只负责负载，后一个只负责在线服务器的索引
							delete clientSetPtr;  //如果当前服务器服务人数已经为0，那么下一步是从当前房间上删除这个为0的服务器，所以删除之前需要释放set集合
							//上面已经删除过这个条目了，所以这里不要再删除了m_mediaRoomMap[fid].erase(serverIt);
							//删除这个已经为0的服务器，上面已经删除了，这里不用管了（m_mediaRoomMap[i].erase(serverIt);）
							//如果删除了某个服务器之后，当前房间的服务器数量为0，那么说明当前房间没人看了，那就清除当前房间
							std::cout << "2" << std::endl;
							if (m_mediaRoomMap[fid].size() == 0) {
								std::cout << "3" << std::endl;
								m_mediaRoomMap.erase(fid);
							}
							std::cout << "4" << std::endl;
						}
					}
					//删除完之后我要插入新的值呀，这个不要忘记了，已经在上面更新了m_mediaRoomMap[fid].emplace(server, clientSetPtr);
				}
				//如果当前流媒体server的play列表里是空的，那说明没有在任何一个房间，那么就没必要处理
			}
			//插入更新后的值
			targetSet.insert(server);
			std::cout << "5" << std::endl;
			return true;
		}
		std::cout << "没有目标server，无法更新" << std::endl;
		return false;
	}

	//外界不能直接获取ServerCondition，都要从这个方法获取，如果直接获取会出错
	//如果没有指定，那么就返回最好的，如果指定了那就返回指定的server，如果指定的没有，返回空
	//注意，再使用getServer之后一定要使用updateServer更新一下，否则会直接丢失该server，后期可以改进一下，设计一个更改函数，那么这里就直接调用了
	//不对，可以先更新再删除，因为更新之后我肯定有地址了，这个时候传给上面的update，然后ServerCondition里有ServerAddr，这个时候直接再查一下不就得到了
	//所以，把下面两行注释起来，但是依然改变值之后依然要调用updateServer，否则会有多余的键，而且外界还没调用refersh函数，说明最终的condition还没改，可以直接找到的
	//所以先删除更没必要了
	//后面写其他模块时想了一下还是觉得一个函数只改执行一个功能
	ServerCondition* getServer(ServerAddr* server) {   //外界不需要提供serverModid了，只需要提供server
		int serverModid = server->m_serverModid;
		assert(m_serviceConditionMap.find(serverModid) != m_serviceConditionMap.end());
		auto targetSet = m_serviceConditionMap[serverModid];
		ServerCondition* targetServer = nullptr;
		//返回指定的server
		auto targetPair = m_serverMap.find(*server);
		if (targetPair != m_serverMap.end()) {
			targetServer = targetPair->second;  //这里要注意右边是.还是->
			//targetSet.erase(targetServer);
			return targetServer;
		}
		//不是nullptr,但是也没找到，返回空
		return nullptr;
	}

	//拆开吧，一个函数不要有那么多的功能，利用重载更好
	//这个是获取当前模块最好的服务器
	ServerCondition* getServer(int serverModid) {
		assert(m_serviceConditionMap.find(serverModid) != m_serviceConditionMap.end());
		auto targetSet = m_serviceConditionMap[serverModid];
		ServerCondition* targetServer = nullptr;
		//targetServer = *targetSet.begin();  //注意等号右边
		//targetSet.erase(targetSet.begin());
		//这里需要 判断是否处于满负荷状态，如果处于满负荷状态，则找下一个
		for (auto& curServer : targetSet) {
			if (!curServer->isFullLoad()) {
				return curServer;
			}
		}
		//如果都是满负荷的，那就选择第一个
		targetServer = *targetSet.begin();
		//然后把当前这个系数改为-1
		return targetServer;
	}

	//这里还需要对外界提供一个getServer的重载版本，目的是实现能够返回某个房间的一个可用的服务器，上面两个getServer显然不行
	//获取server这里加个const，使其增加一个函数签名，因为这个get完之后，外界不需要做修改的，加了个uid，就不需要const了
	ServerCondition* getServer(int uid, int fid, bool& isNewServer)  { //最后一个参数是为了给外层传递是否是新加入的服务器，帮助外层进行判断是否需要发送消息
		//先查看当前是否有fid对应的房间，如果没有那就创建一个
		ServerCondition* targetServer = nullptr;  //目标服务器
		std::map<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp>* roomServers = getMediaServerMap(fid);
		//如果有当前房间，即roomServers[fid]
		if (roomServers != nullptr) {  //说明当前房间存在，尝试从当前房间获取，（注意不要上来就判断为nullptr，这样会导致后面的逻辑出现重复getServer(2)的情况）
			//获取当前房间最好的服务器返回给客户端
			ServerCondition* targetServerCondition = nullptr;
			for (auto it = roomServers->begin(); it != roomServers->end(); it++) {  //一定要注意这是roomServers，见流媒体服务器的主要bug4
				targetServerCondition = it->first;
				Debug("targetServerCondition所指向的地址：%x", targetServerCondition);
				if (!targetServerCondition->isFullLoad()) {
#if 0
					targetServer = targetServerCondition->m_addr;  //只要有一个不是满负载，那就得到有效的服务器了
					//更新负载情况（需要更新两个地方的负载排序，以及当前服务客户端集合，还有user_ServerMap的value值，上面的if也是）
					targetServerCondition->m_clientNums += 1;
					roomServers[fid][targetServerCondition].insert(uid);   //注意这里插入的是旧的集合中，到时候更新集合即可

					//roomServers[fid].insert(targetServerCondition); //2❌  //不能简单的这样插入，其实value是一个对组，这样插入必将失败，一定要连着将map和对应的set一起加入删除
					std::unordered_set<int> clientSet;
					clientSet.swap(roomServers[fid][targetServerCondition]);  //2
					//在交换之前，不能删除原来的targetServerCondition，因为找不到对应的set，除非提前保存
					roomServers[fid].erase(targetServerCondition);  //roomServers[fid]为std::map<ServerCondition*, std::unordered_set<int>*>
					//roomServers[fid]在删除之前不能更新updateServer，因为如果更新，那就找不到对应的it，除非提前保存下来
					serverMap.updateServer(targetServerCondition);   //1
					//更新完总的服务器负载之后之后才能插入当前房间的负载排序，因为不更新，插入的还是旧的
					roomServers[fid][targetServerCondition] = clientSet;  //但是这个会进行值拷贝吧，效率会比较低。。。。这个要改进
#endif
					targetServer = targetServerCondition;  //只要有一个不是满负载，那就得到有效的服务器了
#if 0
					//更新负载情况（需要更新两个地方的负载排序，以及当前服务客户端集合，还有user_ServerMap的value值，上面的if也是）
					targetServerCondition->m_clientNums += 1;
					roomServers[fid][targetServerCondition].insert(uid);   //注意，这个std::unordered_map是伴随着这个房间的服务器一直存在的，只有不分配当前服务器了，那么这个set就可以释放了
					//记录即将要删除的targetServerCondition对应的set，这个后面要给新的值
					std::unordered_set<int>* clientSet = roomServers[fid][targetServerCondition];
					//在交换之前，不能删除原来的targetServerCondition，因为找不到对应的set，除非提前保存
					roomServers[fid].erase(targetServerCondition);  //roomServers[fid]为std::map<ServerCondition*, std::unordered_set<int>*>
					//roomServers[fid]在删除之前不能更新updateServer，因为如果更新，那就找不到对应的it，除非提前保存下来
					serverMap.updateServer(targetServerCondition);   //1
					//更新完总的服务器负载之后之后才能插入当前房间的负载排序，因为不更新，插入的还是旧的
					roomServers[fid][targetServerCondition] = clientSet;  //但是这个会进行值拷贝吧，效率会比较低。。。。这个要改进  //2 3
#endif
					isNewServer = false;
					break;
				}
			}
		}
		ServerCondition* targetServerCondition = nullptr;
		//如果到这，要么是当前房间都满负载了，那么申请新的服务器为当前fid提供服务，要么是根本没有当前房间，但是都有先申请一个地址
		if (targetServer == nullptr) { //如果为nullptr，说明还没有得到有效的服务器，这个时候说明当前房间都满负载了，或者是
			//如果申请到了，那就正常更新即可
			targetServerCondition = getServer(2);
			if (targetServerCondition != nullptr) {
				//在当前服务器上添加播放列表
				targetServerCondition->m_playingFid.push_back(fid);  //如果是第一次创建必须要添加
				//创建一个客户端集合
				std::unordered_set<int>* clientSet = new std::unordered_set<int>;  //注意这是new的
				//clientSet->insert(uid);  //更新当前服务器正在服务的对象  1

				//更新总的负载情况
				//targetServerCondition->m_clientNums += 1;
				//serverMap.updateServer(targetServerCondition);   //3
				if (m_mediaRoomMap.find(fid) == m_mediaRoomMap.end()) {  //如果是这种情况那就不是满负载，而是当前根本没这个房间，其实这个判断根本不需要，因为[]可以直接创建一个对象
					std::map<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp> newRoomServers;
					m_mediaRoomMap[fid] = newRoomServers;
				}
				m_mediaRoomMap[fid][targetServerCondition] = clientSet;   //2 ，注意一定要targetServerCondition更新之后，这里插入的才是有一个客户的值，否则当前客户端负载其实是0的
				targetServer = targetServerCondition;  //要返回的地址 
				isNewServer = true;
			}
			else {
				//如果申请不到了，那就返回空
				//目前还没有申请不到的状况，即使超载也可以返回一个ip，所以这个先不写了
			}
		}
		return targetServer;  //注意这里的targetServer目前不可能为nullptr，但是后面如果getServer有可能为空，这里也可能返回空了
	}

	void printServerMapInfo() {
		//打印现在对象的内存分配情况
		std::cout << "当前服务器模块个数（m_serviceConditionMap的内存分配情况）：" << m_serviceConditionMap.size() << std::endl;
		int i = 1;
		for (auto& serverPair : m_serviceConditionMap) {
			std::cout << "\t当前模块" << i++ << "服务器个数：" << serverPair.second.size() << std::endl;
			for (auto& server : serverPair.second) {
				std::cout << "\t\tip:" << server->m_addr.m_ip << "\tport:" << server->m_addr.m_port 
					<< "\t在线人数:" << server->m_clientNums << "\t当前condition" << server->getCondition() <<  "\t所属模块:" << server->m_addr.m_serverModid << std::endl;
			}
		}
		std::cout << "当前服务器索引情况个数（m_serverMap的内存分配情况）：" << m_serverMap.size() << std::endl;
		i = 1;
		for (auto& serverPair : m_serverMap) {
			std::cout << "\t当前服务器索引" << i++ << "地址详情" << "[" << serverPair.first.m_ip << ":" << serverPair.first.m_port << ":" << serverPair.first.m_serverModid << "]" << std::endl;
		}
		std::cout << "当前服务器模块临时存储区个数（m_serviceConditionMapTemp的内存分配情况）：" << m_serviceConditionMapTemp.size() << std::endl;
		for (auto& serverPair : m_serviceConditionMapTemp) {
			std::cout << "\t当前模块" << i++ << "服务器个数：" << serverPair.second.size() << std::endl;
		}
		std::cout << "当前房间个数（m_mediaRoomMap的内存分配情况）：" << m_mediaRoomMap.size() << std::endl;
		for (auto& roomPair : m_mediaRoomMap) {
			std::cout << "\t当前房间" << roomPair.first << "服务器个数：" << roomPair.second.size() << std::endl;
			for (auto& serverPair : roomPair.second) {
				std::cout << "\t\t[" << serverPair.first->m_addr.m_ip << ":" << serverPair.first->m_addr.m_port << "]" 
					<< "\t在线人数:" << serverPair.first->m_clientNums << "\tcondition" << serverPair.first->getCondition() << "\t模块:" << serverPair.first->m_addr.m_serverModid;
				std::cout << "\t当前房间服务器服务的uid：";
				for (auto& client : *(serverPair.second)) {  //注意这里直接得到的是元素值，而不是迭代器了
					std::cout << client << " ";
				}
				std::cout << "\t当前服务器在哪些房间：";
					for (auto& fid : serverPair.first->m_playingFid) {  //注意这里直接得到的是元素值，而不是迭代器了
						std::cout << fid << " ";
					}
				std::cout << std::endl;
			}
		}
	}
	//获取某一房间服务器集群
	std::map<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp>* getMediaServerMap(int fid) {
		auto targetMapIt = m_mediaRoomMap.find(fid);
		if (targetMapIt != m_mediaRoomMap.end()) {
			return &m_mediaRoomMap[fid];
		}
		return nullptr;
	}
	//获取当前流媒体服务器地址对应的ServerCondition
	//上面的getServer已经定义的很不错了，完全可以借用

	//获取某一个服务器服务的客户端信息
	std::unordered_set<int>* getMediaServerClient(int fid, ServerAddr& server) {
		auto targetMap = getMediaServerMap(fid);
		if (targetMap != nullptr) {
			auto targetServerCondition = getServer(&server);
			if (targetServerCondition != nullptr) {
				return m_mediaRoomMap[fid][targetServerCondition];
			}
		}
		return nullptr;
	}
	//上面那个updateServer是更新m_serviceConditionMap这个的负载情况（不管是modid几），还应该增加一个更新m_mediaRoomMap这个的负载情况
	//注意由于只能先更新updateServer，然后再插入，所以这个函数既更新了m_mediaRoomMap，也更新了m_serviceConditionMap（第二个模块），必须是先更新m_serviceConditionMap，这里再进行insert，或者是这里更新完，
	//然后updateServer那里调用这里的更新函数，然后再insert，必须这样交替，不能在外界同时调用两个。如果同时调用两个（也就是这个内部不再调用updateServer）那会导致，比如先调用updateMediaRoom，
	//那么调用updateServer的时候找不到原来的地址，不过反过来好像是可以的。反过来不行，因为如果updateServer完，那么mediaServerCondition是更新后的值，roomServers.erase(mediaServerCondition);就会找不到
	// 除非保存一份栈区的旧值，然后传进来，注意，房间的map，虽然也有ServerCondition，但是这个不属于他的，而是属于m_serviceConditionMap
	//最终我还是采取外界保留旧值，然后传进来进行更新，但是这个好像对于上层调用者很容易掉坑里，那就只为外界提供一个函数接口，这些操作都是私有成员函数在进行，上层只需要updateRoom即可
	//这个是针对m_mediaRoomMap这个数据结构的进行删除更新的，不过一定要注意传递进来的是旧值，不能是新值
	//bool updateMediaRoom(int uid, int fid, ServerCondition* mediaServerCondition); 
	//这个函数的已经被我融入到updateServer那里了

	//这个是针对m_mediaRoomMap这个数据结构进行添加更新的
	//bool updateMediaRoom(int uid, int fid, ServerCondition* mediaServerCondition) ;这个打算在getServer的重载函数里实现或者lb服务器返回流媒体服务器那里实现
	//上面有一个getServer，只能获取m_serviceConditionMap，这里应该提供一个针对m_mediaRoomMap获取的操作
	

	//昨天在写更新的时候，没有意思到一个问题，就是m_mediaRoomMap中的ServerCondition*是m_serviceConditionMap中的一份副本而已，所以，如果m_serviceConditionMap更新了，那么m_mediaRoomMap必须要更新，
	//m_serviceConditionMap的值少了，那么ServerCondition必须要删除（比如定时更新的时候，如果原来的机器决定废除，那么m_mediaRoomMap中的ServerCondition项也必须要删除，
	// （因为如果定时更新的时候删除掉了，这里还保存着指针，肯定会崩溃，要么就是submit的时候更新一下m_mediaRoomMap，但是这个好像每个房间都要查找更新了。。。。）所以这个并不太可取
	//感觉需要把ServerCondition设置为不同的关键字，不能使用ServerCondition），但是又必须要给当前房间的服务器排序，这该咋办？？其实可以不排序吧，因为这个服务器数量不会太多，顶多几台而已（可以采取轮询分配）
	//但是还是需要解决同步的问题，假如m_serviceConditionMap删除了某个服务器该咋办，总不能一个房间一个房间找吧，不对，好像也不行吧，即使这边m_mediaRoomMap同步成了，但是user_ServerMap那个也记录了
	//那些被删除的服务器，如果停止服务势必需要清除掉这些东西，所以停止服务或者切换服务的时候需要判断当前服务器是否还存在，那又回到之前的问题，删除的时候怎么避免一个房间一个房间找
	//也就是每台流媒体服务器不仅仅要知道服务的对象（uid）（ServerCondition*, std::unordered_set<int>*）还应该知道正在播放哪些电影，在ServerCondition类中增加了当前服务的对象，
	//但其实并不合理，因为基础服务器没有这些的
private:
	//更新策略好像不太对，要以mysql的map为准的，所以需要一个temp,并且由于一个对象用的是同一块地址，mysql如果某一次有增加的，也有减少的，这该怎么办
	//而且减少的服务器可能还在工作，需要保证这个服务器其他关系完整，不能随便删掉
	//可以假定mysql如果没有了这台的信息，那么默认他已经处于不服务状态，就是其他关系表都为空了，比如user_ServerMap。这个需要运维保证，也就是假设这台服务器不行了
	//那么需要等服务完再从mysql删除掉
	//剩下的就是假如从mysql获取之后有新增的，也有删除的怎么办，求差集，然后释放
	std::map<int, serverSet> m_serviceConditionMap;  //包含流媒体和聊天的服务器集群，//这里的hostset代表的是所有的模块的中服务器的所有的服务器服务状态，更新下面的两个状态时这个也要更新
	std::unordered_map<ServerAddr, ServerCondition*, GetHashCode> m_serverMap;
	std::map<int, serverSet> m_serviceConditionMapTemp;
	//流媒体服务器的存储结构，至于为啥设计成这样，参考流媒体服务器对应的文件夹下的设计思路文档（包括这里面的字段代表什么意思，里面也记录了）
	std::unordered_map<int, std::map<ServerCondition*, std::unordered_set<int>*, ServiceConditionCmp>> m_mediaRoomMap;
};

class TypeIdentifier {
public:
	TypeIdentifier() {
		m_readyIdenifier = 0;
		//需要在构造函数这里设置对应的标志值，能不能通过for循环的方式简单的设置呢，或者通过参数传递进来的方式进行设置
		//或者这里不做任何处理，而是将设置标志关系的权力交给MsgCatch管理，
		//我感觉两个类的耦合关系小一点比较好，这个类完成的功能，不应该和MsgCatch有所关联，而是仅仅就做一个类型标志器，你具体怎么使用应该是MsgCatch的事情
		//所以这里不应该在这里设置标志值
		m_typeIdentifier.clear();  //甚至这一步都不需要做
	}
	//获得标志值
	template <typename type>
	int getIdentifier() {
		//注意type_index对象是基于typeid(T)返回的type_info引用，然后进行匿名构造产生的type_index对象，然后find会进行比较（type_index里面有==）
		//当然也可以显示的type_index（typeid(T)）也行
		//但是type_index是个匿名对象，这就牵扯到m_typeIdentifier[]时插入的是副本，还是这个值本身了
		//不用担心，找到了这个https://noerror.net/zh/cpp/container/unordered_map/operator_at.html，里面有T& operator[]( const Key& key );
		//和T& operator[]( Key&& key );
		auto it = m_typeIdentifier.find(typeid(type));
		if (it != m_typeIdentifier.end()) {
			return it->second;  //迭代器要使用->，这都忘记了。。。
		}
		return -1; //不存在就返回-1
	}
	//设置标志值
	//默认标志值为-1，如果是-1，那么就按照默认值增序给其赋值，如果不是-1，那么就按照传进来的值赋值
	template <typename type>
	bool setIdentifier(int identifier = -1) {
		auto it = m_typeIdentifier.find(typeid(type));
		if (it != m_typeIdentifier.end()) {
			std::cout << "已经为该类型设置标志" << std::endl;
			return false;
		}
		if (identifier == -1) {
			m_typeIdentifier[typeid(type)] = m_readyIdenifier++;  //注意typeid(T)
		}
		else {
			m_typeIdentifier[typeid(type)] = identifier;
		}
		return true;
	}
private:
	//注意，unordered_map本身是需要提供hash函数和operator==来工作的，但是std::type_index这个类本身有operator==和hash_code函数，所以可以显示指定
	//这个是参考了网上的unordered_map的使用案例（两种方式指定那两个函数）和cppreference.com里关于type_index的使用总结出来的
	//注意这里为啥使用unordered_map，因为查询时o(1)而map是o(logn)，还有就是之前已经使用过map了，这里想体验一下这个函数
	//type_index是对type_info的一层包装，而typeid返回的是type_info的对象
	std::unordered_map<std::type_index, int> m_typeIdentifier;
	int m_readyIdenifier;  //当前待分配的类型标志值
};

class MsgCatch {
public:
	MsgCatch() {
		//应该在这里设置标志值，只能选择一条一条的 有顺序的 执行setIdentifier
		//不对，可以遍历tuple，然后获取元素0的类型传递进去不就行了，太完美了
		//但是tuple不支持遍历。。。
		//for (auto msgList : m_catch) {
		//	m_typeIdentifierSet.setIdentifier<decltype(msgList[0])>();  //对std::shared_ptr<XXX>进行判断，这个智能指针本质上还是类对象
		//}
		//但是可以使用std::遍历类似tuple的对象（不一定是tuple），但是这个好像是c++17的方法
		//但是为了能够保证很好的匹配，还是决定使用std::apply，因为如果不使用，需要人为的去匹配类型标志值和tuple序列元素顺序的关系，虽然这个关系很简单
		//不过好像gcc版本需要升级到8.3.1，我记得之前配置protobuf的时候，升级到了8，但是不知道，（之前查了是8）查看还是4.8.5
		//在网上查到的是如果使用scl enable devtooset-8 bash 只是针对当前终端有效，只是临时性的，如果想要 永久升级，
		//或者系统同时支持4和8的，可以使用source /opt/rh/devtoolset-8/enable启动一下，然后如果退出当前终端，那么又变回4的了，想用8的需要重新启动
		//当然也可以设置默认版本为某一版本，我暂时没有这个需求就不设定了
#if 0
		//需要c++17支持，当前系统gcc版本默认为4.8.5，如果需要17，需要source /opt/rh/devtoolset-8/enable切换到gcc8,然后就支持17了，
		//不过还需要改makefile，这个暂时不实现
		std::apply()
#elif 1
		auto list0ElementIt = std::get<0>(m_catch).begin();
		m_typeIdentifierSet.setIdentifier<decltype(*list0ElementIt)>();  //list没有[]和at
		//目前只有一个类型需要记录，所以只有上面这个设置就可以了
		//m_typeIdentifierSet.setIdentifier<decltype(std::get<1>(m_catch)[0])>();
#else
		//因为下面的std::get静态编译问题，不得不实现一个遍历的函数，所以这里凑巧可以使用以下
#endif
	}
	//这个MsgCatch设计一开始考虑的是通过<boost::any>设计，但是后面发现这是个第三方库，然后就打算使用tuple，但是使用tuple，因为是通过get0，1，2
	//获取对应的列表，所以如果只用一个tuple，在增加删除的时候，就要开发者记住，对应的信息，这个很不优雅。然后我就打算增加一个map，将对应的类型和0，1，2进行对应，
	//一开始想的是传进去一个int MessageID（也就是proto协议里的id），这个很好设计，但是感觉传进来的还是冗余，因为我本身可以通过message就能得到其类型
	//就完全可以判断是属于tuple中哪个元素了，所以后面就设计了，将不同的类型映射到同一种变量的不同值，相当于是给不同的变量类型起了标志，
	//然后再将这个标志和对应的tuple下标关联，就可以非常优雅的进行增加删除操作了，（具体可以使用typeid）（一开始我还想着使用static_cast<size_t><T>将一类型
	//转为一个变量值，但这个显然是错误的）（当然还有其他的方法实现这个类（比如了解到元编程，但是很不熟悉），但是我还是打算采用这个方式）
	using MsgLists = std::tuple<std::list<std::shared_ptr<lbService::RepostMsgRequest>>>;
	//增加一个msg
	//必须要有int MessageID, Message_T& message，本来我想省略MessageID，但是发现如果省略掉，就需要根据类型建立一个map（比如int对应0，double对应2）
	//已经建立了一个TypeIdentifier，所以可以省掉MessageID
	//注意这里传进来的就是智能指针引用
	template <class Message_T>
	void record(Message_T& message) {
		int msgIdentifier = m_typeIdentifierSet.getIdentifier<Message_T>();
		if (msgIdentifier != -1) {
			//std::get<msgIdentifier>(m_catch).push_back(std::move(message));这样写不行，必须要是常量
			//需要将int msgIdentifier = m_typeIdentifierSet.getIdentifier<Message_T>();改成
			//constexpr int msgIdentifier = m_typeIdentifierSet.getIdentifier<Message_T>();
			//发现了一个致命的错误，就是std::get<msgIdentifier>中的参数必须是在编译期间就能确定是多少，我这显然是运行期间才会进行调用，
			//而且是多次调用这该怎么办
			//经过一晚上的查资料，发现这个是静态多态和动态多态的一个矛盾点，也就是msgIdentifier这个值是不确定的，我只有运行时才确定
			//而这里的get却必须要在编译期间就确定，那怎么办
			//跑完步，突然想到了两个好办法：遍历和分支语句，首先描述一下怎么使用，
			//分支语句很简单就是if(msgIdentifier == 0)std::get<0>(m_catch).push_back(std::move(message));...
			//循环语句用到了元编程，这里不介绍怎么遍历（下午alook中看的一篇帖子里有），而是只介绍在这里怎么用，也就是通过判断msgIdentifier和get<i>
			//中的i是否相等，来决定是否结束循环并返回对应的list
			//解释一下为啥，因为std::get<index>中的index是在编译期完成的，也就是编译期编译到get时就已经知道该返回谁了，index这个值不能已经执行到std::get<index>
			//还没确定下来，但是上面的 m_typeIdentifierSet.getIdentifier<Message_T>();显然没有确定，因为我编译阶段编译到这句时，根本不知道message_T
			//是谁，所以根本确定不下来，只有编译到cpp文件的调用的时候才能确定这里的message_T，所以这就需要一个衔接，也就是上面两个方法
			//可能遍历的还不太好理解，因为遍历我也不知道类型是啥呀，但是编译阶段，遍历过程中std::get<i>(m_catch)中i是确定的，虽然不知道index
			// 下面两个模板也是一样
			//今天了解的元编程和编译原理的一些知识，很有用处。。。可以看看对应的网页（没有删除）
			//std::get<index>(m_catch).push_back(std::move(message));换成下面的
			//替换之后完美解决
			matchList(message, msgIdentifier)->push_back(std::move(message));
		}
	}
	//删除某个list的所有条件相同的消息
	template <class Message_T>
	void remove(Message_T& message) {
		int msgIdentifier = m_typeIdentifierSet.getIdentifier<Message_T>();
		//auto targetList = std::get<msgIdentifier>(m_catch); 这种方式根据上面的分析方式不行，换成下面的
		auto targetList = matchList(message, msgIdentifier);
		if (msgIdentifier != -1) {
			//注意remove_if会将待删除的所有元素都移到最后，然后返回第一个待删除迭代器，然后erase会将起始元素到最后的end范围内的所有元素都给删除掉
			//lambada表达式这是为了判断，可以用bool普通函数或者仿函数
			targetList->erase(std::remove_if(targetList->begin(), targetList->end(),
				[&message](Message_T& requestElement) {
					return requestElement->modid() == 2 && requestElement->fromid() == message->fromid() && requestElement->toid() == message->toid();
				}),
				targetList->end());
		}
	}
	//获取某个消息列表
	//注意这里提供Message_T messsage不是为了使用他，而是为了调用的时候传递一个值进行自动类型推断
	template <class Message_T>
	std::list<Message_T>* getMsgList(Message_T message) {
		int msgIdentifier = m_typeIdentifierSet.getIdentifier<Message_T>();
		//auto targetList = std::get<0>(m_catch);和上面一样的原因
		auto targetList = matchList(message, msgIdentifier);
		return targetList;
	}

	template <class Message_T>
	std::list<Message_T>* matchList(Message_T& message, int msgIdentifier) {//message没啥用，这个主要是为了自动推导用的
		if (msgIdentifier == 0) {
			//Debug("std::get<0>(m_catch)的地址：%x,当前list大小：%d", &std::get<0>(m_catch), std::get<0>(m_catch).size());
			//std::cout << "当前list的类型：" << typeid(std::get<0>(m_catch)).name() << "当前message的类型：" << typeid(Message_T).name() << std::endl;
			return &std::get<0>(m_catch);
		}
		else {
			std::cout << "未定义消息类型" << std::endl;
		}
	}
public:
	MsgLists m_catch;
	//维护一个m_msgList下标和对应事件id的关系（第一个key是事件下标，第二个key是对应的tuple下标）
	TypeIdentifier m_typeIdentifierSet;
};

#if 0
struct mediaHosts {
	HostInfo chatInfo;
	HostInfo mediaInfo;
	bool operator==(const mediaHosts& obj) const {
		return obj.mediaInfo == mediaInfo;
	}
};

class mediaCmp {
public:
	bool operator()(const mediaHosts& left, const mediaHosts& right) {
		return left.mediaInfo.serviceCondition >= right.mediaInfo.serviceCondition;
	}
};
#endif

class LbAgent {
public:
	//using mediaRoomSet = std::set<mediaHosts, mediaCmp>;
	//using ServerCondition = std::map<HostInfo, int>;  //当前服务器集群的服务信息，如果两个业务在同一台主机，也就是key一样的话，那么插入的时候就小心一点

	LbAgent(const char* udpIp, unsigned short udpPort, const char* mysqlAgentIp, unsigned short mysqlAgentPort, std::initializer_list<int> modidSet);
	~LbAgent();
	//对外提供启动接口
	void run();
	//建立map，每当服务器启动都建立对应的map，不需要了，定时更新的函数可以直接建立map
	//void buildMap();
	//当有服务器故障时，也就是定时检测routerMap对应的成功率，如果大于某个值，后续就不再派发这个服务器给对应的房间
	void monitor();
	//定时检查数据库的值是否和routerMap的值一致，如果相等（集合有一个重载==的），什么都不操作，如果返回false，就以数据库建立的表为基准，
	//然后根据routerMap的值，修改新建立的serviceCondition的值，但是原来的map所指向的服务器信息不能删除，因为房间map还在用，那怎么办呢，用智能指针，就不用管销毁工作了
	//房间信息呢？这个是不需要动的，能正在服务，说明能正常服务，如果服务过程中质量下滑到一定阈值，这个时候应该有一个线程标记当前线程为异常状态，后续不分配就行，
	//至于已经在较差的服务上的客户端，那么重连即可
	void planUpdateRouter();
	//mysql服务器返回数据之后的处理动作void udpateRouterMap();
	void DealingRequest(NetConnection* tcpClient, std::shared_ptr<mysqlService::GetRouterResponse> responseData);

	//这么多种消息（客户端的消息，流媒体等服务器的消息），肯定需要一个总函数进行分流
	//void handleUdpMsg();  //其实并不需要这个函数分流，而是直接多注册几个对应的id即可
	//所有消息进来的通道，因为这里采取了mysql的那种简化的架构，所以这里无用的声明都给删掉或者注释掉，不过一些注释能不删还是不删除
	template<class Conn_T, class Request_T>
	void requestHandle(Conn_T* conn, void* userData);

	//处理udp的server地址获取请求（流媒体或者基础服务器）
	void DealingRequest(Udp* udpServer, std::shared_ptr<lbService::GetServerRequest> requestData);

	//当有客户端请求时，分配服务器信息，如果当前文件对应的服务器存在，就直接从里面选取一个质量最好的服务器，如果都满负荷了，那么从routerMap里再挑选一对服务器，加入到map中
	//如果当前房间，服务器人数为0，直接删除
	void enterMediaRoom(int uid, int toFid);
	//如果请求的是聊天服务器信息，那么建立逻辑应该在这，那么客户端的请求应该包含哪些信息呢，模块id（和群聊id，如果是流媒体模块，是一个文件id）
	//（这个时候如果发一条消息应该怎么办，各个服务器那里都应该有一张这个表，里面记录者着有多少服务器服务这个群，然后当前服务器会通知各个服务器并附带消息）

	//如果是双人聊天怎么办呢，感觉上面聊天服务器以及流媒体服务器的聊天部分设计的不合理
	//聊天相关的东西都应该是发布订阅模式
	//也就是群号相当于订阅号，这个订阅号（vector）里有很多uid，当用户登录之后，应该分配一个chat服务器，作为基础服务器（她负责读取用户信息返回给用户
	// （包括当前用户的电影列表）），当用户点击相关影片之后应该再分配对应的流媒体服务器
	//当用户在自己的群聊聊天时，这个时候在线的人其实已经都有了自己的基础服务器（分配的时候随机分配就可以，因为全国的聊天服务器一个群不可能对应一个服务器），
	// 那么我发消息时，应该怎么办？应该把消息上传到负载均衡服务器上，
	//然后通知对应的组中对应的uid的当前所在的服务器去负载均衡服务器接收消息（显然负载均衡服务器这里应该有所有uid和已分配的服务器的对应关系）（负载均衡服务器这边可能需要
	// 去数据库那里找群对应的vector，然后再遍历vector，看看哪些用户在线，如果在线，就通知对应的服务器收消息）（数据库很显然要记录中个人信息那里要记录uid和gpid的对照表
	// 要不然你发都不知道发给谁）
	//那如果时两个人聊天呢，很显然去负载均衡服务器获得对应的服务器信息，然后把消息发到对应的服务器上就可以

	//返回一个基础服务器ip（chat服务器作为基础服务器）
	void returnBaseServer(int uid);

	//服务器反馈客户端的登录状态//客户端获得ip登录之后，服务器回复给lb服务器的消息接收处理函数void syncAcceptResult();
	void DealingRequest(Udp* udpServer, std::shared_ptr<lbService::ServerResonseToLb> acceptResult);
	//处理转发请求
	void DealingRequest(Udp* udpServer, std::shared_ptr<lbService::RepostMsgRequest> requestData);
	//处理私聊的转发逻辑
	void privateTalk(std::shared_ptr<lbService::RepostMsgRequest> requestData);
	//群聊转发处理//处理群聊的转发逻辑  void groupTalk();
	void DealingRequest(NetConnection* conn, std::shared_ptr<user::GetGroupMemberResponse> responseData);
	//获取群组在线成员请求
	void getGroupMemberList(std::shared_ptr<lbService::RepostMsgRequest> requestData);
	//转发业务
	//在编写聊天服务器的时候，增加了对repostMsgRequest的支持（也就是聊天服务器发过来一个转发请求，
	//我这边把消息发到指定的服务器上，让其转发到对应的conn上）void repostMsg();
	//将消息转发给对应的基础服务器
	//void repostResponseTo(lbService::RepostMsgRequest* requestData, sockaddr_in* to = nullptr);
	//因为群组的和私聊的转发逻辑不同，所以省略掉这个函数

	//停止服务请求（基础服务器给lb服务器的消息）
	void DealingRequest(Udp* udpServer, std::shared_ptr<lbService::StopServiceRequest> stopServiceData);
private:
	//有重复代码的辅助函数
	//udp回复消息给某个主机void UdpReply(int msgid, std::string& data, sockaddr_in* to = nullptr);
	template<class Response_T>
	void udpSendMsg(int responseID, Response_T* responseData, sockaddr_in* to);
	template<class Response_T>
	void tcpSendMsg(NetConnection* conn, int responseID, Response_T* responseData);
	//公共打包函数
	template<class Conn_T, class Response_T>
	void packageMsg(Conn_T* conn, int responseID, Response_T* responseData);
	//将若干个host消息打包消息到respnose（打包给udp，是GetRouterResponse类型的数据）
	void getServerResponse(int selectedModid, int uid, ServerAddr* server);
	//停止服务辅助函数
	bool handleStopService(int uid);
public:
	//两个服务器
	UdpServer* udpServer;   //负责处理客户端的udp查询，服务器端的udp查询
	TcpClient* tcpClient;  //连接mysql的
	ServerMap serverMap;   //管理Server集群的对象，包括排序，分配、回收等，注意这里先开辟模块set，如果不开启，后续会assert出错
	//注意上面的serverMap不能直接调用serverMap({1,2})会报错，这里只是声明一下，如果需要初始化，需要在LbAgent中调用拷贝构造或者赋值语句或者初始化列表初始化，初始化列表的时候只执行一次有参构造
	//std::map<int, mediaRoomSet> mediaRoomMap;   //先不涉及关于这个的
	//不需要排序，当然排序也行，而且其他地方包括这里不能直接获取ServerCondition，都要调用那个getServer方法，否则会出错
	//std::unordered_map<int, ServerAddr> user_ServerMap;  //注意这里不能加const，否则会报错   
	//注意这里从std::unordered_map<int, std::pair<ServerAddr, sockaddr_in>> user_ServerMap;变成std::unordered_map<int, ServerInfo> user_ServerMap;看流媒体服务器对应的设计文档
	std::unordered_map<int, ServerInfo> user_ServerMap;  //注意这里不能加const，否则会报错    
	//转发消息的时候需要用到udp地址，所以上面这个需要设计为对组的形式（基础服务器bug22），在同步登陆消息的时候，记录下对应的地址（第一个为tcp地址，第二个为udp地址）
	/*
		错误：将‘std::unordered_map<int, const ServerAddr>::mapped_type {aka const ServerAddr}’作为‘ServerAddr& ServerAddr::operator=(const ServerAddr&)’的‘this’实参时丢弃了类型限定 [-fpermissive]
	*/
	MsgCatch msgCatch;
};

