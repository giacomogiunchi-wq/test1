#pragma once
#include <QPoint>
#include <QWidget>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
class OcctViewport final : public QWidget {
  Q_OBJECT
 public:
  explicit OcctViewport(QWidget* parent = nullptr);
 signals:
  void topologySelected(const QString& type);
 protected:
  QPaintEngine* paintEngine() const override { return nullptr; }
  void paintEvent(QPaintEvent*) override;
  void resizeEvent(QResizeEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
  void mouseMoveEvent(QMouseEvent*) override;
  void mouseReleaseEvent(QMouseEvent*) override;
  void wheelEvent(QWheelEvent*) override;
 private:
  void initialize();
  void reportSelection();
  Handle(V3d_Viewer) viewer_;
  Handle(V3d_View) view_;
  Handle(AIS_InteractiveContext) context_;
  QPoint previous_;
  Qt::MouseButton active_button_{Qt::NoButton};
};
