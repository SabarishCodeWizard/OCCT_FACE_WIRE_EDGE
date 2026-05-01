#ifndef OCCTWIDGET_H
#define OCCTWIDGET_H

#include <QWidget>
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_CompCurve.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <BRep_Tool.hxx>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>

#include <AIS_Trihedron.hxx>
#include <Geom_Axis2Placement.hxx>

// --- ADDED THIS LINE TO FIX THE ERROR ---
#include <AIS_Shape.hxx>
// ----------------------------------------

#include <vector>

// ADD THIS TO KILL THE X11 MACRO CLASH
#ifdef None
#undef None
#endif

class QTextStream;

// Data structure to remember everything about a click
struct PathData {
    TopoDS_Shape shape;
    Handle(AIS_Shape) visualRedPath;
    double resolution;
};

class OcctWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OcctWidget(QWidget *parent = nullptr);
    ~OcctWidget() override;

    void loadStepFile(const std::string& filePath);
    void setSelectionMode(int mode);

    void enableOriginSelectionMode();
    void resetOrigin();

    // Public slots to trigger from MainWindow buttons
    void undoSelection();
    void redoSelection();
    void clearSelections();

signals:
    // Broadcasts messages down to the Status Bar
    void statusUpdate(const QString& msg);

protected:
    QPaintEngine* paintEngine() const override { return nullptr; }

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    Handle(V3d_Viewer) myViewer;
    Handle(V3d_View) myView;
    Handle(AIS_InteractiveContext) myContext;
    Handle(OpenGl_GraphicDriver) myGraphicDriver;

    QPoint myLastMousePos;

    bool myIsSettingOriginMode = false;
    gp_Pnt myCustomOrigin{0.0, 0.0, 0.0};
    gp_Pnt myDefaultOrigin{0.0, 0.0, 0.0};
    Handle(AIS_Trihedron) myOriginMarker;

    // Variables to manage History and the CSV
    std::vector<PathData> myPathHistory;
    std::vector<PathData> myRedoStack;
    QString myCSVPath = "/home/sabarish/Downloads/robot_path.csv"; // Setup your output path here!

    void initOCCT();

    // Centralized file writer
    void processCurrentSelection();
    void regenerateCSV();

    void processEdge(const TopoDS_Edge& edge, QTextStream& out, double resolution);
    void processWire(const TopoDS_Wire& wire, QTextStream& out, double resolution);
    void processFace(const TopoDS_Face& face, QTextStream& out, double resolution);
};

#endif // OCCTWIDGET_H