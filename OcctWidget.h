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

// ADD THIS TO KILL THE X11 MACRO CLASH
#ifdef None
#undef None
#endif

class QTextStream;

class OcctWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OcctWidget(QWidget *parent = nullptr);
    ~OcctWidget() override;

    void loadStepFile(const std::string& filePath);
    void setSelectionMode(int mode);
    void saveSelectionToFile(const QString& filename);

    void enableOriginSelectionMode();
    void resetOrigin(); // NEW: Function to snap back to default

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
    gp_Pnt myDefaultOrigin{0.0, 0.0, 0.0}; // NEW: Stores the original Center of Mass
    Handle(AIS_Trihedron) myOriginMarker;

    void initOCCT();

    void processEdge(const TopoDS_Edge& edge, QTextStream& out, double resolution);
    void processWire(const TopoDS_Wire& wire, QTextStream& out, double resolution);
    void processFace(const TopoDS_Face& face, QTextStream& out, double resolution);
};

#endif // OCCTWIDGET_H