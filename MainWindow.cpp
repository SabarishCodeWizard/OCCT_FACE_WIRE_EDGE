// ==========================================
// 1. ALL QT HEADERS MUST BE INCLUDED FIRST
// ==========================================
#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog> // NEW: Required for the file browser

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

    // ==========================================
    // NEW: Create the Load Model Button
    // ==========================================
    QPushButton *btnLoadModel = new QPushButton("📂 Load STEP Model");
    btnLoadModel->setCursor(Qt::PointingHandCursor);
    btnLoadModel->setStyleSheet(
        "QPushButton { "
        "  background-color: #2E8B57; color: white; font-weight: bold; " // Industrial Green
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
        "  background-color: #007ACC; color: white; font-weight: bold; " // Industrial Blue
        "  padding: 8px 16px; border-radius: 4px; border: none; "
        "} "
        "QPushButton:hover { background-color: #1C97EA; }"
        "QPushButton:pressed { background-color: #005A9E; }"
        );
    topBar->addWidget(btnSetOrigin);

    // Connect the buttons to their actions
    connect(btnLoadModel, &QPushButton::clicked, this, &MainWindow::openLoadDialog);
    connect(btnSetOrigin, &QPushButton::clicked, this, &MainWindow::triggerOriginMode);
}

MainWindow::~MainWindow() {}

void MainWindow::triggerOriginMode()
{
    myOcctWidget->enableOriginSelectionMode();
}

void MainWindow::loadModel(const std::string& filepath)
{
    myOcctWidget->loadStepFile(filepath);
}

// ==========================================
// NEW: Implementation of the File Dialog
// ==========================================
void MainWindow::openLoadDialog()
{
    // Opens a native file browser filtered for STEP files
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open STEP File"),
                                                    "", // Starts in the default directory
                                                    tr("STEP Files (*.step *.stp);;All Files (*)"));

    // If the user actually selected a file (didn't hit cancel)
    if (!fileName.isEmpty()) {
        myOcctWidget->loadStepFile(fileName.toStdString());
    }
}