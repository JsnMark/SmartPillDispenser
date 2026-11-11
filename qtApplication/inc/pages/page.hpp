#ifndef PAGE_H
#define PAGE_H

#include <QWidget>

class Page : public QWidget{
    Q_OBJECT
public:
    explicit Page(QWidget* parent = nullptr) : QWidget(parent) {};
    virtual QString title() const = 0;
    virtual void onShown() {}
};

#endif // PAGE_H