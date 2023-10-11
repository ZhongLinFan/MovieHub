#ifndef USERINFO_H
#define USERINFO_H
#include <QString>
#include <QList>
#include <QMap>
#include <QDebug>
#include <QRandomGenerator>



enum class userInfoType:int{
    favoriteItem = 1001,
    friendItem = 1002,
    groupItem = 1003,
    favoriteBar = 1004,
    friendBar = 1005,
    groupBar = 1006,
};

#endif // USERINFO_H
struct chatRecords{
    int fromId;   //消息来自哪个用户的
    QString data;
};

//为什么不和上面和一起呢，因为聊天记录和这些信息未来属性大概率会不一样，所以拆分出来
struct favorite{
    int id;
    QString name;
    int size;   //文件大小，暂时不用，主要是为了突出这个结构体的特点
    //QList需要进行比较，这里需要重载==
    bool operator==(favorite& obj){
        return id == obj.id && name == obj.name;
    }
};
struct friend_{
    int id;
    QString name;
    QString iconPath;
    int age;   //年龄大小，暂时不用，主要是为了突出这个结构体的特点
    bool operator==(const friend_& obj){  //需要加const
        return id == obj.id;  //这里不判断名字主要是为了查找list方便
    }
    friend_(){
        QRandomGenerator* intGenerator = QRandomGenerator::global();  //静态方法，全局唯一的实力，但是不能定义在类外面，否则重复定义
        iconPath= ":/img/head" + QString::number(intGenerator->bounded(1,7)); //1-6的随即数字
    }
    friend_(int uid):friend_(){
        id = uid;
    }
};

struct group{
    int id;
    QString name;
    int memberNums;   //年龄大小，暂时不用，主要是为了突出这个结构体的特点
    bool operator==(group& obj){
        return id == obj.id && name == obj.name;
    }
};

//记录当前状态
struct curState{
    int watchObj;  //fid
    int chatObj; //chatId
    int chatModid;  //群聊，私聊，直播
    curState(){
        watchObj = 0;
        chatObj = 0;
        chatModid = 0;
    }
};

//一个朋友或者一个组的聊天状态，主要是为了记录当前对话显示到哪里了，以便追加
struct  dialogState{
    int curDisplay;  //当前聊天框显示到哪了
    QList<chatRecords> records;
};

struct userInfo{
public:
    bool isNewUser;   //是否为新用户
    int uid;
    QString name;
    QString iconPath;
    //朋友列表也要生成一个头像，记录在friendList里面
    //QLabel* headLabel; //当前登陆的头像  ,不能将所有的聊天头像对应这一个实力，如果你这样尝试，最终只会有一条消息有头像
    //历史消息缓存（包括各组，各朋友列表，当然只缓存固定条数的数据）
    QMap<int, QMap<int, dialogState>>chatHistory;  //第一个key代表是私还是群

#if 0
    QMap<int, QList<chatRecords>> groupRecords;  //key为组id
    QMap<int, QList<chatRecords>> friendRecords;  //必须要为两层，如果只有QList，
    //那么取一个朋友的聊天记录要便利一遍，而且即使是一对一也要分清是谁发的消息
    //那干脆再套一层map
#endif

    //保存最新的列表，更新只从这里更新
    //为什么要有下面三个缓存，主要是为了不让每次来新数据之后，都在屏幕上更新一下，太浪费时间
    QList<favorite> favoriteList;
    QList<friend_> friendList;
    QList<group> groupList;
    QMap<userInfoType, bool> isHidden;  //当前组是否被隐藏，索引为对应的枚举
    curState state;
    //bool isOld;  //上面三个数据是否过时，如果过时，需要更新,可以在函数体进行判断
public:
    userInfo(){
        uid = 0;
        name.clear();
        isHidden[userInfoType::favoriteBar] = false;
        isHidden[userInfoType::friendItem] = false;
        isHidden[userInfoType::groupItem] = false;
        QRandomGenerator* intGenerator = QRandomGenerator::global();  //静态方法，全局唯一的实力，但是不能定义在类外面，否则重复定义
        iconPath= ":/img/head" + QString::number(intGenerator->bounded(1,7)); //1-6的随即数字
    }
};
