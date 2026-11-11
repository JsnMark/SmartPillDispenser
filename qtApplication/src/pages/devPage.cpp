#include <QString>
#include <QLabel>
#include <QVBoxLayout>
#include "devPage.hpp"

DevPage::DevPage(QWidget* parent) : Page(parent){
    QLabel* label = new QLabel(title(), this);
    QFont font = label->font();
    font.setPointSize(14);
    label->setFont(font);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->addStretch();
}

QString DevPage::title() const{
    return "Dev Page";
}