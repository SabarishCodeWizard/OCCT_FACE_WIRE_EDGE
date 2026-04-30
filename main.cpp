#include <QApplication>
#include "OcctWidget.h"

int main(int argc, char *argv[])
{
    // Force X11 backend on Linux for OCCT compatibility
    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication app(argc, argv);

    OcctWidget viewer;
    viewer.resize(1024, 768);
    viewer.setWindowTitle("Industrial OCCT C++ Viewer");
    viewer.show();

    // Load your STEP file
    viewer.loadStepFile("/home/sabarish/Downloads/step/link1.step");
    return app.exec();
}