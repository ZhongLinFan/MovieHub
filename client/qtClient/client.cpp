#include "client.h"
#include <functional>
#include <thread>
#include <QDebug>



Client::Client(Welcome* welcome, MainWindow* mainWindow, QObject *parent): QObject(parent)
{
    m_user.isNewUser = false;  //应该在结构体内部初始化的，不过这里也没事
    m_mainWindow = mainWindow;
    m_welcome = welcome;
    //这两个要放在最后初始化，因为这里面要用到上面初始化好的参数
    qMediaClient = new MediaClient();
    qBaseClient = new BaseClient();
    qMediaClient->client = this;
    qBaseClient->client = this;
    qBaseClient->setTcpConnect();
    qMediaClient->setUdpConnect();
}





