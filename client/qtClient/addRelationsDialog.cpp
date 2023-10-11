#include "addRelationsDialog.h"
#include "ui_addRelationsDialog.h"
#include "baseClient.h"

AddRelationsDialog::AddRelationsDialog(Client* client, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddRelationsDialog)
{
    ui->setupUi(this);
    orignStyle = ui->chooseGroupAdd->styleSheet();
    //这个是从网上参考的颜色
    warningStyle = "color: rgb(178,34,34);";
    //确认按钮
    connect(ui->isSubmit, &QDialogButtonBox::accepted, this, [&](){  //第三个参数要为this
        int modid = 0;
        int id = 0;
        if(!ui->chooseGroupAdd->isChecked() && !ui->chooseUserAdd->isChecked()){
            ui->chooseGroupAdd->setStyleSheet(warningStyle);
            ui->chooseUserAdd->setStyleSheet(warningStyle);
            return;
        }else if(ui->chooseUserAdd->isChecked()){
            modid = 1;
            id = ui->uid->text().toInt();
            if(id <= 0){
                ui->uid->setStyleSheet("background-"+warningStyle);
                ui->gid->setStyleSheet("background-"+warningStyle);
                return;
            }
        }else{
            modid = 2;
            id = ui->gid->text().toInt();
            if(id <= 0){
                ui->uid->setStyleSheet("background-"+warningStyle);
                ui->gid->setStyleSheet("background-"+warningStyle);
                return;
            }
        }
        emit addRelations(modid, id);
        //关闭内嵌窗口
        this->close();
    });
    //取消按钮
    connect(ui->isSubmit, &QDialogButtonBox::rejected, this, [&](){
        this->close();
    });
    //组选择按钮
    connect(ui->chooseGroupAdd, &QCheckBox::stateChanged, this, [&](int state){ //stateChanged包含了一个参数
        ui->chooseGroupAdd->setStyleSheet(orignStyle);
        ui->chooseUserAdd->setStyleSheet(orignStyle);
        if(state == Qt::Checked){
             ui->chooseUserAdd->setCheckable(false);
        }else{
             ui->chooseUserAdd->setCheckable(true);
        }
    });
    //用户选择按钮
    connect(ui->chooseUserAdd, &QCheckBox::stateChanged, this, [&](int state){
        ui->chooseUserAdd->setStyleSheet(orignStyle);
        ui->chooseGroupAdd->setStyleSheet(orignStyle);
        if(state == Qt::Checked){
             ui->chooseGroupAdd->setCheckable(false);
        }else{
             ui->chooseGroupAdd->setCheckable(true);
        }
    });
    //组id，uid
    connect(ui->uid, &QLineEdit::textChanged, this, [&](){
        ui->uid->setStyleSheet("background-"+orignStyle);
        ui->gid->setStyleSheet("background-"+orignStyle);
    });
    connect(ui->gid, &QLineEdit::textChanged, this, [&](){
        ui->uid->setStyleSheet("background-"+orignStyle);
        ui->gid->setStyleSheet("background-"+orignStyle);
    });
    connect(this,&AddRelationsDialog::addRelations, client->qBaseClient, &BaseClient::addRelations, Qt::DirectConnection);
}

AddRelationsDialog::~AddRelationsDialog()
{
    delete ui;
}
