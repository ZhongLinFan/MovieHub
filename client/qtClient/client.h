#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QDebug>
#include "welcome.h"
#include "mainwindow.h"
#include "userInfo.h"
#include "mediaClient.h"
#include "baseClient.h"

//要使用qt中的多线程，所以需要这个类继承QObject
class Client : public QObject
{
    Q_OBJECT
public:
    Client(Welcome* welcome, MainWindow* mainWindow, QObject *parent = nullptr);
signals:

    //下面的公共函数都是其他模块直接复制过来的，所以没有解释
    //至于为啥要在头文件定义，看主要bug记录
public:
    // 公共解析函数
    template<class Conn_T, class Request_T>
    std::shared_ptr<Request_T> parseRequest(Conn_T* conn){
        Debug("开始解析数据");
        std::shared_ptr<Request_T> requestData (new Request_T);
        requestData->ParseFromArray(conn->m_request->m_data, conn->m_request->m_msglen);
        return requestData;  //防止多次深拷贝
    }

    //公共打包函数
    template<class Conn_T, class Response_T>
    bool packageMsg(int responseID, std::string& responseSerial, Conn_T* conn, Response_T* responseData){
        Debug("requestHandle开始回复数据");
        //responseData的结果序列化
        responseData->SerializeToString(&responseSerial);
        //封装response
        auto response = conn->m_response;
        response->m_msgid = responseID;
        response->m_data = &responseSerial[0];
        response->m_msglen = responseSerial.size();
        response->m_state = HandleState::Done;
        return true;
    }
    //udp发送函数
    template<class Response_T>
    void udpSendMsg(int responseID, sockaddr_in* to, Response_T* responseData){
        qDebug() <<"正在发送数据"<< responseData->uid() ;
        std::string responseSerial;
        if (packageMsg<Udp, Response_T>(responseID, responseSerial, qMediaClient->udpConn, responseData)) {
            qMediaClient->udpConn->sendMsg(*to);
        }
        else {
            std::cout << "msgId为-1，已拒绝发送该包" << std::endl;
        }
    }
    //tcp发送函数
    template<class Response_T>
    void tcpSendMsg(int responseID, NetConnection* conn, Response_T* responseData){
        std::string responseSerial;
        if (packageMsg<NetConnection, Response_T>(responseID, responseSerial, conn, responseData)) {
            //Debug("tcpSendMsg已经打包数据完成，准备执行发送函数");
            conn->sendMsg();
        }
        else {
            std::cout << "msgId为-1，已拒绝发送该包" << std::endl;
        }

    }
public:
    MediaClient* qMediaClient;
    BaseClient* qBaseClient;
    userInfo m_user;
    MainWindow* m_mainWindow;
    Welcome* m_welcome;
};

#endif // CLIENT_H
