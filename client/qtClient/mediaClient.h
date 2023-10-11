#ifndef MEDIACLIENT_H
#define MEDIACLIENT_H


#include <QObject>
#include "Udp.h"
#include <QDebug>
class Client;
class MediaClient : public QObject
{
    Q_OBJECT
public:
    explicit MediaClient(QObject *parent = nullptr);
    void setUdpConnect();  //不能在构造函数中注册
    //获取ip之后的操作
    void handleObtainedServer(Udp*conn, void* userData);
    void startMediaClient();
signals:
    void login();
    void registerUser();
public:
    Udp* udpConn;
    Client* client;
};

#endif // MEDIACLIENT_H
