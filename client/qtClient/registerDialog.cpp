#include "registerDialog.h"
#include "ui_registerDialog.h"
#include <QDialogButtonBox>
#include "baseClient.h"

RegisterDialog::RegisterDialog(Client* client, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    orignStyle = ui->groupName->styleSheet();
    //这个是从网上参考的颜色
    warningStyle = "color: rgb(178,34,34);";
    connect(this, &RegisterDialog::registerGroup, client->qBaseClient, &BaseClient::registerGroup, Qt::DirectConnection);
    connect(ui->isSubmit, &QDialogButtonBox::accepted, this, [&](){  //第三个参数要填this，否则没反应
        if(ui->groupName->text().isEmpty()){
            ui->groupName->setStyleSheet("background-"+warningStyle);
            return;
         }
        emit registerGroup(ui->groupName->text(), ui->groupSummary->text());
        //关闭内嵌窗口
        this->close();
        std::cout << "注册按钮已提交" << std::endl;
    });
    connect(ui->isSubmit, &QDialogButtonBox::rejected, this, [&](){
        this->close();
    });
    //昵称，概要
    connect(ui->groupName, &QLineEdit::textChanged, this, [&](){
        ui->groupName->setStyleSheet("background-"+orignStyle);
    });
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}
