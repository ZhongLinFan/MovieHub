#include "baseService.pb.h"
#include "baseClient.h"
#include "client.h"

BaseClient::BaseClient(QObject *parent) : QObject(parent)
{
    connected = false;
    tcpConn = new TcpClient();
    //注册消息发送成功的函数，这里需要更新列表，如果发送成功才能把消息更新到对话框
    auto sendMsgResultFunc = std::bind(&BaseClient::handleSendMsgResult, this,
       tcpConn, nullptr);

    tcpConn->addMsgRouter((int)baseService::ID_SendMsgResponse, sendMsgResultFunc);
    //注册接受消息的函数
    auto receivedMsgFunc = std::bind(&BaseClient::handleReceivedMsg, this,
       tcpConn, nullptr);
    tcpConn->addMsgRouter((int)baseService::ID_MsgNotify, receivedMsgFunc);

    //注册登陆之后的处理函数，如果登陆成功，应该切换到主窗口，登陆失败就不用管
    auto loginResultFunc = std::bind(&BaseClient::handleLoginResult, this,
       tcpConn, nullptr);
    tcpConn->addMsgRouter((int)baseService::ID_LoginResponse, loginResultFunc);
    //注册注册之后的处理函数，注册成功，应该切换到主窗口，注册失败不用管
    //登陆成功之前没有返回result，而是直接请求各种列表
     auto registerResultFunc = std::bind(&BaseClient::handleRegisterResult, this,
        tcpConn, nullptr);
    tcpConn->addMsgRouter((int)baseService::ID_RegisterResponse, registerResultFunc);
}

void BaseClient::setTcpConnect(){
     connect(this, &BaseClient::switchMainWindow, client->m_welcome, &Welcome::switchMainWindow);
     //连接mainWindow的信号和槽函数
     connect(this, &BaseClient::updateFavoriteList, client->m_mainWindow, &MainWindow::updateFavoriteList);
     connect(this, &BaseClient::updateFriendList, client->m_mainWindow, &MainWindow::updateFriendList);
     connect(this, &BaseClient::updateGroupList, client->m_mainWindow, &MainWindow::updateGroupList);
     //发送接受消息窗口更新
     connect(this, &BaseClient::sendMsgResultNotify, client->m_mainWindow, &MainWindow::handleSendMsgResult);
     connect(this, &BaseClient::recevidMsgNotify, client->m_mainWindow, &MainWindow::handleReceivedMsg);
 }

void BaseClient::startBaseClient(){
    tcpConn->run();
}

void BaseClient::handleRegisterResult(NetConnection*conn, void* userData){
    auto registerResultData = client->parseRequest<NetConnection, baseService::RegisterResponse>(conn);
    if(registerResultData->modid() == 1){  //注册用户结果
        if(registerResultData->result() == 1){  //注册成功
            //在这里切换页面
            emit switchMainWindow(client->m_mainWindow);
            client->m_user.uid =registerResultData->id();
        }else{
            //注册失败，什么都不用管
        }
    }else if(registerResultData->modid() == 2){  //注册群组

    }else{  //未知注册id

    }
}


void BaseClient::handleLoginResult(NetConnection*conn, void* userData){
   auto loginResultData = client->parseRequest<NetConnection, baseService::LoginResponse>(conn);
    std::cout << "当前用户：" << loginResultData->uid() << std::endl;
    if (loginResultData->has_result()) {
        if(loginResultData->result() == 1){
            //登陆成功，在这里需要切换为主窗口，或者释放登陆界面
            std::cout << "登录结果：" << loginResultData->result() << std::endl;
            //应该在主线程切换页面，所以发个信号
            emit switchMainWindow(client->m_mainWindow);
            //记录当前用户信息
            client->m_user.uid =loginResultData->uid();
        }else{
            //给登陆界面发提示，登陆失败
        }
    }
    else {
        if (loginResultData->has_favoritelist()) {
            //为什么这里要用list而不用红黑树呢，之前看过由于vector是连续的，所以在1000以下，比map快，
            //这里list就是连续的
            //这里只比较关键数据，如果关键数据相等，就认为相等（可以定时的更新所有数据）
            QList<favorite> favoriteTemp;
            for (int i = 0; i < loginResultData->favoritelist().file_size(); i++) {
                favorite temp;
                temp.id = loginResultData->favoritelist().file(i).fid();
                temp.name = QString::fromStdString(loginResultData->favoritelist().file(i).name());
                favoriteTemp.append(temp);
            }
            if(favoriteTemp !=  client->m_user.favoriteList){
                emit updateFavoriteList();
                client->m_user.favoriteList.swap(favoriteTemp);
            }
        }
        else if (loginResultData->has_friendlist()) {
            QList<friend_> friendTemp;
            for (int i = 0; i < loginResultData->friendlist().friend__size(); i++) {
                friend_ temp;
                temp.id = loginResultData->friendlist().friend_(i).uid();
                temp.name = QString::fromStdString(loginResultData->friendlist().friend_(i).name());
                friendTemp.append(temp);
            }
            if(friendTemp != client->m_user.friendList){
                emit updateFriendList();
                client->m_user.friendList.swap(friendTemp);
            }
        }
        else if (loginResultData->has_grouplist()) {
            QList<group> groupTemp;
            for (int i = 0; i < loginResultData->grouplist().group_size(); i++) {
                group temp;
                temp.id = loginResultData->grouplist().group(i).gid();
                temp.name = QString::fromStdString(loginResultData->grouplist().group(i).name());
                groupTemp.append(temp);
            }
            if(groupTemp != client->m_user.groupList){
                emit updateGroupList();
                client->m_user.groupList.swap(groupTemp);
            }
        }
        else {
            std::cout << "未知列表" << std::endl;
        }
    }
}

