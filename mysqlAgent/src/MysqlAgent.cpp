#include "MysqlAgent.h"
#include <functional>

MysqlAgent::MysqlAgent(unsigned short port, int threadNums, std::string dbName)
{
	Debug("开始初始化mysqlAgent");
	m_server = new TcpServer(port, threadNums);
	m_pool = ConnectPool::getConnectPool(dbName);
	//处理路由表的函数
	m_server->addMsgRouter((int)mysqlService::ID_GetRouterRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::GetRouterRequest, mysqlService::GetRouterResponse>, this, std::placeholders::_1, std::placeholders::_2)); //上面的模板参数一确定，这里的sql语句已经确定了，
	//所以在这里直接传进去，保证handleRequest有更强的通用性
	//还传什么sql语句呀，把查询或者修改的操作放到analyzeQueryResult里，就完全不需要考虑传递指令的问题了，因为特定的处理函数那里肯定知道使用哪个sql语句
	//处理user表的查询基本信息的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_GetUserBaseRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::GetUserBaseRequest, mysqlService::GetUserBaseResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理user表修改信息的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_ModifyUserRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::ModifyUserRequest, mysqlService::UpdateUserResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理user表添加用户的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_InsertUserRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::InsertUserRequest, mysqlService::UpdateUserResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理user表删除用户的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_DeleteUserRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::DeleteUserRequest, mysqlService::UpdateUserResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理user表查询用户喜欢电影的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_GetUserFavoriteRequest, 
		std::bind(&MysqlAgent::requestHandle<user::GetUserFavoriteRequest, user::GetUserFavoriteResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理user表添加用户喜欢电影的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_InsertFavoriteRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::InsertFavoriteRequest, mysqlService::UpdateFavoriteResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理user表删除用户喜欢电影的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_DeleteFavoriteRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::DeleteFavoriteRequest, mysqlService::UpdateFavoriteResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理friends表获得朋友列表的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_GetFriendsRequest, 
		std::bind(&MysqlAgent::requestHandle<user::GetFriendsRequest, user::GetFriendsResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理friends表添加朋友的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_InsertFriendRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::InsertFriendRequest, mysqlService::UpdateFriendResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理friends表删除朋友的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_DeleteFriendRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::DeleteFriendRequest, mysqlService::UpdateFriendResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理group表注册群组的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_RegisterGroupRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::RegisterGroupRequest, mysqlService::RegisterGroupResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理group表删除群组的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_DeleteGroupRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::DeleteGroupRequest, mysqlService::DeleteGroupResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理group表，获得某个组的一定条件的组员信息
	m_server->addMsgRouter((int)mysqlService::ID_GetGroupMemberRequest,
		std::bind(&MysqlAgent::requestHandle<user::GetGroupMemberRequest, user::GetGroupMemberResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理group表某用户群组列表的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_GetUserGroupsRequest, 
		std::bind(&MysqlAgent::requestHandle<user::GetUserGroupsRequest, user::GetUserGroupsResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理group表某群添加成员的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_AddUserToGroupRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::AddUserToGroupRequest, mysqlService::AddUserToGroupResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理group表某群删除成员的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_DelUserFromGroupRequest, 
		std::bind(&MysqlAgent::requestHandle<mysqlService::DelUserFromGroupRequest, mysqlService::DelUserFromGroupResponse>, this, std::placeholders::_1, std::placeholders::_2));
	//处理movies表，获取电影基本信息的回调函数
	m_server->addMsgRouter((int)mysqlService::ID_GetMovieInfoRequest,
		std::bind(&MysqlAgent::requestHandle<mysqlService::GetMovieInfoRequest, mysqlService::GetMovieInfoResponse>, this, std::placeholders::_1, std::placeholders::_2));
	Debug("mysqlAgent初始化完成");
	//初始化mysql语句表，这些应该是从文件中读取的，但是这里直接读了
	//第一个0代表哪张表，第二个代表这个表的指令
	//1、router表
	sqls[0][0] = "select * from router";
	sqls[0][1] = "select * from router where modid = %d";  //查询某个业务服务器的所有ip地址
	//2、users表
	sqls[1][0] = "select * from `users` where uid = %d";  //查询uid的个人信息
	sqls[1][1] = "update `users` set %s where uid = %d";  //修改uid的个人信息
	sqls[1][2] = "insert into `users` values(NULL, '%s', '%s', 1)";  //插入一个用户信息,1代表已经注册完并且登录
	sqls[1][3] = "delete from `users`  where uid = %d";  //删除一个用户信息
	//获得用户的最喜欢的电影列表，名称，uid
	sqls[1][4] = "select movies.fid, movies.name from `users` join `favorites` on `users`.uid = `favorites`.uid join `movies` on `favorites`.fid =`movies`.fid where `users`.uid = %d"; 
	//添加一条favorite
	sqls[1][5] = "insert into `favorites` values(%d, %d)"; 
	//删除一条favorite
	sqls[1][6] = "delete from `favorites`  where uid = %d and fid = %d";
	//查询用户的朋友列表uid，name
	sqls[1][7] = "select `users`.uid, `users`.name from `users` where uid in"
		"(select `friends`.friend_id from `friends` where `uid` = %d"
		" union "
		"select `friends`.uid from `friends` where `friend_id` = %d)";
	//添加一个朋友
	sqls[1][8] = "insert into `friends` values(%d, %d)";
	//删除一个朋友
	sqls[1][9] = "delete from `friends`  where (uid = %d and friend_id = %d ) or (uid = %d and friend_id = %d)";
	//删除所有的喜欢列表
	sqls[1][10] = "delete from `favorites`  where uid = %d";
	//删除所有的朋友
	sqls[1][11] = "delete from `friends`  where uid = %d or friend_id = %d ";
	//3、group表
	//注册一个group
	sqls[2][0] = "insert into `group` values(NULL, '%s', '%s', %d, 1)";
	sqls[2][1] = "insert into `group_member` values(%d, %d)";
	//删除一个group
	sqls[2][2] = "delete from `group_member` where `gid` = %d";
	sqls[2][3] = "delete from `group` where `gid` = %d";
	//查询满足一定条件的所有组员信息
	sqls[2][4] = "select `uid`, `name` from `users` where uid in (select `uid` from `group_member` where `gid` = %d) and %s";
	//查询某用户的群组列表
	sqls[2][5] = "select `gid`, `name` from `group` where `gid` in (select `gid` from `group_member` where `uid` = %d)";
	//删除某群组的某个成员  --注意这里没有增加的是因为21已经有了
	sqls[2][6] = "delete from `group_member` where `gid` = %d and `uid` = %d";
	//退出所有组
	sqls[2][7] = "delete from `group_member` where `uid` = %d";
	//修改群组信息,暂时还没对外提供，只是供增加删除组成员时更改成员人数
	sqls[2][8] = "update `group` set %s where `gid` = %d";

	//4、movies表
	sqls[3][0] = "select * from movies where fid = %d";
	//登录操作需要 查询、修改 状态
	//注册操作需要 增加 用户表数据或者group表数据，以及friend表数据，member数据
	//添加组或者朋友操作需要添加 frined表数据，member数据
}

MysqlAgent::~MysqlAgent()
{
}

void MysqlAgent::run()
{
	m_server->run();
}

//公共函数
template<class Request_T, class Response_T>
void MysqlAgent::requestHandle(NetConnection* conn, void* userData)
{
	Debug("requestHandle开始执行");
	//解析data数据（接收数据已经交给底层tcpserver了）
	Request_T requestData;
	requestData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);

	//取出一条连接根据解析的数据进行查询,并解析数据,并将数据放到responseData里
	//路由的响应消息
	Response_T responseData;
	queryAndDeal(conn, &requestData, &responseData);  //requestsql注意内部函数是requestsql而不是requestSql
}

//公共函数
template<class Request_T, class Response_T>
void MysqlAgent::queryAndDeal(NetConnection* clientConn, Request_T* requestData, Response_T* responseData)
{
	MysqlConn* conn = m_pool->getConnect();
	//conn->query(sql);  //把这个放到下面那个函数里，主要是为了sql语句指定的方便，因为在analyzeQueryResult里肯定知道该用哪个sql语句
	//根据request的数据构造sql语句查询并分析结果
	int responseType = 0;
	analyzeQueryResult(conn, requestData, responseType, responseData);
	m_pool->putBackConnect(conn);
	//组织响应并回复  这个函数放在这里比较好，因为分析完之后就不需要mysql连接的缓冲区了，可以直接释放掉，如果放到analyzeQueryResult，那么就连接的时间就变多了
	packageAndResponse(clientConn, responseType, responseData);
}

//公共函数
template<class Response_T>
void MysqlAgent::packageAndResponse(NetConnection* conn, int responseID, Response_T* responseData)
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
	Debug("response->m_msglen:%ld", responseSerial.size());
	response->m_state = HandleState::Done; //这一句代表当前响应处理完成，可以让sendMsg函数开始工作了
	//发送数据到对端
	conn->sendMsg();
}

//注意这些函数不能用模板函数，因为模板函数是流程一致的，而分析具体结果不同的指令结果不同，所以只能用重载
//处理路由表查询的函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::GetRouterRequest* requestData,
	int& responseType, mysqlService::GetRouterResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[0][1].data(), requestData->modid());  //modid代表什么服务器
	//查询
	conn->query(sql);  //注意不能想当然的在这直接放回连接，如果放回，那么这次读的数据可能就被其他查询丢失了
	//router::HostInfo host;
	mysqlService::HostInfo* host;
	while (conn->next()) {
		//解析模块id（是聊天服务还是流媒体）
		//| id     | modid | server_ip | server_port | modify_time         |
		responseData->set_modid(std::stoi(conn->value(1)));
		//host.set_ip(std::stoi(conn->value(2)));
		//host.set_port(std::stoi(conn->value(3)));
		//responseData->add_host(&host);  注意这样用的不对，不是先初始化host，再添加，而是先添加再初始化，可以参考这篇文章怎么使用repeated https://www.cnblogs.com/longchang/p/12859475.html
		host = responseData->add_host();
		host->set_ip(conn->value(2));
		host->set_port(std::stoi(conn->value(3)));
	}
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_GetRouterResponse;
}

