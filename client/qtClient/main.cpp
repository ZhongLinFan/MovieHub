#include <QApplication>
#include "client.h"
#include "welcome.h"
#include "mainwindow.h"
#include <QThread>
#include <QDebug>
#include "registerDialog.h"
#include "addRelationsDialog.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QThread* mediaThread = new QThread;
    QThread* baseThread = new QThread;
    Welcome* welcome = new Welcome();
    MainWindow* mainWindow = new MainWindow();
    Client* client = new Client(welcome, mainWindow);
    mainWindow->registerDialog = new RegisterDialog(client, mainWindow);
    mainWindow->addRelationsDialog =new AddRelationsDialog(client, mainWindow);
    //让下面两个页面记住client
    welcome->setClient(client);
    mainWindow->setClient(client);
    //设置mainWindow中关于client的信号，因为mainWindow需要设置好client才能正确添加信号，否则很大概率会出问题
    mainWindow->setClientConnect();
    //启动子线程执行mediaClient的run
    client->qMediaClient->moveToThread(mediaThread);
    QObject::connect(mediaThread, &QThread::started, client->qMediaClient, &MediaClient::startMediaClient);
    mediaThread->start();
    qDebug() << "开始移动startBaseClient";
    //启动子线程执行baseerver的run
    client->qBaseClient->moveToThread(baseThread);
    QObject::connect(baseThread, &QThread::started, client->qBaseClient, &BaseClient::startBaseClient);
    baseThread->start();
    //显示登陆窗口
    welcome->show();
    return a.exec();
}