//发送消息到对端
void BaseClient::sendMsg(int modid, int toId, const QString& msg){
    qDebug() << "正在发送消息到对端，发送模式：" << modid << "发送目的地址" << toId;
    baseService::SendMsgRequest msgRequest;
    msgRequest.set_modid(modid);
    msgRequest.set_msg(msg.toStdString());  //这里要注意转换
    msgRequest.set_toid(toId);
    client->tcpSendMsg(baseService::ID_SendMsgRequest, tcpConn, &msgRequest);  //这里竟然也可以自己转换
    //发送完不能追加消息，需要等到消息发送成功，才能追加消息
}


//发送消息成功的处理
void BaseClient::handleSendMsgResult(NetConnection*conn, void* userData){
    qDebug() << "handleSendMsgResult正在下响应";
    //这个响应并不合理，需要改进，假如突然发很多消息，都不知道确认的是哪个了，不过现在先这样用着
    //加入有一条发送成功，一条发送失败，他应该怎么更新，大概率会出错
    auto receivedData = client->parseRequest<NetConnection, baseService::SendMsgResponse>(conn);
    //发送信号
    emit sendMsgResultNotify(receivedData->fromid(), receivedData->result());
}

//接受消息
void BaseClient::handleReceivedMsg(NetConnection*conn, void* userData){
    auto receivedData = client->parseRequest<NetConnection, baseService::MsgNotify>(conn);
    //取出数据
    int modid = receivedData->modid();
    int toId = receivedData->toid();  //目的gid或者是这个消息的目的uid
    int fromId = receivedData->fromid();  //来自谁发的（哪个uid）
    //先将消息压入缓存中
    chatRecords record;
    record.data = QString::fromStdString(receivedData->msg());
    record.fromId = fromId;
    qDebug() << "handleReceivedMsg当前要记录的消息" <<  record.data;
    if(modid == 1){
        client->m_user.chatHistory[modid][fromId].records.append(record);  //注意这里是toid
        //发送信号，
        emit recevidMsgNotify(modid, fromId);
    }else if(modid == 2 && fromId != client->m_user.uid){  //如果群组是自己发的消息，不处理
        client->m_user.chatHistory[modid][toId].records.append(record);  //注意这里是toid
        //发送信号，
        emit recevidMsgNotify(modid, toId);

    }else{
        std::cout << "未知消息，已忽略" << std::endl;
        return;
    }
    QMapIterator<int, QMap<int, dialogState>> i(client->m_user.chatHistory);
      while (i.hasNext()) {
          i.next();
          qDebug() << "当前消息模块：" << i.key();
          QMapIterator<int, dialogState> j(i.value());
          while (j.hasNext()) {
              j.next();
              qDebug() << "\t当前消息来源：" << j.key();
              for(int k = 0; k < j.value().records.size(); k++){
                      qDebug() << "\t\t第" << k << "条消息：" <<  j.value().records[k].data;
              }
          }
      }

}

//注册群组
void BaseClient::registerGroup(const QString& groupName, const QString& summary){
    qDebug() << "正在注册群组";
    baseService::RegisterRequest registerData;
    registerData.set_id(client->m_user.uid);
    registerData.set_modid(2);  //群聊
    registerData.set_name(groupName.toStdString());
    registerData.set_summary(summary.toStdString());
    std::cout << client->m_user.uid << groupName.toStdString() << summary.toStdString() << std::endl;
    client->tcpSendMsg(baseService::ID_RegisterRequest, tcpConn, &registerData);  //这里竟然也可以自己转int
}

//添加群组或者用户
void BaseClient::addRelations(int modid, int id){
     qDebug() << "正在添加群组";
    baseService::AddRelationsRequest addData;
    addData.set_modid(modid);  //添加组
    if (id <= 0) {
        std::cout << "id输入错误，已退出" << std::endl;
        return;
    }
    addData.set_id(id);
    client->tcpSendMsg(baseService::ID_AddRelationsRequest, tcpConn, &addData);  //这里竟然也可以自己转int
}


