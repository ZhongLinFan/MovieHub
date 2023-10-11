#include "lbService.pb.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "client.h"
#include "registerDialog.h"
#include "addRelationsDialog.h"
#include <QListWidget>
#include <QLabel>
#include <QSizePolicy>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //给个人主页添加三项当作标题
    QListWidgetItem* item =  new QListWidgetItem("我的喜欢", ui->userInfo, static_cast<int>(userInfoType::favoriteBar));
    item->setBackground(QColor(139,137,112));
    item = new QListWidgetItem("我的好友", ui->userInfo, static_cast<int>(userInfoType::friendBar));
    item->setBackground(QColor(139,137,112));
    item = new QListWidgetItem("我的群组", ui->userInfo, static_cast<int>(userInfoType::groupBar));
    item->setBackground(QColor(139,137,112));
    //相关信号
    //退出登陆
    connect(ui->logoutBtn, &QPushButton::clicked, this, &MainWindow::logout);
    //传递信号,为什么要传递信号呢，因为clicked没有参数，为什么不在这里直接发送呢，
    //因为考虑到这个功能应该是后台在做，为沙welcome不是呢，因为那个发送次数少，而且传递的话，参数太多了
    connect(ui->sendBtn, &QPushButton::clicked, this, [&](){
        qDebug() << "sendBtn clicked的模块个数" <<m_client->m_user.chatHistory.size() ;
        //为什么要大费苦心的传递modid和toId呢，client可以自己获取呀，
        //因为可能client执行sendMsg的时候，人家又发第二个了
        int modid = m_client->m_user.state.chatModid;
        int toId = m_client->m_user.state.chatObj;
        emit sendMsg(modid, toId, ui->sendBuf->toPlainText());  //这个地方有个编译bug，见bug12
        //由于群聊没有消息发送成功确认机制，而是直接默认发送成功，所以这里直接加上
        //群聊也有确认机制。。。。。只不过base服务器是直接返回成功的结果，所以下面就是多此一举，会导致发送端每发出一个消息都会多一个换行
        //if(m_client->m_user.state.chatModid == 2){
            //updateWidgetAfterSend();  //不管之前的统统需要清空
        //}
    });
    connect(ui->userInfo, &QListWidget::itemClicked, this, [&](QListWidgetItem* item){
        userInfoType itemType = static_cast<userInfoType>(item->type());
        if(itemType == userInfoType::favoriteItem){
            //发送一个请求当前fid的请求
            qDebug() << "获取流媒体服务器操作";
            lbService::GetServerRequest requestData;
            requestData.set_modid(2);  //1代表请求基础ip，2代表请求流媒体ip
            requestData.set_uid(m_client->m_user.uid);  //客户uid
            requestData.set_fid(item->toolTip().toInt());
            m_client->udpSendMsg((int)lbService::ID_GetServerRequest,
             &m_client->qMediaClient->udpConn->m_recvAddr, &requestData);
        }else if(itemType == userInfoType::friendItem || itemType == userInfoType::groupItem){
            ui->sendBuf->clear();  //不管之前的统统需要清空
            //不管上次有没有发出去成功，都把sendBuf设置为原来的颜色
            ui->sendBuf->setStyleSheet("background-color: rgb(255,255,255);");

            //判断上次的页面是不是这次的页面，如果是，不需要清空chatHistoy,只需要更新
            int lastChatObj = m_client->m_user.state.chatObj;
            int lastChatModid = m_client->m_user.state.chatModid;
            int curChatObj = item->toolTip().toInt();
            int curChatModid = static_cast<int>(itemType)-static_cast<int>(userInfoType::friendItem)+1;
            qDebug() << "lastChatObj"  << lastChatObj << "lastChatModid" << lastChatModid << "\t" << (!lastChatObj && !lastChatModid);
            //如果不是初始状态，才会清理。初始状态，直接跳过
            if(lastChatObj && lastChatModid){  //都不为0，才认为不是初始状态（有一个是0就认为是初始状态）
                auto& lastTargetRecords = m_client->m_user.chatHistory[lastChatModid][lastChatObj];
                //和上次聊天对象不是同一个人或者群组
                qDebug() << "当前widget的item:" << ui->chatHistory->count();
                if(curChatObj != lastChatObj || curChatModid != lastChatModid){
                    int itemNums = ui->chatHistory->count();
                    for(int i = 0; i < itemNums; i++){
                        qDebug() << "i=" << i;
                        //delete ui->chatHistory->item(i); //不对，不能直接删除
                        /*
                            使用takeItem(row)函数将QListWidget中的第row个item移除，
                            移除需要使用delete手动释放其在堆上占用的空间
                            https://www.cnblogs.com/LifeoFHanLiu/p/9940520.html
                            https://blog.csdn.net/weixin_43519792/article/details/103552551
                       */
                       QListWidgetItem* zeroItem = ui->chatHistory->takeItem(0); //这里是0，不是i，因为每移除一个item都会导致每个item的row发生变化
                       QWidget* window = ui->chatHistory->itemWidget(zeroItem);
                       delete window;
                       delete zeroItem;
                    }
                    lastTargetRecords.curDisplay = 0;
                    qDebug() << "删除完之后widget的item:" << ui->chatHistory->count();
                }
            } 
            ui->rightWidget->setCurrentIndex(1);
            //展示当前聊天对象的聊天信息
            updateChatWidegt(curChatModid, curChatObj);
            m_client->m_user.state.chatObj = item->toolTip().toInt();
            m_client->m_user.state.chatModid = static_cast<int>(itemType)-static_cast<int>(userInfoType::friendItem)+1;
            ui->chatName->setText(item->text());
        }else{
            bool setIsHidden = !m_client->m_user.isHidden[itemType];
            m_client->m_user.isHidden[itemType] = setIsHidden;
            int max = ui->userInfo->count();
            int i = ui->userInfo->currentRow()+1;
            qDebug() << "当前行" << i;
            while(i < max && ui->userInfo->item(i)->type() != static_cast<int>(userInfoType::favoriteBar) &&
                              ui->userInfo->item(i)->type() != static_cast<int>(userInfoType::friendBar) &&
                              ui->userInfo->item(i)->type() != static_cast<int>(userInfoType::groupBar)
                  ){
                    ui->userInfo->item(i)->setHidden(setIsHidden);
                    i++;
            }
        }
    });
    connect(ui->returnBtn, &ReturnBtn::clicked,this,[&](){
        ui->rightWidget->setCurrentIndex(0);
    });
}

