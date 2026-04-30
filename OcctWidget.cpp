// ==========================================
// 1. ALL QT HEADERS MUST BE INCLUDED FIRST
// ==========================================
#include <QMouseEvent>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

// ==========================================
// 2. LOCAL HEADER INCLUDED SECOND
// ==========================================
#include "OcctWidget.h"

// ==========================================
// 3. OPENCASCADE & X11 HEADERS INCLUDED LAST
// ==========================================
#if defined(Q_OS_WIN)
#include <WNT_Window.hxx>
#elif defined(Q_OS_MAC)
#include <Cocoa_Window.hxx>
#else
#include <Xw_Window.hxx>
#include <X11/Xlib.h>
#endif

#include <Aspect_DisplayConnection.hxx>
#include <STEPControl_Reader.hxx>
#include <AIS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <gp_Pnt.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <Geom_Axis2Placement.hxx>
#include <AIS_Trihedron.hxx>

OcctWidget::OcctWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_PaintOnScreen);

    // --- THIS IS THE FIX ---
    // Forces Qt to give this child widget a real X11 Window ID immediately
    setAttribute(Qt::WA_NativeWindow);
    // -----------------------

    setMouseTracking(true);
}

OcctWidget::~OcctWidget() {}

void OcctWidget::initOCCT()
{
    if (!myView.IsNull()) return;

    Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
    myGraphicDriver = new OpenGl_GraphicDriver(displayConnection);

    myViewer = new V3d_Viewer(myGraphicDriver);
    myViewer->SetDefaultLights();
    myViewer->SetLightOn();

    myViewer->SetRectangularGridValues(0, 0, 10, 10, 0);
    myViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

    myView = myViewer->CreateView();

#if defined(Q_OS_WIN)
    Handle(WNT_Window) wind = new WNT_Window((Aspect_Handle)winId());
#elif defined(Q_OS_MAC)
    Handle(Cocoa_Window) wind = new Cocoa_Window((NSView *)winId());
#else
    Handle(Xw_Window) wind = new Xw_Window(displayConnection, static_cast<Window>(winId()));
#endif

    myView->SetWindow(wind);
    if (!wind->IsMapped()) {
        wind->Map();
    }

    myView->SetBgGradientColors(Quantity_NOC_BLACK, Quantity_NOC_GRAY30, Aspect_GFM_VER);
    myContext = new AIS_InteractiveContext(myViewer);
}

void OcctWidget::loadStepFile(const std::string& filePath)
{
    if (myView.IsNull()) initOCCT();
    // --- NEW: Clear the viewer of old models before loading a new one ---
    myContext->RemoveAll(Standard_True);
    // --------------------------------------------------------------------
    STEPControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(filePath.c_str());

    if (stat == IFSelect_RetDone) {
        reader.TransferRoots();
        TopoDS_Shape shape = reader.OneShape();

        // 1. Calculate Center of Mass
        GProp_GProps gprops;
        BRepGProp::VolumeProperties(shape, gprops);
        gp_Pnt centerOfMass = gprops.CentreOfMass();

        // 2. Calculate Bounding Box
        Bnd_Box boundingBox;
        BRepBndLib::Add(shape, boundingBox);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        boundingBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        gp_Pnt boundingBoxCenter((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);

        qDebug() << "\n==========================================================";
        qDebug() << "[MODEL ORIGIN DATA] STEP File Loaded Successfully";
        qDebug() << "Absolute Part Origin   : [0.000, 0.000, 0.000] <-- (Wireframe Reference)";
        qDebug().nospace() << "Center of Mass (X,Y,Z) : [" << centerOfMass.X() << ", " << centerOfMass.Y() << ", " << centerOfMass.Z() << "]";
        qDebug().nospace() << "Bounding Box Center    : [" << boundingBoxCenter.X() << ", " << boundingBoxCenter.Y() << ", " << boundingBoxCenter.Z() << "]";
        qDebug() << "==========================================================\n";

        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        myContext->SetDisplayMode(aisShape, 1, Standard_False);
        myContext->Display(aisShape, Standard_True);

        // =====================================================================
        // Initialize Custom Origin to Center of Mass initially
        // =====================================================================
        myCustomOrigin = centerOfMass;
        gp_Ax2 partOriginCoords(myCustomOrigin, gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));

        Handle(Geom_Axis2Placement) originPlacement = new Geom_Axis2Placement(partOriginCoords);
        myOriginMarker = new AIS_Trihedron(originPlacement);
        myOriginMarker->SetSize(100.0);
        myContext->Display(myOriginMarker, Standard_True);

        myView->FitAll();
        myView->Redraw();

        setSelectionMode(2); // Default to EDGE selection
    } else {
        qWarning() << "[ERROR] Failed to load STEP file:" << filePath.c_str();
    }
}

