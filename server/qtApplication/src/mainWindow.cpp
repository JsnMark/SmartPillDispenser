#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QtWidgets/qstackedwidget.h>


#include "mainWindow.hpp"
#include "sideBar.hpp"
#include "topBar.hpp"
#include "mainPage.hpp"
#include "devPage.hpp"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent){
    // on heap since parents own them
    QWidget* central = new QWidget(this); // required to have a central widget
    QHBoxLayout* outer = new QHBoxLayout(central);
    QVBoxLayout* inner = new QVBoxLayout();

    topbar = new TopBar();
    inner->addWidget(topbar);

    sidebar = new SideBar();
    outer->addWidget(sidebar);
    outer->addLayout(inner, 1);

    stackedWidget = new QStackedWidget();
    MainPage* mainPage = new MainPage();
    stackedWidget->addWidget(mainPage);
    DevPage* devPage = new DevPage();
    stackedWidget->addWidget(devPage);

    connect(sidebar, &SideBar::pageSelected, this, [this](int i) {
        stackedWidget->setCurrentIndex(i);
        Page* page = static_cast<Page*>(stackedWidget->currentWidget());
        topbar->setTitle(page->title());
        page->onShown();
    });
    topbar->setTitle(mainPage->title());

    inner->addWidget(stackedWidget);


    setCentralWidget(central);
    resize(900, 600);
}