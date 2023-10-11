#include "mysqlService.pb.h"
#include "MediaServer.h"

MediaServer::MediaServer(std::string mediaServerIp, unsigned short mediaServerPort, std::string lbServerIp, unsigned short lbServerPort, 
	std::string mysqlServerIp, unsigned short mysqlServerPort){
	//主要服务器初始化
	streamServer = new UdpServer(mediaServerIp.data(), mediaServerPort);
	mysqlClient = new TcpClient(mysqlServerIp.data(), mysqlServerPort);

	//处理lb服务器要求增加房间的函数
	streamServer->addMsgRouter(mediaService::ID_AddRoomRequest,
		std::bind(&MediaServer::createRoom, this, std::placeholders::_1, std::placeholders::_2));
	//处理mysql服务器返回fid相关信息的函数
	mysqlClient->addMsgRouter(mysqlService::ID_GetMovieInfoResponse, //tcp2
		std::bind(&MediaServer::createRoom2, this, std::placeholders::_1, std::placeholders::_2));
	//room.push_back(Room("/home/tony/myprojects/MovieHub/resource/喜剧之王.mp4"));
	mysqlClient->connectServer(); //这里有一个隐患就是服务器还不知道有没有连接成功MySQL
}


void MediaServer::run() {
	//开启一个线程启动mediaServer
	std::thread startMediaServer(&UdpServer::run, streamServer);
	//开启一个线程启动mysqlClient
	std::thread startMysqlClient(&TcpClient::run, mysqlClient);
	//然后再启动房间，每个房间内部会继续创建线程执行相应的逻辑，不需要启动房间，创建房间的时候就会启动了

	//主线程干啥，等待吧
	startMediaServer.join();
	startMysqlClient.join();
}

void MediaServer::createRoom(Udp* lbClient, void* userData) {
	mediaService::AddRoomRequest addRoomRequest;
	addRoomRequest.ParseFromArray(lbClient->m_request->m_data, lbClient->m_request->m_msglen);
	std::cout << "创建房间第一步，房间id为：" << addRoomRequest.fid() << std::endl;
	//给mysql发送消息，要求获得fid对应的基础信息
	mysqlService::GetMovieInfoRequest movieInfoRequest;
	movieInfoRequest.set_fid(addRoomRequest.fid());
	tcpSendMsg(mysqlClient, mysqlService::ID_GetMovieInfoRequest, &movieInfoRequest);
}

void MediaServer::createRoom2(NetConnection* mysqlClient, void* userData) { //必须为NetConnection不能为TcpCLient
	mysqlService::GetMovieInfoResponse movieInfo;
	movieInfo.ParseFromArray(mysqlClient->m_request->m_data, mysqlClient->m_request->m_msglen);
	std::cout << "创建房间第二步，房间id为：" << movieInfo.fid() << "的路径为" << movieInfo.path() << std::endl;
	std::string path = movieInfo.path() + "/" + movieInfo.name();  //注意movieInfo.path()这个仅仅是路径
	MediaRoom* mediaRoom = new MediaRoom(this, path);
	std::thread startRoom(&MediaRoom::run, mediaRoom);
	startRoom.join();
}
void MediaServer::destroyRoom() {

}