//处理users表查询基础信息的函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::GetUserBaseRequest* requestData,
	int& responseType, mysqlService::GetUserBaseResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][0].data(), requestData->uid());
	//sprintf(sql.data(), sqls[1][0], requestData->uid());    //不能这样写，这样写返回的是会报错
	//查询
	conn->query(sql);  //注意不能想当然的在这直接放回连接，如果放回，那么这次读的数据可能就被其他查询丢失了
	responseData->set_isgot(false);
	responseData->set_uid(requestData->uid());  //这里一定设置为这个值，这个是如果查不到，也会返回当前的uid，代表当前uid查不到，要不然客户端那边不知道哪个uid查不到
	while (conn->next()) {
		responseData->set_isgot(true);
		responseData->set_uid(std::stoi(conn->value(0)));
		responseData->set_name(conn->value(1));
		responseData->set_passwd(conn->value(2));
		responseData->set_state(std::stoi(conn->value(3)));
	}
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_GetUserBaseResponse;
}

//处理users表修改基础信息的函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::ModifyUserRequest* requestData,
	int& responseType, mysqlService::UpdateUserResponse* responseData)
{
	//组织sql语句
	//拼凑条件
	std::string colmsVal;
	for (int i = 0; i < requestData->colms_size(); i++) {
		std::string temp = requestData->colms(i).colmname();
		temp += "=";
		if (requestData->colms(i).has_name()) {
			temp += requestData->colms(i).name();
		}
		else if (requestData->colms(i).has_passwd()) {
			temp += requestData->colms(i).passwd();
		}
		else if (requestData->colms(i).has_state()) {
			temp += std::to_string(requestData->colms(i).state());
		}
		else {
			continue;
		}
		colmsVal += temp;
		if (i != requestData->colms_size() - 1) {
			colmsVal += ",";
		}
	}
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][1].data(), colmsVal.data(), requestData->uid());
	Debug("修改基础信息_sql:%s", sql);
	//更新
	if (conn->update(sql)) {
		responseData->set_result(1);
	}
	else {
		responseData->set_result(-1);
	}
	responseData->set_uid(requestData->uid());
	responseData->set_updatekind(1);//操作类型，1代表修改，2代表删除，3代表插入
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_UpdateUserResponse;
}