void MainWindow::setClientConnect(){
    //发送消息信号  //不能上面的构造函数注册，因为可能m_client还没构造出来
    connect(this, &MainWindow::sendMsg, m_client->qBaseClient, &BaseClient::sendMsg, Qt::DirectConnection);
    //registerDialog还没创建出来，如果在构造函数执行下面两个信号连接的话
    //注册群组信号
    connect(ui->registerGroupBtn, &QPushButton::clicked, this, [&](){
        registerDialog->show();
    });

    //添加用户或者群组信号
    connect(ui->addRelationsBtn, &QPushButton::clicked, this, [&](){
        addRelationsDialog->show();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateFavoriteList(){
    qDebug()<< "正在执行updateFavoriteList";
    auto targetList = &m_client->m_user.favoriteList;
    qDebug() << "m_client->m_user.favoriteList.size()" << targetList->size();
    for(int i = 0; i < targetList->size(); i++){
        QListWidgetItem* item = new QListWidgetItem((*targetList)[i].name, nullptr, static_cast<int>(userInfoType::favoriteItem));
        item->setToolTip(QString::number((*targetList)[i].id));
        ui->userInfo->insertItem(1+i, item);
    }
}
void MainWindow::updateFriendList(){
    auto targetList = &m_client->m_user.friendList;
    qDebug() << "m_client->m_user.friendList.size()" << targetList->size();
    for(int i = 0; i < targetList->size(); i++){
        QListWidgetItem* item = new QListWidgetItem((*targetList)[i].name, nullptr, static_cast<int>(userInfoType::friendItem));
        item->setToolTip(QString::number((*targetList)[i].id));
        ui->userInfo->insertItem(2+m_client->m_user.favoriteList.size()+i, item);
    }

}
void MainWindow::updateGroupList(){
    auto targetList = &m_client->m_user.groupList;
    qDebug() << "m_client->m_user.groupList.size()" << targetList->size();
    for(int i = 0; i < targetList->size(); i++){
        QListWidgetItem* item = new QListWidgetItem((*targetList)[i].name, nullptr, static_cast<int>(userInfoType::groupItem));
        item->setToolTip(QString::number((*targetList)[i].id));
        ui->userInfo->insertItem(3+m_client->m_user.favoriteList.size()+m_client->m_user.friendList.size()+i, item);
    }
}

void MainWindow::logout(){
    qDebug();
    baseService::LogoutRequest logoutData;
    logoutData.set_uid(m_client->m_user.uid);
    m_client->tcpSendMsg((int)baseService::ID_LogoutRequest, m_client->qBaseClient->tcpConn, &logoutData);
    exit(0);
}

void MainWindow::showBaseInfo(){
    ui->baseInfo->insertPlainText("uid:");
    ui->baseInfo->insertPlainText(QString::number(m_client->m_user.uid));
}

void MainWindow::handleSendMsgResult(int fromId, int result){
    //这里需要在显示窗口追加一行这个聊天记录，不过现在可以不做，但是需要保证发送成功，否则更新就是失败的
    //清除缓冲
    //不管当前页有没有在聊天窗口，都不需要将聊天记录更新到窗口，这个功能应该是点击相应的人的名字才去更新index之后的值的
    //上述描述不对，如果在当前窗口需要更新，包括接受消息的处理，如果不更新，会出现发送完，返回然后再进去才会出现消息
    //点击更新是点击更新的，这个相当于加里个自动更新的逻辑
    if(result == 1){
        updateWidgetAfterSend(fromId);
    }
    if(result == -1){
         //背景板颜色可以通过setStyleSheet或者palette进行
         ui->sendBuf->setStyleSheet("background-color: rgb(178,34,34);");
    }
}

void MainWindow::updateWidgetAfterSend(int fromId){
    QString msg = ui->sendBuf->toPlainText();
    ui->sendBuf->clear();
    //将当前消息追加到聊天窗口记录中，不应该在这里做
    //ui->chatHistory->addItem(msg);
    //将当前消息记录到缓存中
    int modid = m_client->m_user.state.chatModid;
    int toId =  m_client->m_user.state.chatObj;
    chatRecords record;
    record.data = msg;
    record.fromId = m_client->m_user.uid;
    qDebug() << "updateWidgetAfterSend当前要记录的消息" << msg;
    m_client->m_user.chatHistory[modid][toId].records.append(record);
    qDebug() << "updateWidgetAfterSend的模块个数" <<m_client->m_user.chatHistory.size() ;
    QMapIterator<int, QMap<int, dialogState>> i(m_client->m_user.chatHistory);
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
    //这里其实还应该加上modid相同，但是先不加了，后面再加
    //但是我现在只有私聊消息才会有确认机制，群聊直接默认发送成功，所以这里可以直接判断是否为1，来保证群聊和私聊id相同也不至于出错
    //上面说法不对，群聊也有确认机制，只不过是直接base确认的，所以返回的结果必须要有modid，不过这需要很快速的切换才可以，所以可以暂时不加东西
    if(m_client->m_user.state.chatObj == fromId){
        //如果是在当前窗口直接显示所有的消息
        updateChatWidegt(modid, toId);
    }
}

void MainWindow::handleReceivedMsg(int modid, int fromId){
    //这里只需要提示用户消息来了，其他的什么都不需要做
    if(m_client->m_user.state.chatObj == fromId && m_client->m_user.state.chatModid == modid){
         updateChatWidegt(modid, fromId);
    }
}

void MainWindow::updateChatWidegt(int modid, int toId){
    if(ui->rightWidget->currentIndex() == 1){

        auto& curTargetRecords = m_client->m_user.chatHistory[modid][toId];
        for(int i = curTargetRecords.curDisplay; i < curTargetRecords.records.size(); i++){
            //这里的设计思路看bug19那块的，都记在那里了
            //创建一个视图模型
            QWidget* window = new QWidget;
            //创建一个布局，并将消息设置到布局上
            QHBoxLayout* layout = new QHBoxLayout(window);  //父对象
            //创建一个头像label
            QLabel* headLabel = new QLabel(window);//父对象
            //设置头像的伸展策略
            headLabel->setFixedSize(20,20);         //设置头像尺寸
            headLabel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed, QSizePolicy::Label));
             //创建一个文本消息label
            QLabel* msgLabel = new QLabel(curTargetRecords.records[i].data, window); //父对象
            window->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));

            //int width = ui->chatHistory->width() - 50;
            //msgLabel->setFixedSize(width, 20);
            msgLabel->setWordWrap(true);  //设置文本自动换行，否则不管多少字一直是一行，再配合上面的侵略策略即可实现超常文本的展示
            //因为不能设置sizeHine，而且我又不想文字比较短的时候文本框那么长，就采取了这种折中的方案
            //，我量了一下，一个字的宽度为20，而且我还设置了固定尺寸的聊天框，所以不会错的
            double expectWidth = 0;  //20为一个字的宽度
            QString data = curTargetRecords.records[i].data;
            for(int i = 0; i < data.size(); i++){
                ushort uni = data[i].unicode();
                if(uni >= 0x4E00 && uni <= 0x9FA5)
                {
                    expectWidth += 15.5;
                }else{
                    expectWidth += 8;
                }
            }
            int maxWidth = ui->chatHistory->width() - 50;  //45是实际测的，刚好的宽度
            if(expectWidth < maxWidth){
                msgLabel->setMinimumWidth(expectWidth);  //设置最佳尺寸
            }else{
                msgLabel->setFixedWidth(maxWidth);
            }
            msgLabel->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum, QSizePolicy::Label));
            if(curTargetRecords.records[i].fromId == m_client->m_user.uid){
                msgLabel->setStyleSheet("background-color: rgb(26,173,25);");
                headLabel->setPixmap(m_client->m_user.iconPath);
                layout->addWidget(msgLabel, 1, Qt::AlignRight);   //一定要把拉神因子设为大于0的呀，否则根本不会侵略其他的空间。。。。
                msgLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                layout->addWidget(headLabel, 0, Qt::AlignRight);
            }else{
               friend_ from(curTargetRecords.records[i].fromId);
               int targetFriendIndex = m_client->m_user.friendList.indexOf(from);
               headLabel->setPixmap(m_client->m_user.friendList[targetFriendIndex].iconPath);
               layout->addWidget(headLabel, 0, Qt::AlignLeft);
               msgLabel->setStyleSheet("background-color: rgb(241,241,241);");
               msgLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
               layout->addWidget(msgLabel, 1, Qt::AlignLeft);
            }
            //设置视图模型的布局
            window->setLayout(layout);
            //创建一个项，将当前项和布局进行关联
            QListWidgetItem* item = new QListWidgetItem();
            item->setSizeHint(layout->sizeHint());
            ui->chatHistory->addItem(item);  //将当前项添加到widget中
            ui->chatHistory->setItemWidget(item, window);  //将当前项添加到widget中
        }
        curTargetRecords.curDisplay = curTargetRecords.records.size();
    }
}


