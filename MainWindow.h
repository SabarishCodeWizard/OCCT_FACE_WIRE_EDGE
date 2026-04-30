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
    void openLoadDialog();
    void resetOrigin(); // NEW: Slot for resetting the origin

private:
    OcctWidget *myOcctWidget;
};

#endif // MAINWINDOW_H