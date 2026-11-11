#include <QLabel>
#include <QHBoxLayout>
#include "topBar.hpp"

TopBar::TopBar(QWidget* parent) : QWidget(parent){
    title = new QLabel("This is the TopBar", this);
    QFont font = title->font();
    font.setPointSize(18);
    font.setBold(true);
    title->setFont(font);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(title);
    layout->addStretch(); // push to left

    setFixedHeight(50);
}

void TopBar::setTitle(const QString &new_title){
    title->setText(new_title);
}