#include <QButtonGroup>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qtoolbutton.h>
#include "sideBar.hpp"

SideBar::SideBar(QWidget* parent) : QWidget(parent){
    QVBoxLayout* layout = new QVBoxLayout(this);
    QButtonGroup* buttonGroup = new QButtonGroup(this);

    QToolButton* mainPageButton = new QToolButton();
    mainPageButton->setText("Main Page");
    mainPageButton->setFixedSize(100, 35);
    layout->addWidget(mainPageButton);
    buttonGroup->addButton(mainPageButton, 0);

    QToolButton* devPageButton = new QToolButton();
    devPageButton->setText("Dev Page");
    devPageButton->setFixedSize(100, 35);
    layout->addWidget(devPageButton);
    buttonGroup->addButton(devPageButton, 1);

    connect(buttonGroup, &QButtonGroup::idClicked, this, &SideBar::pageSelected);

    layout->addStretch(); // push button to top
    setFixedWidth(130);
}