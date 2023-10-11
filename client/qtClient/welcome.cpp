#include "lbService.pb.h"
#include "baseService.pb.h"
#include "welcome.h"
#include "ui_welcome.h"
#include <QDebug>
#include "client.h"
#include "mainwindow.h"

Welcome::Welcome(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Welcome)
{
    ui->setupUi(this);
    connect(ui->loginAndRegisterBtn, &QPushButton::clicked, this, &Welcome::getServer);
    connect(ui->chooseRegisterBtn, &QPushButton::clicked, this, &Welcome::chooseRegister);
}

Welcome::~Welcome()
{
    delete ui;
}


//登陆操作
void Welcome::login()
{
     qDebug() << "登陆操作";
     qDebug() << "uid:"<< ui->uid->text().toInt() << "密码:" << ui->passwd->text();
     baseService::LoginRequest loginRequeset;
     if(ui->uid->text().toInt() <= 0){
         return;
     }
     loginRequeset.set_uid(ui->uid->text().toInt());
     loginRequeset.set_passwd(ui->passwd->text().toStdString());
     m_client->tcpSendMsg((int)baseService::ID_LoginRequest, m_client->qBaseClient->tcpConn, &loginRequeset);
}

//选择注册操作
void Welcome::chooseRegister()
{
    qDebug() << "切换为注册操作";
    ui->loginAndRegisterBtn->setText("注册");
    ui->uidLabel->setText("昵称：");
    m_client->m_user.isNewUser = true;
}

//注册操作
void Welcome::registerUser()
{
    qDebug() << "注册操作";
    baseService::RegisterRequest registerData;
    registerData.set_name(ui->uid->text().toStdString());
    registerData.set_modid(1);  //注册用户
    registerData.set_passwd(ui->passwd->text().toStdString());
    registerData.set_summary("可爱的夏天");
    m_client->tcpSendMsg((int)baseService::ID_RegisterRequest, m_client->qBaseClient->tcpConn, &registerData);
}

//获取server操作
void Welcome::getServer(){
    qDebug() << "获取基础服务器操作";
    if(ui->uid->text().size() == 0 || ui->passwd->text().size() == 0 || m_client->qBaseClient->connected){
        return;
    }
    lbService::GetServerRequest responseData;
    responseData.set_modid(1);  //1代表请求基础ip，2代表请求流媒体ip
    responseData.set_uid(ui->uid->text().toInt());  //客户uid
    m_client->udpSendMsg((int)lbService::ID_GetServerRequest,
     &m_client->qMediaClient->udpConn->m_recvAddr, &responseData);
}

//切换主页面
//这里不能用QMainWindow,需要使用MainWindow，否则showBaseInfo会报错，需要使用子类指针
void Welcome::switchMainWindow(MainWindow* mainWindow){
    mainWindow->show();
    mainWindow->showBaseInfo();
    this->close();
}

