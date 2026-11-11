#include "mainWindow.hpp"
#include "sideBar.hpp"
#include "topBar.hpp"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent){
    // on heap since these are still needed, but they wont change
    QWidget* central = new QWidget(this); // required to have a central widget
    QHBoxLayout* outer = new QHBoxLayout(central);
    QVBoxLayout* inner = new QVBoxLayout;

    sidebar = new SideBar();
    outer->addWidget(sidebar);
    outer->addStretch(0);

    topbar = new TopBar();
    inner->addWidget(topbar);
    inner->addStretch(1);



    outer->addLayout(inner, 1);
    setCentralWidget(central);
    resize(900, 600);
}