//处理users表添加基础信息的函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::InsertUserRequest* requestData,
	int& responseType, mysqlService::UpdateUserResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][2].data(), requestData->name().data(), requestData->passwd().data());
	Debug("添加基础信息_sql:%s", sql);
	//添加操作
	conn->update(sql);
	//查询最后的那个信息，返回给对端
	conn->query("select last_insert_id()");  //返回的结果是一行一列的最大的id值，注意这个连接必须是没关闭的，否则会返回null，当然也有其他方法获取最新的id
	//select last_insert_id()这个很有可能错误，有挺多坑的https://blog.csdn.net/wangzhezizun/article/details/84533572
	responseData->set_uid(requestData->uid());
	while (conn->next()) {
		int registedUid = std::stoi(conn->value(0));
		if (registedUid != 0) {
			responseData->set_result(registedUid);
		}
		else {
			responseData->set_result(requestData->uid());
		}
		Debug("添加用户,返回的uid:%d", responseData->uid());
	}
	responseData->set_updatekind(3);//操作类型，1代表修改，2代表删除，3代表插入
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_UpdateUserResponse;
}

//处理users表删除基础信息的函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteUserRequest* requestData,
	int& responseType, mysqlService::UpdateUserResponse* responseData)
{
	char sql[256] = { 0 };
	//注意删除user表之前需要清理组成员表，朋友关系表，喜欢的电影表
	sprintf(sql, sqls[1][10].data(), requestData->uid());
	Debug("删除喜欢列表_sql:%s", sql);
	conn->update(sql);
	memset(sql, 0, 256);
	sprintf(sql, sqls[1][11].data(), requestData->uid(), requestData->uid());
	Debug("删除朋友列表_sql:%s", sql);
	conn->update(sql);
	memset(sql, 0, 256);
	sprintf(sql, sqls[2][7].data(), requestData->uid());
	Debug("删除组成员列表_sql:%s", sql);
	//组织sql语句，删除user成员
	memset(sql, 0, 256);
	sprintf(sql, sqls[1][3].data(), requestData->uid());
	conn->update(sql);
	//更新
	if (conn->update(sql)) {
		responseData->set_result(1);
	}
	else {
		responseData->set_result(-1);
	}
	responseData->set_uid(requestData->uid());
	responseData->set_updatekind(2);//操作类型，1代表修改，2代表删除，3代表插入
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_UpdateUserResponse;
}

