#ifndef WELCOME_H
#define WELCOME_H
#include <QMainWindow>


class Client;
class MainWindow;
namespace Ui {
class Welcome;
}

class Welcome : public QMainWindow
{
    Q_OBJECT

public:
    explicit Welcome(QWidget *parent = nullptr);
    ~Welcome();
    //登陆操作
    void login();
    //选择注册操作
    void chooseRegister();
    //注册操作
    void registerUser();
    //获取server操作
    void getServer();
    //设置client指针
    inline void setClient(Client* client){
        m_client = client;
    }
    //切换主页面
   void switchMainWindow(MainWindow* mainWindow);
private:
    Ui::Welcome *ui;
    Client* m_client;  //当前的页面所属的client
};

#endif // WELCOME_H
