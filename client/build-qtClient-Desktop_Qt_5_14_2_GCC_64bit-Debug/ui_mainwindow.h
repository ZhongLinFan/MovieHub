/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "returnbtn.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout_4;
    QWidget *moviePlayer;
    QVBoxLayout *verticalLayout_2;
    QGraphicsView *graphicsView;
    QWidget *widget_3;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *playBtn;
    QPushButton *stopBtn;
    QSlider *horizontalSlider;
    QStackedWidget *rightWidget;
    QWidget *homePage;
    QGridLayout *gridLayout_3;
    QPushButton *addRelationsBtn;
    QPushButton *logoffBtn;
    QPushButton *logoutBtn;
    QPushButton *registerGroupBtn;
    QTextBrowser *baseInfo;
    QListWidget *userInfo;
    QWidget *chatRoom;
    QVBoxLayout *verticalLayout;
    QWidget *statusBar;
    QHBoxLayout *horizontalLayout;
    ReturnBtn *returnBtn;
    QTextBrowser *chatName;
    QListWidget *chatHistory;
    QWidget *sendWidget;
    QHBoxLayout *horizontalLayout_2;
    QPlainTextEdit *sendBuf;
    QPushButton *sendBtn;
    QMenuBar *menubar;
    QMenu *menuMovieHub;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(788, 470);
        MainWindow->setMaximumSize(QSize(16777215, 16777215));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayout_4 = new QHBoxLayout(centralwidget);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        moviePlayer = new QWidget(centralwidget);
        moviePlayer->setObjectName(QString::fromUtf8("moviePlayer"));
        verticalLayout_2 = new QVBoxLayout(moviePlayer);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        graphicsView = new QGraphicsView(moviePlayer);
        graphicsView->setObjectName(QString::fromUtf8("graphicsView"));

        verticalLayout_2->addWidget(graphicsView);

        widget_3 = new QWidget(moviePlayer);
        widget_3->setObjectName(QString::fromUtf8("widget_3"));
        horizontalLayout_3 = new QHBoxLayout(widget_3);
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        playBtn = new QPushButton(widget_3);
        playBtn->setObjectName(QString::fromUtf8("playBtn"));

        horizontalLayout_3->addWidget(playBtn);

        stopBtn = new QPushButton(widget_3);
        stopBtn->setObjectName(QString::fromUtf8("stopBtn"));

        horizontalLayout_3->addWidget(stopBtn);

        horizontalSlider = new QSlider(widget_3);
        horizontalSlider->setObjectName(QString::fromUtf8("horizontalSlider"));
        horizontalSlider->setOrientation(Qt::Horizontal);

        horizontalLayout_3->addWidget(horizontalSlider);


        verticalLayout_2->addWidget(widget_3);


        horizontalLayout_4->addWidget(moviePlayer);

        rightWidget = new QStackedWidget(centralwidget);
        rightWidget->setObjectName(QString::fromUtf8("rightWidget"));
        rightWidget->setMaximumSize(QSize(250, 16777215));
        homePage = new QWidget();
        homePage->setObjectName(QString::fromUtf8("homePage"));
        gridLayout_3 = new QGridLayout(homePage);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        addRelationsBtn = new QPushButton(homePage);
        addRelationsBtn->setObjectName(QString::fromUtf8("addRelationsBtn"));

        gridLayout_3->addWidget(addRelationsBtn, 3, 2, 1, 1);

        logoffBtn = new QPushButton(homePage);
        logoffBtn->setObjectName(QString::fromUtf8("logoffBtn"));

        gridLayout_3->addWidget(logoffBtn, 2, 2, 1, 1);

        logoutBtn = new QPushButton(homePage);
        logoutBtn->setObjectName(QString::fromUtf8("logoutBtn"));

        gridLayout_3->addWidget(logoutBtn, 2, 1, 1, 1);

        registerGroupBtn = new QPushButton(homePage);
        registerGroupBtn->setObjectName(QString::fromUtf8("registerGroupBtn"));

        gridLayout_3->addWidget(registerGroupBtn, 3, 1, 1, 1);

        baseInfo = new QTextBrowser(homePage);
        baseInfo->setObjectName(QString::fromUtf8("baseInfo"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(baseInfo->sizePolicy().hasHeightForWidth());
        baseInfo->setSizePolicy(sizePolicy);
        baseInfo->setMinimumSize(QSize(0, 40));
        baseInfo->setMaximumSize(QSize(258, 50));

        gridLayout_3->addWidget(baseInfo, 0, 1, 1, 2);

        userInfo = new QListWidget(homePage);
        userInfo->setObjectName(QString::fromUtf8("userInfo"));
        userInfo->setEnabled(true);

        gridLayout_3->addWidget(userInfo, 1, 1, 1, 2);

        rightWidget->addWidget(homePage);
        chatRoom = new QWidget();
        chatRoom->setObjectName(QString::fromUtf8("chatRoom"));
        verticalLayout = new QVBoxLayout(chatRoom);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        statusBar = new QWidget(chatRoom);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(statusBar->sizePolicy().hasHeightForWidth());
        statusBar->setSizePolicy(sizePolicy1);
        statusBar->setMinimumSize(QSize(0, 30));
        statusBar->setMaximumSize(QSize(250, 30));
        horizontalLayout = new QHBoxLayout(statusBar);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        returnBtn = new ReturnBtn(statusBar);
        returnBtn->setObjectName(QString::fromUtf8("returnBtn"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(returnBtn->sizePolicy().hasHeightForWidth());
        returnBtn->setSizePolicy(sizePolicy2);
        returnBtn->setMinimumSize(QSize(15, 15));
        returnBtn->setMaximumSize(QSize(15, 15));
        returnBtn->setStyleSheet(QString::fromUtf8(""));

        horizontalLayout->addWidget(returnBtn);

        chatName = new QTextBrowser(statusBar);
        chatName->setObjectName(QString::fromUtf8("chatName"));
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(chatName->sizePolicy().hasHeightForWidth());
        chatName->setSizePolicy(sizePolicy3);

        horizontalLayout->addWidget(chatName);


        verticalLayout->addWidget(statusBar);

        chatHistory = new QListWidget(chatRoom);
        chatHistory->setObjectName(QString::fromUtf8("chatHistory"));
        chatHistory->setMinimumSize(QSize(231, 0));
        chatHistory->setMaximumSize(QSize(231, 16777215));

        verticalLayout->addWidget(chatHistory);

        sendWidget = new QWidget(chatRoom);
        sendWidget->setObjectName(QString::fromUtf8("sendWidget"));
        sendWidget->setEnabled(true);
        sendWidget->setMaximumSize(QSize(250, 40));
        horizontalLayout_2 = new QHBoxLayout(sendWidget);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        sendBuf = new QPlainTextEdit(sendWidget);
        sendBuf->setObjectName(QString::fromUtf8("sendBuf"));
        sendBuf->setMaximumSize(QSize(258, 40));

        horizontalLayout_2->addWidget(sendBuf);

        sendBtn = new QPushButton(sendWidget);
        sendBtn->setObjectName(QString::fromUtf8("sendBtn"));

        horizontalLayout_2->addWidget(sendBtn);


        verticalLayout->addWidget(sendWidget);

        rightWidget->addWidget(chatRoom);

        horizontalLayout_4->addWidget(rightWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 788, 25));
        menuMovieHub = new QMenu(menubar);
        menuMovieHub->setObjectName(QString::fromUtf8("menuMovieHub"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuMovieHub->menuAction());

        retranslateUi(MainWindow);

        rightWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        playBtn->setText(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276", nullptr));
        stopBtn->setText(QCoreApplication::translate("MainWindow", "\346\232\202\345\201\234", nullptr));
        addRelationsBtn->setText(QCoreApplication::translate("MainWindow", "\346\267\273\345\212\240", nullptr));
        logoffBtn->setText(QCoreApplication::translate("MainWindow", "\346\263\250\351\224\200", nullptr));
        logoutBtn->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        registerGroupBtn->setText(QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214\347\276\244\350\201\212", nullptr));
        sendBtn->setText(QCoreApplication::translate("MainWindow", "\345\217\221\351\200\201", nullptr));
        menuMovieHub->setTitle(QCoreApplication::translate("MainWindow", "MovieHub", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