//处理users表获取favorites的表函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, user::GetUserFavoriteRequest* requestData,
	int& responseType, user::GetUserFavoriteResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][4].data(), requestData->uid());
	//更新
	conn->query(sql);
	responseData->set_uid(requestData->uid());
	user::File* file;
	while (conn->next()) {
		file = responseData->add_file(); 
		file->set_fid(std::stoi(conn->value(0)));
		file->set_name(conn->value(1));
	}
	responseType = (int)mysqlService::ID_GetUserFavoriteResponse;
}

//处理favorites表的添加函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::InsertFavoriteRequest* requestData,
	int& responseType, mysqlService::UpdateFavoriteResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][5].data(), requestData->uid(), requestData->fid());
	//更新
	if (conn->update(sql)) {
		responseData->set_result(1);
	}
	else {
		responseData->set_result(-1);
	}
	responseData->set_uid(requestData->uid());
	responseData->set_updatekind(1);	//操作类型，1代表添加，2代表删除
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_UpdateFavoriteResponse;
}

//处理favorites表的删除函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteFavoriteRequest* requestData,
	int& responseType, mysqlService::UpdateFavoriteResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][6].data(), requestData->uid(), requestData->fid());
	//更新
	if (conn->update(sql)) {
		responseData->set_result(1);
	}
	else {
		responseData->set_result(-1);
	}
	responseData->set_uid(requestData->uid());
	responseData->set_updatekind(2);	//操作类型，1代表添加，2代表删除
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_UpdateFavoriteResponse;
}

