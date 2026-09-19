#include "occt_viewport.hpp"
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <QMouseEvent>
#include <QWheelEvent>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

OcctViewport::OcctViewport(QWidget* parent) : QWidget(parent) {
  setAttribute(Qt::WA_NativeWindow); setAttribute(Qt::WA_PaintOnScreen); setMouseTracking(true); setFocusPolicy(Qt::StrongFocus);
  initialize();
}
void OcctViewport::initialize() {
  const Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection();
  const Handle(Graphic3d_GraphicDriver) driver = new OpenGl_GraphicDriver(display);
  viewer_ = new V3d_Viewer(driver); viewer_->SetDefaultLights(); viewer_->SetLightOn();
  context_ = new AIS_InteractiveContext(viewer_); view_ = viewer_->CreateView();
  Handle(Aspect_NeutralWindow) window = new Aspect_NeutralWindow();
  window->SetNativeHandle(static_cast<Aspect_Drawable>(winId())); window->SetSize(width(), height());
  view_->SetWindow(window); if (!window->IsMapped()) window->Map();
  const Handle(AIS_Shape) box = new AIS_Shape(BRepPrimAPI_MakeBox(100.0, 80.0, 60.0).Shape());
  context_->Display(box, false); context_->Activate(box, AIS_Shape::SelectionMode(TopAbs_FACE)); context_->Activate(box, AIS_Shape::SelectionMode(TopAbs_EDGE));
  view_->SetBackgroundColor(Quantity_NOC_GRAY20); view_->FitAll(); view_->MustBeResized();
}
void OcctViewport::paintEvent(QPaintEvent*) { if (!view_.IsNull()) view_->Redraw(); }
void OcctViewport::resizeEvent(QResizeEvent*) { if (!view_.IsNull()) view_->MustBeResized(); }
void OcctViewport::mousePressEvent(QMouseEvent* event) { previous_=event->position().toPoint(); active_button_=event->button(); if(active_button_==Qt::LeftButton && (event->modifiers()&Qt::ShiftModifier)) view_->StartRotation(previous_.x(),previous_.y()); }
void OcctViewport::mouseMoveEvent(QMouseEvent* event) { const QPoint now=event->position().toPoint(); if(active_button_==Qt::LeftButton && (event->modifiers()&Qt::ShiftModifier)) view_->Rotation(now.x(),now.y()); else if(active_button_==Qt::MiddleButton) view_->Pan(now.x()-previous_.x(),previous_.y()-now.y()); else context_->MoveTo(now.x(),now.y(),view_,true); previous_=now; }
void OcctViewport::mouseReleaseEvent(QMouseEvent* event) { if(active_button_==Qt::LeftButton && !(event->modifiers()&Qt::ShiftModifier)) { context_->SelectDetected(AIS_SelectionScheme_Replace); reportSelection(); } active_button_=Qt::NoButton; }
void OcctViewport::wheelEvent(QWheelEvent* event) { const QPoint p=event->position().toPoint(); const int delta=event->angleDelta().y(); view_->Zoom(p.x(),p.y(),p.x(),p.y()+(delta>0?20:-20)); }
void OcctViewport::reportSelection() { context_->InitSelected(); if(!context_->MoreSelected()){ emit topologySelected("None"); return; } const TopoDS_Shape shape=context_->SelectedShape(); QString name="Shape"; if(shape.ShapeType()==TopAbs_FACE) name="Face"; else if(shape.ShapeType()==TopAbs_EDGE) name="Edge"; emit topologySelected(name); }
