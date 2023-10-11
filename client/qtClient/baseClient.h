#ifndef BASECLIENT_H
#define BASECLIENT_H

#include <QObject>
#include "TcpClient.h"

class Client;
class MainWindow;
class BaseClient : public QObject
{
    Q_OBJECT
public:
    explicit BaseClient(QObject *parent = nullptr);
    void startBaseClient();
    void setTcpConnect();  //不能在构造函数里连接，会出现空指针的问题
    //注册结果处理
    void handleRegisterResult(NetConnection*conn, void* userData);
    //登陆之后，会有4个返回结果都要处理
    void handleLoginResult(NetConnection*conn, void* userData);
    //发送消息到对端
    void sendMsg(int modid, int toId, const QString& msg);
    //发送消息结果处理
    void handleSendMsgResult(NetConnection*conn, void* userData);
    //接受消息
    void handleReceivedMsg(NetConnection*conn, void* userData);
    //注册群组
    void registerGroup(const QString& groupName, const QString& summary);  //这里需要是常量引用，否则
    //会编译出问题，和主要bug12一样的
    //添加群组或者用户
    void addRelations(int modid, int id);
signals:
    void switchMainWindow(MainWindow* mainWindow);
    void updateFavoriteList();
    void updateFriendList();
    void updateGroupList();
    void sendMsgResultNotify(int fromId, int result);    //发送消息成功，也不需要显示，如果人家发完就退出了，只需要追加消息即可，如果当前窗口或者点击即可显示
    //需要显示，否则会出现发送完需要退出重新进入，才能显示，第一个参数是来自哪个朋友的消息确认统治
    void recevidMsgNotify(int modid, int fromId);  //不用显示，只需要提示即可，当点击对应的人的名单时才需要将最新的消息显示出来
    //不对，在当前页面的话，需要显示，在其他页面需要提示
public:

public:
    TcpClient* tcpConn;
    bool connected;   //是否连接到服务器
    Client* client;
};

#endif // BASECLIENT_H
