/********************************************************************************
** Form generated from reading UI file 'addRelationsDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDRELATIONSDIALOG_H
#define UI_ADDRELATIONSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AddRelationsDialog
{
public:
    QVBoxLayout *verticalLayout_4;
    QWidget *widget_2;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *chooseGroupAdd;
    QLabel *gidLabel;
    QLineEdit *gid;
    QWidget *widget_4;
    QVBoxLayout *verticalLayout;
    QWidget *widget;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *chooseUserAdd;
    QLabel *uidLabel;
    QLineEdit *uid;
    QWidget *widget_3;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_2;
    QDialogButtonBox *isSubmit;
    QSpacerItem *horizontalSpacer;

    void setupUi(QDialog *AddRelationsDialog)
    {
        if (AddRelationsDialog->objectName().isEmpty())
            AddRelationsDialog->setObjectName(QString::fromUtf8("AddRelationsDialog"));
        AddRelationsDialog->resize(228, 288);
        verticalLayout_4 = new QVBoxLayout(AddRelationsDialog);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        widget_2 = new QWidget(AddRelationsDialog);
        widget_2->setObjectName(QString::fromUtf8("widget_2"));
        verticalLayout_3 = new QVBoxLayout(widget_2);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        chooseGroupAdd = new QCheckBox(widget_2);
        chooseGroupAdd->setObjectName(QString::fromUtf8("chooseGroupAdd"));
        chooseGroupAdd->setCheckable(true);

        verticalLayout_3->addWidget(chooseGroupAdd);

        gidLabel = new QLabel(widget_2);
        gidLabel->setObjectName(QString::fromUtf8("gidLabel"));

        verticalLayout_3->addWidget(gidLabel);

        gid = new QLineEdit(widget_2);
        gid->setObjectName(QString::fromUtf8("gid"));

        verticalLayout_3->addWidget(gid);


        verticalLayout_4->addWidget(widget_2);

        widget_4 = new QWidget(AddRelationsDialog);
        widget_4->setObjectName(QString::fromUtf8("widget_4"));
        verticalLayout = new QVBoxLayout(widget_4);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        widget = new QWidget(widget_4);
        widget->setObjectName(QString::fromUtf8("widget"));
        verticalLayout_2 = new QVBoxLayout(widget);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        chooseUserAdd = new QCheckBox(widget);
        chooseUserAdd->setObjectName(QString::fromUtf8("chooseUserAdd"));

        verticalLayout_2->addWidget(chooseUserAdd);

        uidLabel = new QLabel(widget);
        uidLabel->setObjectName(QString::fromUtf8("uidLabel"));

        verticalLayout_2->addWidget(uidLabel);

        uid = new QLineEdit(widget);
        uid->setObjectName(QString::fromUtf8("uid"));

        verticalLayout_2->addWidget(uid);


        verticalLayout->addWidget(widget);


        verticalLayout_4->addWidget(widget_4);

        widget_3 = new QWidget(AddRelationsDialog);
        widget_3->setObjectName(QString::fromUtf8("widget_3"));
        horizontalLayout = new QHBoxLayout(widget_3);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_2 = new QSpacerItem(4, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        isSubmit = new QDialogButtonBox(widget_3);
        isSubmit->setObjectName(QString::fromUtf8("isSubmit"));
        isSubmit->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout->addWidget(isSubmit);

        horizontalSpacer = new QSpacerItem(4, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout_4->addWidget(widget_3);


        retranslateUi(AddRelationsDialog);

        QMetaObject::connectSlotsByName(AddRelationsDialog);
    } // setupUi

    void retranslateUi(QDialog *AddRelationsDialog)
    {
        AddRelationsDialog->setWindowTitle(QCoreApplication::translate("AddRelationsDialog", "Dialog", nullptr));
        chooseGroupAdd->setText(QCoreApplication::translate("AddRelationsDialog", "\346\267\273\345\212\240\347\276\244\347\273\204", nullptr));
        gidLabel->setText(QCoreApplication::translate("AddRelationsDialog", "\347\276\244\347\273\204ID:", nullptr));
        chooseUserAdd->setText(QCoreApplication::translate("AddRelationsDialog", "\346\267\273\345\212\240\347\224\250\346\210\267", nullptr));
        uidLabel->setText(QCoreApplication::translate("AddRelationsDialog", "\347\224\250\346\210\267ID:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AddRelationsDialog: public Ui_AddRelationsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDRELATIONSDIALOG_H