//处理friends表，查询某个用户的朋友列表函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, user::GetFriendsRequest* requestData,
	int& responseType, user::GetFriendsResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][7].data(), requestData->uid(), requestData->uid());
	//更新
	conn->query(sql);
	responseData->set_uid(requestData->uid());
	user::Person* person;
	while (conn->next()) {
		person = responseData->add_friend_();    //不知道这里的friend为啥变成了friend_,可能还有一个member使用了person
		person->set_uid(std::stoi(conn->value(0)));
		person->set_name(conn->value(1));
	}
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_GetFriendsResponse;
}

//处理friends表，添加某个朋友
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::InsertFriendRequest* requestData,
	int& responseType, mysqlService::UpdateFriendResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][8].data(), requestData->uid(), requestData->friend_id());
	//更新
	if (conn->update(sql)) {
		responseData->set_result(1);
	}
	else {
		responseData->set_result(-1);
	}
	responseData->set_uid(requestData->uid());
	responseData->set_updatekind(1);//操作类型，1代表添加，2代表删除
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_UpdateFriendResponse;
}

//处理friends表，删除某个朋友
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteFriendRequest* requestData,
	int& responseType, mysqlService::UpdateFriendResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[1][9].data(), requestData->uid(), requestData->friend_id(), requestData->friend_id(), requestData->uid());
	//更新
	if (conn->update(sql)) {
		responseData->set_result(1);
	}
	else {
		responseData->set_result(-1);
	}
	responseData->set_uid(requestData->uid());
	responseData->set_updatekind(2);//操作类型，1代表添加，2代表删除
	//指定返回的消息类型
	responseType = (int)mysqlService::ID_UpdateFriendResponse;
}

//处理group表，注册一个group
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::RegisterGroupRequest* requestData,
	int& responseType, mysqlService::RegisterGroupResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[2][0].data(), requestData->name().data(), requestData->summary().data(), requestData->uid());
	//更新
	if (conn->update(sql)) {
		responseData->set_result(1);
		conn->query("select last_insert_id()"); 
		while (conn->next()) {
			responseData->set_gid(std::stoi(conn->value(0)));
		}
		//添加当前成员为组成员
		memset(sql, 0, 256);
		sprintf(sql, sqls[2][1].data(), responseData->gid(), requestData->uid());
		conn->update(sql);
	}
	else {
		Debug("注册失败");
		responseData->set_result(-1);
	}
	responseData->set_uid(requestData->uid());
	responseType = (int)mysqlService::ID_RegisterGroupResponse;
}

//处理group表，删除一个group
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteGroupRequest* requestData,
	int& responseType, mysqlService::DeleteGroupResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	//先查找是否是群主，如果是直接删掉，否则返回删除失败的结果
	sprintf(sql, "select `owner` from `group` where `gid` = %d", requestData->gid());
	conn->query(sql);
	conn->next();
	if (std::stoi(conn->value(0)) != requestData->uid()) {
		responseData->set_result(-1);
	}
	else {
		//删除成员
		memset(sql, 0, 256);
		sprintf(sql, sqls[2][2].data(), requestData->gid());
		conn->update(sql);
		//清除组信息
		memset(sql, 0, 256);
		sprintf(sql, sqls[2][3].data(), requestData->gid());
		conn->update(sql);
		responseData->set_result(1);
	}
	responseData->set_uid(requestData->uid());
	responseData->set_gid(requestData->gid());
	responseType = (int)mysqlService::ID_DeleteGroupResponse;
}

