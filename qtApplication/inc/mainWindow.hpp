#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

class TopBar;
class SideBar;

class MainWindow : public QMainWindow{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
private:
    TopBar* topbar;
    SideBar* sidebar;
    QStackedWidget* stackedWidget;
};

#endif // MAINWINDOW_H