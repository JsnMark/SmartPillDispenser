#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QWidget>

class SideBar : public QWidget {
    Q_OBJECT
public:
    explicit SideBar(QWidget* parent = nullptr);
};

#endif // SIDEBAR_H