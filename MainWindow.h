#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "OcctWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadModel(const std::string& filepath);

private slots:
    void triggerOriginMode();
    void openLoadDialog(); // NEW: Slot for the Load Button

private:
    OcctWidget *myOcctWidget;
};

#endif // MAINWINDOW_H