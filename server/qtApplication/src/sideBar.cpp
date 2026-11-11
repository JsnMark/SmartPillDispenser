#include <QButtonGroup>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtWidgets/qboxlayout.h>
#include "sideBar.hpp"

SideBar::SideBar(QWidget* parent) : QWidget(parent){
    QVBoxLayout* layout = new QVBoxLayout(this);
    QButtonGroup* buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    QPushButton* button1 = new QPushButton("Button 1");
    QPushButton* button2 = new QPushButton("Button 2");
    for(QPushButton* b: {button1, button2}){
        b->setCheckable(true);
        b->setFixedSize(100, 35);
        layout->addWidget(b);
        buttonGroup->addButton(b);
    }
    layout->addStretch(); // push button to top
    setFixedWidth(130);
}