void OcctWidget::setSelectionMode(int mode)
{
    if(myContext.IsNull()) return;

    myContext->Deactivate();

    switch(mode) {
    case 1:
        myContext->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));
        qDebug() << "Selection Mode changed to: FACE";
        break;
    case 2:
        myContext->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
        qDebug() << "Selection Mode changed to: EDGE";
        break;
    case 3:
        myContext->Activate(AIS_Shape::SelectionMode(TopAbs_WIRE));
        qDebug() << "Selection Mode changed to: WIRE";
        break;
    default:
        myContext->Activate(0);
        qDebug() << "Selection Mode changed to: WHOLE SHAPE";
    }
}

void OcctWidget::enableOriginSelectionMode()
{
    myIsSettingOriginMode = true;
    qDebug() << "\n[SYSTEM] Origin Selection Mode ACTIVATED. Click any part of the model to snap the origin to it.";
}

void OcctWidget::saveSelectionToFile(const QString& filename)
{
    if(myContext.IsNull()) return;

    // Ask the user for the resolution in mm
    bool ok;
    double resolution = QInputDialog::getDouble(this,
                                                tr("Set Robot Path Resolution"),
                                                tr("Enter point spacing (mm):"),
                                                2.0,   // Default value
                                                0.1,   // Minimum value
                                                100.0, // Maximum value
                                                2,     // Decimals
                                                &ok);

    if (!ok) {
        qDebug() << "Data extraction cancelled by user.";
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << filename;
        return;
    }

    QTextStream out(&file);
    if (file.size() == 0) {
        out << "X,Y,Z\n";
    }

    myContext->InitSelected();
    while (myContext->MoreSelected()) {
        TopoDS_Shape shape = myContext->SelectedShape();

        Handle(AIS_Shape) plottedPath = new AIS_Shape(shape);
        myContext->SetColor(plottedPath, Quantity_NOC_RED, Standard_False);
        myContext->SetWidth(plottedPath, 3.0, Standard_False);
        myContext->Display(plottedPath, Standard_True);

        switch (shape.ShapeType()) {
        case TopAbs_FACE:
            qDebug() << "Processing selected FACE with resolution:" << resolution << "mm";
            processFace(TopoDS::Face(shape), out, resolution);
            break;
        case TopAbs_WIRE:
            qDebug() << "Processing selected WIRE with resolution:" << resolution << "mm";
            processWire(TopoDS::Wire(shape), out, resolution);
            break;
        case TopAbs_EDGE:
            qDebug() << "Processing selected EDGE with resolution:" << resolution << "mm";
            processEdge(TopoDS::Edge(shape), out, resolution);
            break;
        default:
            qWarning() << "Selected shape is not a Face, Wire, or Edge.";
            break;
        }

        myContext->NextSelected();
    }

    file.close();
    qDebug() << "Robot path successfully written to Excel file:" << filename;
}

void OcctWidget::processFace(const TopoDS_Face& face, QTextStream& out, double resolution)
{
    TopExp_Explorer wireExplorer(face, TopAbs_WIRE);
    int loopCount = 1;
    for (; wireExplorer.More(); wireExplorer.Next()) {
        TopoDS_Wire wire = TopoDS::Wire(wireExplorer.Current());
        QString boundaryMarker = QString("--- NEW BOUNDARY LOOP %1 ---").arg(loopCount);
        out << boundaryMarker << "\n";
        qDebug() << "\n" << boundaryMarker;

        processWire(wire, out, resolution);
        loopCount++;
    }
}

void OcctWidget::processWire(const TopoDS_Wire& wire, QTextStream& out, double resolution)
{
    BRepAdaptor_CompCurve compCurve(wire, Standard_True);

    Standard_Real first = compCurve.FirstParameter();
    Standard_Real last = compCurve.LastParameter();
    GCPnts_UniformAbscissa discretizer(compCurve, resolution, first, last);

    if (discretizer.IsDone()) {
        qDebug() << "--- Start Extracting WIRE (" << static_cast<int>(discretizer.NbPoints()) << " points) ---";
        for (int i = 1; i <= discretizer.NbPoints(); ++i) {
            Standard_Real param = discretizer.Parameter(i);
            gp_Pnt pt = compCurve.Value(param);

            // MATH: Absolute Point - Custom Origin = Local Coordinates
            double localX = pt.X() - myCustomOrigin.X();
            double localY = pt.Y() - myCustomOrigin.Y();
            double localZ = pt.Z() - myCustomOrigin.Z();

            out << localX << "," << localY << "," << localZ << "\n";
            qDebug().nospace() << "  -> Pt " << i << ": [X: " << localX << ", Y: " << localY << ", Z: " << localZ << "]";
        }
        qDebug() << "--- Finished WIRE ---";
    }
}

