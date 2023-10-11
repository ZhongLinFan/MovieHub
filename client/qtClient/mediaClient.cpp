#include "lbService.pb.h"
#include "mediaClient.h"
#include "client.h"

MediaClient::MediaClient(QObject *parent) : QObject(parent)
{
    udpConn = new UdpClient("127.0.0.1", 10001);//lb服务器的地址
    //注册相关的函数，如果客户端输入登陆按钮，media立马发给udp一个server请求，
    //如果返回一个server，那么把刚才输入的信息发给tcp，如果登陆成功，应该切换到mainWindow，
    //如果登陆失败，应该保持现状
    //注册收到lb服务器返回的基础服务器ip之后的操作
    //这里有一个巨大的bug，在主要bug8那里记录了，这里就不记录了(注意这个bug还是mediaClient是Udp的类型的时候，后面改了)
    auto obtainedServerFunc = std::bind(&MediaClient::handleObtainedServer, this,
        udpConn, nullptr);
    udpConn->addMsgRouter((int)lbService::ID_GetServerResponse, obtainedServerFunc);

}
void MediaClient::setUdpConnect(){
    //连接welcome的信号和槽函数
    connect(this, &MediaClient::login, client->m_welcome, &Welcome::login);
    connect(this, &MediaClient::registerUser, client->m_welcome, &Welcome::registerUser);
}

void MediaClient::startMediaClient(){
    udpConn->run();
}

void MediaClient::handleObtainedServer(Udp* conn, void* userData){
    auto obtainedIpData = client->parseRequest<Udp, lbService::GetServerResponse>(conn);
    if(obtainedIpData->modid() == 1){  //如果是收到的是基础服务器模块，那就代表还没登陆或注册
        //修改baseClient的ip
        auto tcpConn = client->qBaseClient->tcpConn;
        tcpConn->changeServer(obtainedIpData->host(0).ip().data(), obtainedIpData->host(0).port());
        tcpConn->connectServer();
        //连接成功之后发送登陆或者注册请求
        while(!tcpConn->m_connected);
        client->qBaseClient->connected = true;
        //为什么要把登陆和注册操作放在welcome里，因为可以很方便的获取那个页面的值，这里调用一下接口就行
        //不能调用接口，因为多线程的原因
        if(client->m_user.isNewUser){  //如果是新用户就注册，也就是如果点击了切换注册按钮，那么就设为新用户
            qDebug() << "登陆页面正在进行注册操作";
            emit registerUser();

        }else{  //登陆操作
            qDebug() << "登陆页面进行登陆操作";
            emit login();
        }
    }else if(obtainedIpData->modid() == 2){
        //流媒体服务器的地址
        qDebug() << "当前是流媒体地址"<< "当前的地址目的uid：" << obtainedIpData->uid();;
        qDebug() << "获得的地址详情";
        for(int i = 0; i < obtainedIpData->host_size(); i++){
            qDebug() << QString::fromStdString(obtainedIpData->host(i).ip()) << obtainedIpData->host(i).port();
        }
    }else{
        qDebug() << "未知服务器地址" ;
    }
}

