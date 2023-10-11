#ifndef ADDRELATIONSDIALOG_H
#define ADDRELATIONSDIALOG_H

#include <QDialog>
#include "client.h"

namespace Ui {
class AddRelationsDialog;
}

class AddRelationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddRelationsDialog(Client* client, QWidget *parent = nullptr);
    ~AddRelationsDialog();
signals:
    void addRelations(int modid, int id);
private:
    Ui::AddRelationsDialog *ui;
    QString orignStyle;
    QString warningStyle;
};

#endif // ADDRELATIONSDIALOG_H
