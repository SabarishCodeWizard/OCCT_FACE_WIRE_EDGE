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
    setAttribute(Qt::WA_NativeWindow);
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

    // Wipe memory cleanly before loading
    myContext->RemoveAll(Standard_True);
    clearSelections();

    STEPControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(filePath.c_str());

    if (stat == IFSelect_RetDone) {
        reader.TransferRoots();
        TopoDS_Shape shape = reader.OneShape();

        GProp_GProps gprops;
        BRepGProp::VolumeProperties(shape, gprops);
        gp_Pnt centerOfMass = gprops.CentreOfMass();

        Bnd_Box boundingBox;
        BRepBndLib::Add(shape, boundingBox);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        boundingBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        gp_Pnt boundingBoxCenter((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);

        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        myContext->SetDisplayMode(aisShape, 1, Standard_False);
        myContext->Display(aisShape, Standard_True);

        myDefaultOrigin = centerOfMass;
        myCustomOrigin = centerOfMass;

        gp_Ax2 partOriginCoords(myCustomOrigin, gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
        Handle(Geom_Axis2Placement) originPlacement = new Geom_Axis2Placement(partOriginCoords);
        myOriginMarker = new AIS_Trihedron(originPlacement);
        myOriginMarker->SetSize(100.0);
        myContext->Display(myOriginMarker, Standard_True);

        myView->FitAll();
        myView->Redraw();
        setSelectionMode(2);

        emit statusUpdate("✅ Model Loaded Successfully. Ready for Selection.");
    } else {
        emit statusUpdate("❌ Error: Failed to load STEP file.");
    }
}

void OcctWidget::resetOrigin()
{
    if (myOriginMarker.IsNull()) {
        emit statusUpdate("⚠️ Warning: No model loaded to reset.");
        return;
    }

    if (QMessageBox::question(this, "Reset Origin", "Are you sure you want to reset the origin back to the default Center of Mass?") == QMessageBox::Yes) {
        myCustomOrigin = myDefaultOrigin;

        gp_Ax2 defaultCoords(myCustomOrigin, gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
        Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(defaultCoords);
        myOriginMarker->SetComponent(placement);
        myContext->Redisplay(myOriginMarker, Standard_True);

        emit statusUpdate("🔄 Origin Reset to Default Center of Mass. Note: CSV data will use this new origin on your next Path action.");
    }
}

void OcctWidget::setSelectionMode(int mode)
{
    if(myContext.IsNull()) return;
    myContext->Deactivate();

    switch(mode) {
    case 1: myContext->Activate(AIS_Shape::SelectionMode(TopAbs_FACE)); break;
    case 2: myContext->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE)); break;
    case 3: myContext->Activate(AIS_Shape::SelectionMode(TopAbs_WIRE)); break;
    default: myContext->Activate(0);
    }
}

void OcctWidget::enableOriginSelectionMode()
{
    myIsSettingOriginMode = true;
    emit statusUpdate("🎯 Origin Mode ACTIVE: Click any edge, face, or wire on the 3D model to snap the origin to it.");
}

// ====================================================================
// NEW CORE ARCHITECTURE: History, Undo, Redo & CSV Generation
// ====================================================================

void OcctWidget::clearSelections()
{
    // Remove all red lines from the screen
    for (const auto& step : myPathHistory) myContext->Remove(step.visualRedPath, Standard_False);
    for (const auto& step : myRedoStack) myContext->Remove(step.visualRedPath, Standard_False);

    myPathHistory.clear();
    myRedoStack.clear();
    myContext->UpdateCurrentViewer();

    regenerateCSV(); // This will clear the file
    emit statusUpdate("❌ All selections cleared. CSV wiped.");
}

void OcctWidget::undoSelection()
{
    if (myPathHistory.empty()) {
        emit statusUpdate("⚠️ Nothing to undo.");
        return;
    }

    // Move from Active History to Redo Stack
    PathData lastAction = myPathHistory.back();
    myPathHistory.pop_back();
    myContext->Remove(lastAction.visualRedPath, Standard_True); // Hide the red line
    myRedoStack.push_back(lastAction);

    regenerateCSV();
    emit statusUpdate(QString("↩️ Undo successful. Current Paths in CSV: %1").arg(myPathHistory.size()));
}

void OcctWidget::redoSelection()
{
    if (myRedoStack.empty()) {
        emit statusUpdate("⚠️ Nothing to redo.");
        return;
    }

    // Move from Redo Stack back to Active History
    PathData nextAction = myRedoStack.back();
    myRedoStack.pop_back();
    myContext->Display(nextAction.visualRedPath, Standard_True); // Show the red line again
    myPathHistory.push_back(nextAction);

    regenerateCSV();
    emit statusUpdate(QString("↪️ Redo successful. Current Paths in CSV: %1").arg(myPathHistory.size()));
}

