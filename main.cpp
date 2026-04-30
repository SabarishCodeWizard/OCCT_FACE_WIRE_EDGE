#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Force X11 backend on Linux for OCCT compatibility
    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication app(argc, argv);

    // Apply a global dark industrial theme
    app.setStyle("Fusion");
    app.setStyleSheet(
        "QMainWindow { background-color: #1E1E1E; }"
        "QMessageBox { background-color: #2D2D30; color: #FFFFFF; }"
        "QMessageBox QLabel { color: #FFFFFF; font-size: 14px; }"
        "QMessageBox QPushButton { background-color: #3E3E42; color: white; padding: 6px 20px; border-radius: 3px; }"
        "QMessageBox QPushButton:hover { background-color: #505050; }"
        "QInputDialog { background-color: #2D2D30; color: #FFFFFF; }"
        "QInputDialog QLabel { color: #FFFFFF; }"
        "QInputDialog QDoubleSpinBox { background-color: #1E1E1E; color: white; padding: 4px; border: 1px solid #3E3E42; }"
        );

    MainWindow window;
    window.resize(1280, 800);
    window.setWindowTitle("6-Axis Robot Studio - Centered View");
    window.show();

    return app.exec();
}