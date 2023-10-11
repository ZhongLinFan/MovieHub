#include "returnBtn.h"
#include <QPainter>
#include <QDebug>

ReturnBtn::ReturnBtn(QWidget *parent) : QWidget(parent)
{
    returnIcon.load(":/img/return");
    setFixedSize(returnIcon.size());
}


void ReturnBtn::mousePressEvent(QMouseEvent* ev){
    emit clicked();
}

void ReturnBtn::paintEvent(QPaintEvent* ev){
    QPainter p(this); //在哪里画
    p.drawPixmap(rect(), returnIcon);
}
