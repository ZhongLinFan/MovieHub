#pragma once
#include "mysqlService.pb.h"
#include "user.pb.h"  //用户基础信息格式，注意这个不再包含于mysqlService
#include "TcpServer.h"
#include "ConnectPool.h"
#include <map>

//感觉这样写好麻烦，我写到一半发现其实每台服务器都有一个自己的连接池会更好，因为那样就涉及不到sql语句的拼凑，想怎么查怎么查，不需要使用protobuf编解码进行一层传输
//而是只需要取连接，查询sql，然后解析结果就行，这里多了一层编解码


class MysqlAgent {
public:
	MysqlAgent(unsigned short port, int threadNums, std::string dbName);
	~MysqlAgent();

	//启动MysqlAgent
	void run();
	
	//处理负载均衡服务器、聊天服务器和流媒体服务器的响应函数（因为发现他们的流程都是一样的，只不过analyzeQueryResult不一样，所以可以放到一起）
	// 但是后面发现前面反序列化的时候也不一样，这个时候就需要模板了
	//处理格式要符合using msgCallBack = std::function<void(NetConnection* conn, void* userData)>;
	template<class Request_T, class Response_T>
	void requestHandle(NetConnection* conn, void* userData); //可以确保肯定是NetConnection，所以不需要模板化

private:
	//数据库查询操作并解析数据操作
	template<class Request_T, class Response_T>
	void queryAndDeal(NetConnection* clientConn, Request_T* requestData, Response_T* responseData);
	template<class Response_T>
	void packageAndResponse(NetConnection* conn, int responseID, Response_T* responseData);

	//分析必须要传MysqlConn* conn而不能只传递那个结果，因为每行的数据会保存在conn里
	//处理路由表的函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::GetRouterRequest* requestData, int& responseType, mysqlService::GetRouterResponse* responseData);
	//处理users表查询个人基础信息的函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::GetUserBaseRequest* requestData, int& responseType, mysqlService::GetUserBaseResponse* responseData);
	//处理users表修改个人基础信息的函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::ModifyUserRequest* requestData, int& responseType, mysqlService::UpdateUserResponse* responseData);
	//处理users表添加个人基础信息的函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::InsertUserRequest* requestData, int& responseType, mysqlService::UpdateUserResponse* responseData);
	//处理users表删除个人基础信息的函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteUserRequest* requestData, int& responseType, mysqlService::UpdateUserResponse* responseData);
	//处理users表查询用户喜欢电影的函数
	void analyzeQueryResult(MysqlConn* conn, user::GetUserFavoriteRequest* requestData, int& responseType, user::GetUserFavoriteResponse* responseData);
	//处理users表添加用户喜欢电影的函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::InsertFavoriteRequest* requestData, int& responseType, mysqlService::UpdateFavoriteResponse* responseData);
	//处理users表删除用户喜欢电影的函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteFavoriteRequest* requestData, int& responseType, mysqlService::UpdateFavoriteResponse* responseData);
	//处理friends表获得朋友列表的回调函数
	void analyzeQueryResult(MysqlConn* conn, user::GetFriendsRequest* requestData, int& responseType, user::GetFriendsResponse* responseData);
	//处理friends表添加朋友的回调函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::InsertFriendRequest* requestData, int& responseType, mysqlService::UpdateFriendResponse* responseData);
	//处理friends表删除朋友的回调函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteFriendRequest* requestData, int& responseType, mysqlService::UpdateFriendResponse* responseData);
	//处理group表注册群组的回调函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::RegisterGroupRequest* requestData, int& responseType, mysqlService::RegisterGroupResponse* responseData);
	//处理group表删除群组的回调函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::DeleteGroupRequest* requestData, int& responseType, mysqlService::DeleteGroupResponse* responseData);
	//处理group表，获得某个组的一定条件的组员信息
	void analyzeQueryResult(MysqlConn* conn, user::GetGroupMemberRequest* requestData, int& responseType, user::GetGroupMemberResponse* responseData);
	//处理group表获得某用户群组列表的回调函数
	void analyzeQueryResult(MysqlConn* conn, user::GetUserGroupsRequest* requestData, int& responseType, user::GetUserGroupsResponse* responseData);
	//处理group表某群组添加成员的回调函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::AddUserToGroupRequest* requestData, int& responseType, mysqlService::AddUserToGroupResponse* responseData);
	//处理group表某群组删除成员的回调函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::DelUserFromGroupRequest* requestData, int& responseType, mysqlService::DelUserFromGroupResponse* responseData);
	//处理movies表，获取电影基本信息的回调函数
	void analyzeQueryResult(MysqlConn* conn, mysqlService::GetMovieInfoRequest* requestData, int& responseType, mysqlService::GetMovieInfoResponse* responseData);
public:
	TcpServer* m_server;
	ConnectPool* m_pool;
	std::map<int, std::map<int, std::string>> sqls;  //外层的key代表哪张表，内层的key对应的是指令id
};