#ifndef RETURNBTN_H
#define RETURNBTN_H

#include <QWidget>

class ReturnBtn : public QWidget
{
    Q_OBJECT
public:
    explicit ReturnBtn(QWidget *parent = nullptr); 

public:
    //产生的鼠标按下事件
    void mousePressEvent(QMouseEvent* ev);
    //产生的绘图事件
    void paintEvent(QPaintEvent* ev);
signals:
    void clicked();

public:
    QPixmap returnIcon;

};
#endif // RETURNBTN_H