void OcctWidget::processEdge(const TopoDS_Edge& edge, QTextStream& out, double resolution)
{
    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) return;

    BRepAdaptor_Curve adaptor(edge);
    GCPnts_UniformAbscissa discretizer(adaptor, resolution, first, last);

    if (discretizer.IsDone()) {
        qDebug() << "--- Start Extracting EDGE (" << static_cast<int>(discretizer.NbPoints()) << " points) ---";
        for (int i = 1; i <= discretizer.NbPoints(); ++i) {
            Standard_Real param = discretizer.Parameter(i);
            gp_Pnt pt = adaptor.Value(param);

            // MATH: Absolute Point - Custom Origin = Local Coordinates
            double localX = pt.X() - myCustomOrigin.X();
            double localY = pt.Y() - myCustomOrigin.Y();
            double localZ = pt.Z() - myCustomOrigin.Z();

            out << localX << "," << localY << "," << localZ << "\n";
            qDebug().nospace() << "  -> Pt " << i << ": [X: " << localX << ", Y: " << localY << ", Z: " << localZ << "]";
        }
        qDebug() << "--- Finished EDGE ---";
    }
}

void OcctWidget::paintEvent(QPaintEvent *)
{
    if (myView.IsNull()) initOCCT();
    myView->Redraw();
}

void OcctWidget::resizeEvent(QResizeEvent *)
{
    if (!myView.IsNull()) myView->MustBeResized();
}

void OcctWidget::mousePressEvent(QMouseEvent *event)
{
    myLastMousePos = event->pos();

    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x() * devicePixelRatio();
        int y = event->pos().y() * devicePixelRatio();

        myContext->MoveTo(x, y, myView, Standard_True);

        if (event->modifiers() & Qt::ShiftModifier) {
            myContext->ShiftSelect(Standard_True);
        } else {
            myContext->Select(Standard_True);
        }

        myView->Redraw();
        myContext->InitSelected();

        if (myContext->HasSelectedShape()) {

            // ========================================================
            // BRANCH 1: User is setting a new origin
            // ========================================================
            if (myIsSettingOriginMode) {
                TopoDS_Shape selectedShape = myContext->SelectedShape();

                // Calculate the exact bounding box center of whatever they clicked
                Bnd_Box boundingBox;
                BRepBndLib::Add(selectedShape, boundingBox);
                Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
                boundingBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                gp_Pnt newOriginSnap((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);

                // Ask for confirmation using a professional message box
                QString msg = QString("Do you want to set the new Robot Origin here?\n\nX: %1\nY: %2\nZ: %3")
                                  .arg(newOriginSnap.X(), 0, 'f', 2)
                                  .arg(newOriginSnap.Y(), 0, 'f', 2)
                                  .arg(newOriginSnap.Z(), 0, 'f', 2);

                if (QMessageBox::question(this, "Confirm New Origin", msg) == QMessageBox::Yes) {
                    myCustomOrigin = newOriginSnap; // Save the offset

                    // Move the visual Trihedron to the new spot
                    gp_Ax2 newCoords(myCustomOrigin, gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
                    Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(newCoords);
                    myOriginMarker->SetComponent(placement);
                    myContext->Redisplay(myOriginMarker, Standard_True);

                    qDebug() << "[SUCCESS] New Origin Set!";
                }

                // Turn off origin mode so the next click extracts data normally
                myIsSettingOriginMode = false;
                myContext->ClearSelected(Standard_True);
                return; // Stop here so we don't trigger the CSV export
            }

            // ========================================================
            // BRANCH 2: Normal Robot Path Extraction
            // ========================================================
            qDebug() << "\n[SUCCESS] Geometry selected! Triggering data extraction...";
            // Adjust this path back to your specific system layout as needed
            saveSelectionToFile("/home/sabarish/Downloads/robot_path.csv");
        } else {
            // Un-comment if you want standard debug warnings for missed clicks
            // qDebug() << "[WARNING] Clicked, but no geometry was detected under the mouse.";
        }
    }
}

void OcctWidget::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->pos().x() * devicePixelRatio();
    int y = event->pos().y() * devicePixelRatio();

    if (event->buttons() & Qt::RightButton) {
        myView->Rotation(x, y);
    } else if (event->buttons() & Qt::MiddleButton) {
        int lastX = myLastMousePos.x() * devicePixelRatio();
        int lastY = myLastMousePos.y() * devicePixelRatio();
        myView->Pan(x - lastX, lastY - y);
    } else {
        myContext->MoveTo(x, y, myView, Standard_True);
    }
    myLastMousePos = event->pos();
}

void OcctWidget::wheelEvent(QWheelEvent *event)
{
    myView->Zoom(0, 0, event->angleDelta().y() / 10, 0);
}