//处理group表，获得某个组的一定条件的组员信息
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, user::GetGroupMemberRequest* requestData,
	int& responseType, user::GetGroupMemberResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[2][4].data(), requestData->gid(), requestData->condition().data());
	Debug("获得群组一定条件成员列表_sql:%s", sql);
	conn->query(sql);
	responseData->set_uid(requestData->uid());
	responseData->set_gid(requestData->gid());
	user::Person* member;
	while (conn->next()) {
		member = responseData->add_member();
		member->set_uid(std::stoi(conn->value(0)));
		member->set_name(conn->value(1));
	}
	responseType = (int)mysqlService::ID_GetGroupMemberResponse;
}

//处理group表，获得某个用户的群列表
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, user::GetUserGroupsRequest* requestData,
	int& responseType, user::GetUserGroupsResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	//先查找是否是群主，如果是直接删掉，否则返回删除失败的结果
	sprintf(sql, sqls[2][5].data(), requestData->uid());
	conn->query(sql);
	responseData->set_uid(requestData->uid());
	user::ChatGroup* group;
	while (conn->next()) {
		group = responseData->add_group();
		group->set_gid(std::stoi(conn->value(0)));
		group->set_name(conn->value(1));
	}
	responseType = (int)mysqlService::ID_GetUserGroupsResponse;
}

//处理group表，从某群组中增加某个成员
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::AddUserToGroupRequest* requestData,
	int& responseType, mysqlService::AddUserToGroupResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[2][1].data(), requestData->gid(), requestData->uid());
	if (conn->update(sql)) {
		//这个时候成员人数应该+1
		memset(sql, 0, 256);
		sprintf(sql, sqls[2][8].data(), "member_nums=member_nums+1", requestData->gid());
		conn->update(sql);
		responseData->set_result(1);
		responseData->set_uid(requestData->uid());
		responseData->set_gid(requestData->gid());
	}
	else {
		responseData->set_result(-1);
	}
	responseType = (int)mysqlService::ID_AddUserToGroupResponse;
}

//处理group表，从某群组中删除某个成员
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::DelUserFromGroupRequest* requestData,
	int& responseType, mysqlService::DelUserFromGroupResponse* responseData)
{
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[2][6].data(), requestData->gid(), requestData->uid());
	if (conn->update(sql)) {
		//这个时候成员人数应该-1
		memset(sql, 0, 256);
		sprintf(sql, sqls[2][8].data(), "member_nums=member_nums-1", requestData->gid());
		conn->update(sql);
		responseData->set_result(1);
		responseData->set_uid(requestData->uid());
		responseData->set_gid(requestData->gid());
	}
	else {
		responseData->set_result(-1);
	}
	responseType = (int)mysqlService::ID_DelUserFromGroupResponse;
}

//处理movies表，获取电影基本信息的回调函数
void MysqlAgent::analyzeQueryResult(MysqlConn* conn, mysqlService::GetMovieInfoRequest* requestData,
	int& responseType, mysqlService::GetMovieInfoResponse* responseData) {
	//组织sql语句
	char sql[256] = { 0 };
	sprintf(sql, sqls[3][0].data(), requestData->fid());
	conn->query(sql);
	responseData->set_toid(requestData->fromid());
	while (conn->next()) {
		responseData->set_fid(requestData->fid());
		responseData->set_name(conn->value(1));
		responseData->set_path(conn->value(2));
		responseData->set_summary(conn->value(3));
	}
	responseType = (int)mysqlService::ID_GetMovieInfoResponse;
}