/********************************************************************************
** Form generated from reading UI file 'welcome.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WELCOME_H
#define UI_WELCOME_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Welcome
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTextBrowser *textBrowser;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QLabel *uidLabel;
    QLineEdit *uid;
    QLabel *passwdLabel;
    QLineEdit *passwd;
    QPushButton *loginAndRegisterBtn;
    QPushButton *chooseRegisterBtn;
    QMenuBar *menubar;
    QMenu *menuMovieHub;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *Welcome)
    {
        if (Welcome->objectName().isEmpty())
            Welcome->setObjectName(QString::fromUtf8("Welcome"));
        Welcome->resize(420, 480);
        centralwidget = new QWidget(Welcome);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        textBrowser = new QTextBrowser(centralwidget);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));

        verticalLayout->addWidget(textBrowser);

        widget = new QWidget(centralwidget);
        widget->setObjectName(QString::fromUtf8("widget"));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        uidLabel = new QLabel(widget);
        uidLabel->setObjectName(QString::fromUtf8("uidLabel"));

        horizontalLayout->addWidget(uidLabel);

        uid = new QLineEdit(widget);
        uid->setObjectName(QString::fromUtf8("uid"));

        horizontalLayout->addWidget(uid);

        passwdLabel = new QLabel(widget);
        passwdLabel->setObjectName(QString::fromUtf8("passwdLabel"));

        horizontalLayout->addWidget(passwdLabel);

        passwd = new QLineEdit(widget);
        passwd->setObjectName(QString::fromUtf8("passwd"));

        horizontalLayout->addWidget(passwd);

        loginAndRegisterBtn = new QPushButton(widget);
        loginAndRegisterBtn->setObjectName(QString::fromUtf8("loginAndRegisterBtn"));

        horizontalLayout->addWidget(loginAndRegisterBtn);


        verticalLayout->addWidget(widget);

        chooseRegisterBtn = new QPushButton(centralwidget);
        chooseRegisterBtn->setObjectName(QString::fromUtf8("chooseRegisterBtn"));

        verticalLayout->addWidget(chooseRegisterBtn);

        Welcome->setCentralWidget(centralwidget);
        menubar = new QMenuBar(Welcome);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 420, 25));
        menuMovieHub = new QMenu(menubar);
        menuMovieHub->setObjectName(QString::fromUtf8("menuMovieHub"));
        Welcome->setMenuBar(menubar);
        statusbar = new QStatusBar(Welcome);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        Welcome->setStatusBar(statusbar);

        menubar->addAction(menuMovieHub->menuAction());

        retranslateUi(Welcome);

        QMetaObject::connectSlotsByName(Welcome);
    } // setupUi

    void retranslateUi(QMainWindow *Welcome)
    {
        Welcome->setWindowTitle(QCoreApplication::translate("Welcome", "MainWindow", nullptr));
        textBrowser->setHtml(QCoreApplication::translate("Welcome", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Cantarell'; font-size:11pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600; font-style:italic; color:#ce5c00;\">Welcome to MovieHub</span></p>\n"
"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><img src=\":/img/welcome\" /></p></body></html>", nullptr));
        uidLabel->setText(QCoreApplication::translate("Welcome", "uid:", nullptr));
        passwdLabel->setText(QCoreApplication::translate("Welcome", "\345\257\206\347\240\201:", nullptr));
        loginAndRegisterBtn->setText(QCoreApplication::translate("Welcome", "\347\231\273\351\231\206", nullptr));
        chooseRegisterBtn->setText(QCoreApplication::translate("Welcome", "\350\277\230\346\262\241\346\234\211\345\270\220\345\217\267\357\274\237\346\263\250\345\206\214\345\270\220\345\217\267", nullptr));
        menuMovieHub->setTitle(QCoreApplication::translate("Welcome", "MovieHub", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Welcome: public Ui_Welcome {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WELCOME_H
