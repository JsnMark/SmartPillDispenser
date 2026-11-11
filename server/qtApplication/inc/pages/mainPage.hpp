#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>
#include <QString>

#include "page.hpp"

class MainPage : public Page{
    Q_OBJECT
public:
    explicit MainPage(QWidget* parent = nullptr);
    QString title() const override;
};

#endif // MAINPAGE_H