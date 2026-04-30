// ==========================================
// 1. ALL QT HEADERS MUST BE INCLUDED FIRST
// ==========================================
#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>

// ==========================================
// 2. LOCAL HEADER INCLUDED SECOND
// ==========================================
#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // Create the OCCT 3D Viewer Widget
    myOcctWidget = new OcctWidget(this);
    setCentralWidget(myOcctWidget);

    // Create a professional top control bar
    QToolBar *topBar = new QToolBar("Industrial Controls", this);
    topBar->setMovable(false);
    topBar->setStyleSheet("QToolBar { background-color: #2D2D30; border-bottom: 2px solid #3E3E42; padding: 5px; }");
    addToolBar(Qt::TopToolBarArea, topBar);

    // Create the Load Model Button
    QPushButton *btnLoadModel = new QPushButton("📂 Load STEP Model");
    btnLoadModel->setCursor(Qt::PointingHandCursor);
    btnLoadModel->setStyleSheet(
        "QPushButton { "
        "  background-color: #2E8B57; color: white; font-weight: bold; "
        "  padding: 8px 16px; border-radius: 4px; border: none; margin-right: 10px;"
        "} "
        "QPushButton:hover { background-color: #3CB371; }"
        "QPushButton:pressed { background-color: #228B22; }"
        );
    topBar->addWidget(btnLoadModel);

    // Create the Set Origin Button
    QPushButton *btnSetOrigin = new QPushButton("🎯 Set Local Origin");
    btnSetOrigin->setCursor(Qt::PointingHandCursor);
    btnSetOrigin->setStyleSheet(
        "QPushButton { "
        "  background-color: #007ACC; color: white; font-weight: bold; "
        "  padding: 8px 16px; border-radius: 4px; border: none; margin-right: 10px;"
        "} "
        "QPushButton:hover { background-color: #1C97EA; }"
        "QPushButton:pressed { background-color: #005A9E; }"
        );
    topBar->addWidget(btnSetOrigin);

    // ==========================================
    // NEW: Create the Reset Origin Button
    // ==========================================
    QPushButton *btnResetOrigin = new QPushButton("🔄 Reset Origin");
    btnResetOrigin->setCursor(Qt::PointingHandCursor);
    btnResetOrigin->setStyleSheet(
        "QPushButton { "
        "  background-color: #D2691E; color: white; font-weight: bold; " // Industrial Orange
        "  padding: 8px 16px; border-radius: 4px; border: none; "
        "} "
        "QPushButton:hover { background-color: #CD853F; }"
        "QPushButton:pressed { background-color: #A0522D; }"
        );
    topBar->addWidget(btnResetOrigin);

    // Connect the buttons to their actions
    connect(btnLoadModel, &QPushButton::clicked, this, &MainWindow::openLoadDialog);
    connect(btnSetOrigin, &QPushButton::clicked, this, &MainWindow::triggerOriginMode);
    connect(btnResetOrigin, &QPushButton::clicked, this, &MainWindow::resetOrigin); // NEW
}

MainWindow::~MainWindow() {}

void MainWindow::triggerOriginMode()
{
    myOcctWidget->enableOriginSelectionMode();
}

void MainWindow::resetOrigin()
{
    myOcctWidget->resetOrigin();
}

void MainWindow::loadModel(const std::string& filepath)
{
    myOcctWidget->loadStepFile(filepath);
}

void MainWindow::openLoadDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open STEP File"),
                                                    "",
                                                    tr("STEP Files (*.step *.stp);;All Files (*)"));

    if (!fileName.isEmpty()) {
        myOcctWidget->loadStepFile(fileName.toStdString());
    }
}