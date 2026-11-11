#ifndef DEVPAGE_H
#define DEVPAGE_H

#include <QWidget>
#include <QString>

#include "page.hpp"

class DevPage : public Page{
    Q_OBJECT
public:
    explicit DevPage(QWidget* parent = nullptr);
    QString title() const override;
};

#endif // DEVPAGE_H