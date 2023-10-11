/********************************************************************************
** Form generated from reading UI file 'registerDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REGISTERDIALOG_H
#define UI_REGISTERDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_RegisterDialog
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *input;
    QGridLayout *gridLayout;
    QLineEdit *groupSummary;
    QLineEdit *groupName;
    QLabel *groupSummaryLabel;
    QLabel *groupNameLabel;
    QWidget *operatorInput;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QDialogButtonBox *isSubmit;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *RegisterDialog)
    {
        if (RegisterDialog->objectName().isEmpty())
            RegisterDialog->setObjectName(QString::fromUtf8("RegisterDialog"));
        RegisterDialog->resize(259, 161);
        verticalLayout = new QVBoxLayout(RegisterDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        input = new QWidget(RegisterDialog);
        input->setObjectName(QString::fromUtf8("input"));
        gridLayout = new QGridLayout(input);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        groupSummary = new QLineEdit(input);
        groupSummary->setObjectName(QString::fromUtf8("groupSummary"));

        gridLayout->addWidget(groupSummary, 1, 3, 1, 1);

        groupName = new QLineEdit(input);
        groupName->setObjectName(QString::fromUtf8("groupName"));

        gridLayout->addWidget(groupName, 0, 3, 1, 1);

        groupSummaryLabel = new QLabel(input);
        groupSummaryLabel->setObjectName(QString::fromUtf8("groupSummaryLabel"));

        gridLayout->addWidget(groupSummaryLabel, 1, 2, 1, 1);

        groupNameLabel = new QLabel(input);
        groupNameLabel->setObjectName(QString::fromUtf8("groupNameLabel"));

        gridLayout->addWidget(groupNameLabel, 0, 2, 1, 1);


        verticalLayout->addWidget(input);

        operatorInput = new QWidget(RegisterDialog);
        operatorInput->setObjectName(QString::fromUtf8("operatorInput"));
        horizontalLayout = new QHBoxLayout(operatorInput);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(19, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        isSubmit = new QDialogButtonBox(operatorInput);
        isSubmit->setObjectName(QString::fromUtf8("isSubmit"));
        isSubmit->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout->addWidget(isSubmit);

        horizontalSpacer_2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addWidget(operatorInput);


        retranslateUi(RegisterDialog);

        QMetaObject::connectSlotsByName(RegisterDialog);
    } // setupUi

    void retranslateUi(QDialog *RegisterDialog)
    {
        RegisterDialog->setWindowTitle(QCoreApplication::translate("RegisterDialog", "Dialog", nullptr));
        groupSummaryLabel->setText(QCoreApplication::translate("RegisterDialog", "\347\276\244\347\256\200\344\273\213\357\274\232", nullptr));
        groupNameLabel->setText(QCoreApplication::translate("RegisterDialog", "\347\276\244\346\230\265\347\247\260\357\274\232", nullptr));
    } // retranslateUi

};

namespace Ui {
    class RegisterDialog: public Ui_RegisterDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REGISTERDIALOG_H
