#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>
#include "client.h"

namespace Ui {
class RegisterDialog;
}

class Client;
class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(Client* client, QWidget *parent = nullptr);
    ~RegisterDialog();

signals:
    //注册群聊信号
    void registerGroup(const QString& groupName, const QString& summary);  //这里需要const，和bug12一样
public:
    Ui::RegisterDialog *ui;
    QString orignStyle;
    QString warningStyle;
};


#endif // INPUTDIALOG_H
