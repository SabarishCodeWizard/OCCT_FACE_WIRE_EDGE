// ==========================================
// 1. ALL QT HEADERS MUST BE INCLUDED FIRST
// ==========================================
#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStatusBar> // NEW: For the dynamic status text

// ==========================================
// 2. LOCAL HEADER INCLUDED SECOND
// ==========================================
#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // Create the OCCT 3D Viewer Widget
    myOcctWidget = new OcctWidget(this);
    setCentralWidget(myOcctWidget);

    // ==========================================
    // NEW: Setup the Dynamic Status Bar
    // ==========================================
    QStatusBar *statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->setStyleSheet("background-color: #007ACC; color: #FFFFFF; font-weight: bold; padding-left: 10px;");
    statusBar->showMessage("Ready - Awaiting STEP File");

    // Connect the widget's internal signals to the status bar using a Lambda to handle the argument mismatch
    connect(myOcctWidget, &OcctWidget::statusUpdate, this, [statusBar](const QString& msg) {
        statusBar->showMessage(msg);
    });
    // Create a professional top control bar
    QToolBar *topBar = new QToolBar("Industrial Controls", this);
    topBar->setMovable(false);
    topBar->setStyleSheet("QToolBar { background-color: #2D2D30; border-bottom: 2px solid #3E3E42; padding: 5px; }");
    addToolBar(Qt::TopToolBarArea, topBar);

    // 1. Load Button
    QPushButton *btnLoadModel = new QPushButton("📂 Load");
    btnLoadModel->setCursor(Qt::PointingHandCursor);
    btnLoadModel->setStyleSheet("QPushButton { background-color: #2E8B57; color: white; font-weight: bold; padding: 6px 12px; border-radius: 3px; margin-right: 5px; } QPushButton:hover { background-color: #3CB371; }");
    topBar->addWidget(btnLoadModel);

    // 2. Origin Buttons
    QPushButton *btnSetOrigin = new QPushButton("🎯 Set Origin");
    btnSetOrigin->setCursor(Qt::PointingHandCursor);
    btnSetOrigin->setStyleSheet("QPushButton { background-color: #007ACC; color: white; font-weight: bold; padding: 6px 12px; border-radius: 3px; margin-right: 5px;} QPushButton:hover { background-color: #1C97EA; }");
    topBar->addWidget(btnSetOrigin);

    QPushButton *btnResetOrigin = new QPushButton("🔄 Reset Origin");
    btnResetOrigin->setCursor(Qt::PointingHandCursor);
    btnResetOrigin->setStyleSheet("QPushButton { background-color: #D2691E; color: white; font-weight: bold; padding: 6px 12px; border-radius: 3px; margin-right: 15px;} QPushButton:hover { background-color: #CD853F; }");
    topBar->addWidget(btnResetOrigin);

    // ==========================================
    // NEW: History Control Buttons
    // ==========================================
    QPushButton *btnUndo = new QPushButton("↩️ Undo");
    btnUndo->setCursor(Qt::PointingHandCursor);
    btnUndo->setStyleSheet("QPushButton { background-color: #555555; color: white; font-weight: bold; padding: 6px 12px; border-radius: 3px; margin-right: 5px;} QPushButton:hover { background-color: #777777; }");
    topBar->addWidget(btnUndo);

    QPushButton *btnRedo = new QPushButton("↪️ Redo");
    btnRedo->setCursor(Qt::PointingHandCursor);
    btnRedo->setStyleSheet("QPushButton { background-color: #555555; color: white; font-weight: bold; padding: 6px 12px; border-radius: 3px; margin-right: 15px;} QPushButton:hover { background-color: #777777; }");
    topBar->addWidget(btnRedo);

    QPushButton *btnClear = new QPushButton("❌ Clear All Paths");
    btnClear->setCursor(Qt::PointingHandCursor);
    btnClear->setStyleSheet("QPushButton { background-color: #B22222; color: white; font-weight: bold; padding: 6px 12px; border-radius: 3px;} QPushButton:hover { background-color: #DC143C; }");
    topBar->addWidget(btnClear);

    // Wiring everything up
    connect(btnLoadModel, &QPushButton::clicked, this, &MainWindow::openLoadDialog);
    connect(btnSetOrigin, &QPushButton::clicked, this, &MainWindow::triggerOriginMode);
    connect(btnResetOrigin, &QPushButton::clicked, this, &MainWindow::resetOrigin);
    connect(btnUndo, &QPushButton::clicked, myOcctWidget, &OcctWidget::undoSelection);
    connect(btnRedo, &QPushButton::clicked, myOcctWidget, &OcctWidget::redoSelection);
    connect(btnClear, &QPushButton::clicked, myOcctWidget, &OcctWidget::clearSelections);
}

MainWindow::~MainWindow() {}

void MainWindow::triggerOriginMode() { myOcctWidget->enableOriginSelectionMode(); }
void MainWindow::resetOrigin() { myOcctWidget->resetOrigin(); }
void MainWindow::loadModel(const std::string& filepath) { myOcctWidget->loadStepFile(filepath); }

void MainWindow::openLoadDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open STEP File"), "", tr("STEP Files (*.step *.stp);;All Files (*)"));
    if (!fileName.isEmpty()) {
        myOcctWidget->loadStepFile(fileName.toStdString());
    }
}