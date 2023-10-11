#include "mysqlService.pb.h"
#include "TcpClient.h"

//发送的辅助函数
template<class Response_T>
void packageAndResponse(NetConnection* conn, int responseID, Response_T* responseData)
{
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

//using hookCallBack = std::function<void(NetConnection* conn, void* userData)>;
//注意TcpClient也是一个NetConnection类型
void start(NetConnection* conn, void* userData) {
	//注意下面的只能一项一项的打开，因为一起打开有的会重定义
#if 0
	//1、给mysql代理服务器发送一个(int)router::ID_GetRouteRequest类型的消息
	mysqlService::GetRouterRequest GetRouterData;
	GetRouterData.set_modid(1);  //0序列化之后里面啥都没有
	packageAndResponse<mysqlService::GetRouterRequest>(conn, (int)mysqlService::ID_GetRouterRequest, &GetRouterData);

	//2、给mysql代理服务器发送一个(int)router::ID_GetUserBaseRequest类型的消息
	mysqlService::GetUserBaseRequest GetUserBaseData;
	GetUserBaseData.set_uid(1);  //查询用户1的个人信息
	packageAndResponse<mysqlService::GetUserBaseRequest>(conn, (int)mysqlService::ID_GetUserBaseRequest, &GetUserBaseData);

	//3.1、给mysql代理服务器发送一个(int)router::ID_ModifyUserRequest类型的消息
	mysqlService::ModifyUserRequest ModifyUserData;
	ModifyUserData.set_uid(1);  //修改用户1的个人信息
	mysqlService::ColmPair* colm;
	colm = ModifyUserData.add_colms();
	colm->set_colmname("`name`");
	colm->set_name("'小李子'");
	colm = ModifyUserData.add_colms();
	colm->set_colmname("`passwd`");
	colm->set_passwd("55555");
	colm = ModifyUserData.add_colms();
	colm->set_colmname("`state`");
	colm->set_state(1);
	packageAndResponse<mysqlService::ModifyUserRequest>(conn, (int)mysqlService::ID_ModifyUserRequest, &ModifyUserData);

	//3.2验证
	mysqlService::GetUserBaseRequest verifyModi;
	verifyModi.set_uid(1);  //查询用户1的个人信息
	packageAndResponse<mysqlService::GetUserBaseRequest>(conn, (int)mysqlService::ID_GetUserBaseRequest, &verifyModi);

	//4、插入一个用户数据
	mysqlService::InsertUserRequest InsertUserData;
	InsertUserData.set_uid(5495);  //这个uid模拟随机生成的一个id
	InsertUserData.set_name("'张三'");
	InsertUserData.set_passwd("7777");  //这个uid模拟随机生成的一个id
	packageAndResponse<mysqlService::InsertUserRequest>(conn, (int)mysqlService::ID_InsertUserRequest, &InsertUserData);
	//5删除一个用户数据
	mysqlService::DeleteUserRequest DeleteUserData;
	DeleteUserData.set_uid(1);  //删除的uid
	DeleteUserData.set_name("小李");
	DeleteUserData.set_passwd("1234");
	packageAndResponse<mysqlService::DeleteUserRequest>(conn, (int)mysqlService::ID_DeleteUserRequest, &DeleteUserData);

	//6、获取某个用户的喜欢列表
	mysqlService::GetUserFavoriteRequest GetUserFavoriteData;
	GetUserFavoriteData.set_uid(1);
	packageAndResponse<mysqlService::GetUserFavoriteRequest>(conn, (int)mysqlService::ID_GetUserFavoriteRequest, &GetUserFavoriteData);


	//7.1、某个用户添加一个喜欢的电影
	mysqlService::InsertFavoriteRequest InsertFavoriteData;
	InsertFavoriteData.set_uid(1);
	InsertFavoriteData.set_fid(3);
	packageAndResponse<mysqlService::InsertFavoriteRequest>(conn, (int)mysqlService::ID_InsertFavoriteRequest, &InsertFavoriteData);
	//7.2、验证拉取看看是否获取成功
	mysqlService::GetUserFavoriteRequest GetUserFavoriteData;
	GetUserFavoriteData.set_uid(1);
	packageAndResponse<mysqlService::GetUserFavoriteRequest>(conn, (int)mysqlService::ID_GetUserFavoriteRequest, &GetUserFavoriteData);

	//8.1、某个用户删除一个喜欢的电影
	mysqlService::DeleteFavoriteRequest DeleteFavoriteData;
	DeleteFavoriteData.set_uid(1);
	DeleteFavoriteData.set_fid(3);
	packageAndResponse<mysqlService::DeleteFavoriteRequest>(conn, (int)mysqlService::ID_DeleteFavoriteRequest, &DeleteFavoriteData);
	//8.2、验证拉取看看是否删除成功
	mysqlService::GetUserFavoriteRequest GetUserFavoriteData;
	GetUserFavoriteData.set_uid(1);
	packageAndResponse<mysqlService::GetUserFavoriteRequest>(conn, (int)mysqlService::ID_GetUserFavoriteRequest, &GetUserFavoriteData);
	//7和8没有添加响应函数，都是直接忽略

	//9、获得某个用户的朋友列表
	mysqlService::GetFriendsRequest GetFriendsData;
	GetFriendsData.set_uid(1);
	packageAndResponse<mysqlService::GetFriendsRequest>(conn, (int)mysqlService::ID_GetFriendsRequest, &GetFriendsData);

	//10.1、添加一个朋友
	mysqlService::InsertFriendRequest InsertFriendData;
	InsertFriendData.set_uid(1);
	InsertFriendData.set_friend_id(6);
	packageAndResponse<mysqlService::InsertFriendRequest>(conn, (int)mysqlService::ID_InsertFriendRequest, &InsertFriendData);
	//10.2、验证拉取看看是否获取成功
	mysqlService::GetFriendsRequest GetFriendsData;
	GetFriendsData.set_uid(1);
	packageAndResponse<mysqlService::GetFriendsRequest>(conn, (int)mysqlService::ID_GetFriendsRequest, &GetFriendsData);

	//11.1、删除一个朋友
	mysqlService::DeleteFriendRequest DeleteFriendData;
	DeleteFriendData.set_uid(1);
	DeleteFriendData.set_friend_id(6);
	packageAndResponse<mysqlService::DeleteFriendRequest>(conn, (int)mysqlService::ID_DeleteFriendRequest, &DeleteFriendData);
	//11.2、验证拉取看看是否获取成功
	mysqlService::GetFriendsRequest GetFriendsData;
	GetFriendsData.set_uid(1);
	packageAndResponse<mysqlService::GetFriendsRequest>(conn, (int)mysqlService::ID_GetFriendsRequest, &GetFriendsData);
	//10、11没有响应函数

	//12、创建一个组
	mysqlService::RegisterGroupRequest RegisterGroupData;
	RegisterGroupData.set_uid(1);  //群主uid
	RegisterGroupData.set_name("'电影交流群'");
	RegisterGroupData.set_summary("'深入讨论电影的精华之处'"); 
	packageAndResponse<mysqlService::RegisterGroupRequest>(conn, (int)mysqlService::ID_RegisterGroupRequest, &RegisterGroupData);
	//13、删除一个组
	mysqlService::DeleteGroupRequest DeleteGroupData;
	DeleteGroupData.set_uid(1);  //群主uid
	DeleteGroupData.set_gid(3);  //删除的群号
	packageAndResponse<mysqlService::DeleteGroupRequest>(conn, (int)mysqlService::ID_DeleteGroupRequest, &DeleteGroupData);

	//14、查询满足一定条件的所有组成员
	mysqlService::GetGroupMemberRequest GetGroupMemberData;
	GetGroupMemberData.set_uid(1);  //查询的人的uid，可以后面用来判断当前uid是否有资格查
	GetGroupMemberData.set_gid(1);  //查询群号
	//GetGroupMemberData.set_condition("'state=0'");  //状态是在线的  这个是不正确的，查询结果为空
	GetGroupMemberData.set_condition("state=1");  //状态不在线的 
	packageAndResponse<mysqlService::GetGroupMemberRequest>(conn, (int)mysqlService::ID_GetGroupMemberRequest, &GetGroupMemberData);

	//15、获得某用户的组列表
	mysqlService::GetUserGroupsRequest GetUserGroupsData;
	GetUserGroupsData.set_uid(2); 
	packageAndResponse<mysqlService::GetUserGroupsRequest>(conn, (int)mysqlService::ID_GetUserGroupsRequest, &GetUserGroupsData);
	
	//16、增加一个成员
	mysqlService::AddUserToGroupRequest AddUserToGroupData;
	AddUserToGroupData.set_uid(6);
	AddUserToGroupData.set_gid(1);
	packageAndResponse<mysqlService::AddUserToGroupRequest>(conn, (int)mysqlService::ID_AddUserToGroupRequest, &AddUserToGroupData);

#endif
	//17、删除一个群成员，这个只能由管理员和本人有权力
	mysqlService::DelUserFromGroupRequest DelUserFromGroupData;
	DelUserFromGroupData.set_uid(6);
	DelUserFromGroupData.set_gid(1);
	packageAndResponse<mysqlService::DelUserFromGroupRequest>(conn, (int)mysqlService::ID_DelUserFromGroupRequest, &DelUserFromGroupData);
}

//1、注册一个router::ID_GetRouterResponse类型的回调函数，用来处理mysql代理服务器响应的数据
//using msgCallBack = std::function<void(NetConnection* conn, void* userData)>;
void handleGetRouterResponse(NetConnection* conn, void* userData) {
	mysqlService::GetRouterResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "modid" << responseData.modid() << std::endl;
	for (int i = 0; i < responseData.host_size(); i++) {
		std::cout << "ip：" << responseData.host(i).ip() << "\tport：" << responseData.host(i).port() << std::endl;
	}
}

//2、查询个人信息响应的函数
void handleGetUserBaseResponse(NetConnection* conn, void* userData) {
	mysqlService::GetUserBaseResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "isgot:" << responseData.isgot() << std::endl;
	std::cout << "uid：" << responseData.uid() << "\tname：" << responseData.name() << "\tpasswd：" << responseData.passwd() << "\tstate：" << responseData.state() << std::endl;
}

//3、修改、添加、删除个人信息响应
void handleUpdateUserResponse(NetConnection* conn, void* userData) {
	mysqlService::UpdateUserResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "uid：" << responseData.uid() << "\t操作类型：" << responseData.updatekind() << "\t操作结果：" << responseData.result() << std::endl;
}

//6、查询个人favorites的响应
void handleGetUserFavoriteResponse(NetConnection* conn, void* userData) {
	mysqlService::GetUserFavoriteResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "uid：" << responseData.uid() << "喜欢的电影列表："<< std::endl;
	for (int i = 0; i < responseData.file_size(); i++) {
		std::cout << i  << "：" << responseData.file(i).name() << std::endl;
	}
}

//9、获得某人的朋友列表
void handleGetFriendsResponse(NetConnection* conn, void* userData) {
	mysqlService::GetFriendsResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "uid：" << responseData.uid() << "的朋友列表：" << std::endl;
	for (int i = 0; i < responseData.friend__size(); i++) {  //可能是因为friend是友元关键字。。。。
		std::cout << i << "：" << responseData.friend_(i).name() << std::endl;
	}
}

//12、注册一个群组响应
void handleRegisterGroupResponse(NetConnection* conn, void* userData) {
	mysqlService::RegisterGroupResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "gid为：" << responseData.gid() << std::endl;
	//可以在这里查询当前群成员吗，不过我这里只是简单验证群号，然后手动验证是否有成员
}

//13、删除一个群组响应  这个响应是有必要的，因为只有群主可以删掉
void handleDeleteGroupResponse(NetConnection* conn, void* userData) {
	mysqlService::DeleteGroupResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "uid"<< responseData.uid() <<"\t删除群号"<< responseData.gid()<< "的结果为：" << responseData.result() << std::endl;
}

//14、查询一定组员信息的响应
void handleGetGroupMemberResponse(NetConnection* conn, void* userData) {
	mysqlService::GetGroupMemberResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "查询人员uid" << responseData.uid() << "\t查询的群号" << responseData.gid() <<std::endl;
	std::cout << "人员列表：" << std::endl;
	for (int i = 0; i < responseData.member_size(); i++) {
		std::cout << i << "：" << responseData.member(i).name() << std::endl;
	}
}

//15、查询某个用户的群列表响应
void handleGetUserGroupsResponse(NetConnection* conn, void* userData) {
	mysqlService::GetUserGroupsResponse responseData;
	responseData.ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
	std::cout << "用户uid" << responseData.uid() << std::endl;
	std::cout << "群列表：" << std::endl;
	for (int i = 0; i < responseData.group_size(); i++) {
		std::cout << i << "：" << responseData.group(i).name() << std::endl;
	}
}


int main(int argc, void* arg[])
{
	TcpClient* client = new TcpClient("127.0.0.1", 10000);
	client->setConnStart(start);
	client->addMsgRouter((int)mysqlService::ID_GetRouterResponse, handleGetRouterResponse);
	client->addMsgRouter((int)mysqlService::ID_GetUserBaseResponse, handleGetUserBaseResponse);
	client->addMsgRouter((int)mysqlService::ID_UpdateUserResponse, handleUpdateUserResponse);
	client->addMsgRouter((int)mysqlService::ID_GetUserFavoriteResponse, handleGetUserFavoriteResponse);
	client->addMsgRouter((int)mysqlService::ID_GetFriendsResponse, handleGetFriendsResponse);
	client->addMsgRouter((int)mysqlService::ID_RegisterGroupResponse, handleRegisterGroupResponse);
	client->addMsgRouter((int)mysqlService::ID_DeleteGroupResponse, handleDeleteGroupResponse);
	client->addMsgRouter((int)mysqlService::ID_GetGroupMemberResponse, handleGetGroupMemberResponse);
	client->addMsgRouter((int)mysqlService::ID_GetUserGroupsResponse, handleGetUserGroupsResponse);
	client->connectServer();
	client->m_evloop->run();
}