void OcctWidget::processCurrentSelection()
{
    if (myContext.IsNull() || !myContext->HasSelectedShape()) return;

    bool ok;
    double resolution = QInputDialog::getDouble(this, tr("Set Robot Path Resolution"), tr("Enter point spacing (mm):"), 2.0, 0.1, 100.0, 2, &ok);

    if (!ok) {
        emit statusUpdate("⚠️ Extraction cancelled by user.");
        myContext->ClearSelected(Standard_True);
        return;
    }

    myContext->InitSelected();
    int addedCount = 0;
    while (myContext->MoreSelected()) {
        TopoDS_Shape shape = myContext->SelectedShape();

        Handle(AIS_Shape) plottedPath = new AIS_Shape(shape);
        myContext->SetColor(plottedPath, Quantity_NOC_RED, Standard_False);
        myContext->SetWidth(plottedPath, 3.0, Standard_False);
        myContext->Display(plottedPath, Standard_True);

        // Save the click to History
        myPathHistory.push_back({shape, plottedPath, resolution});
        addedCount++;

        myContext->NextSelected();
    }

    myContext->ClearSelected(Standard_True);
    myRedoStack.clear(); // Standard behavior: doing a new action destroys the Redo timeline

    regenerateCSV();
    emit statusUpdate(QString("✅ Extracted %1 new path(s). Total Paths in CSV: %2").arg(addedCount).arg(myPathHistory.size()));
}

void OcctWidget::regenerateCSV()
{
    QFile file(myCSVPath);
    // WriteOnly + Truncate means it completely overwrites the old file instantly
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << myCSVPath;
        return;
    }

    QTextStream out(&file);
    out << "X,Y,Z\n";

    // Play back the entire history stack to generate the perfect CSV state
    for (const auto& step : myPathHistory) {
        switch (step.shape.ShapeType()) {
        case TopAbs_FACE: processFace(TopoDS::Face(step.shape), out, step.resolution); break;
        case TopAbs_WIRE: processWire(TopoDS::Wire(step.shape), out, step.resolution); break;
        case TopAbs_EDGE: processEdge(TopoDS::Edge(step.shape), out, step.resolution); break;
        default: break;
        }
    }
    file.close();
}

// ====================================================================

void OcctWidget::processFace(const TopoDS_Face& face, QTextStream& out, double resolution)
{
    TopExp_Explorer wireExplorer(face, TopAbs_WIRE);
    int loopCount = 1;
    for (; wireExplorer.More(); wireExplorer.Next()) {
        TopoDS_Wire wire = TopoDS::Wire(wireExplorer.Current());
        QString boundaryMarker = QString("--- NEW BOUNDARY LOOP %1 ---").arg(loopCount);
        out << boundaryMarker << "\n";
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
        for (int i = 1; i <= discretizer.NbPoints(); ++i) {
            Standard_Real param = discretizer.Parameter(i);
            gp_Pnt pt = compCurve.Value(param);

            double localX = pt.X() - myCustomOrigin.X();
            double localY = pt.Y() - myCustomOrigin.Y();
            double localZ = pt.Z() - myCustomOrigin.Z();
            out << localX << "," << localY << "," << localZ << "\n";
        }
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
        for (int i = 1; i <= discretizer.NbPoints(); ++i) {
            Standard_Real param = discretizer.Parameter(i);
            gp_Pnt pt = adaptor.Value(param);

            double localX = pt.X() - myCustomOrigin.X();
            double localY = pt.Y() - myCustomOrigin.Y();
            double localZ = pt.Z() - myCustomOrigin.Z();
            out << localX << "," << localY << "," << localZ << "\n";
        }
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
            if (myIsSettingOriginMode) {
                TopoDS_Shape selectedShape = myContext->SelectedShape();

                Bnd_Box boundingBox;
                BRepBndLib::Add(selectedShape, boundingBox);
                Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
                boundingBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                gp_Pnt newOriginSnap((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);

                QString msg = QString("Do you want to set the new Robot Origin here?\n\nX: %1\nY: %2\nZ: %3")
                                  .arg(newOriginSnap.X(), 0, 'f', 2).arg(newOriginSnap.Y(), 0, 'f', 2).arg(newOriginSnap.Z(), 0, 'f', 2);

                if (QMessageBox::question(this, "Confirm New Origin", msg) == QMessageBox::Yes) {
                    myCustomOrigin = newOriginSnap;

                    gp_Ax2 newCoords(myCustomOrigin, gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
                    Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(newCoords);
                    myOriginMarker->SetComponent(placement);
                    myContext->Redisplay(myOriginMarker, Standard_True);

                    emit statusUpdate("🎯 New Local Origin Set Successfully!");
                } else {
                    emit statusUpdate("Origin Setup Cancelled.");
                }

                myIsSettingOriginMode = false;
                myContext->ClearSelected(Standard_True);
                return;
            }

            // Route standard clicks directly to the new History function
            processCurrentSelection();
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