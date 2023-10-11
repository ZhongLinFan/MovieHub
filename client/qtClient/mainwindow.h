#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "baseService.pb.h"
#include <QMainWindow>

class Client;
class AddRelationsDialog;
class RegisterDialog;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    //设置client指针
    inline void setClient(Client* client){
        m_client = client; //相关信号
    }
    //设置mainWindow中关于client的信号
    void setClientConnect();
    //更新主页列表
    void updateFavoriteList();
    void updateFriendList();
    void updateGroupList();
    //退出登陆
    void logout();
    //更新个人信息
    void showBaseInfo();
    //处理发送消息结果
    void handleSendMsgResult(int fromId, int result);
    void updateWidgetAfterSend(int fromId);
    void updateChatWidegt(int modid, int toId);
    //处理接受的消息
    void handleReceivedMsg(int modid, int fromId);
signals:
    //发送群消息
    void sendMsg(int modid, int toId, const QString& msg);
public:
    Ui::MainWindow *ui;
    Client* m_client;
    RegisterDialog* registerDialog;
    AddRelationsDialog* addRelationsDialog;
};
#endif // MAINWINDOW_H
