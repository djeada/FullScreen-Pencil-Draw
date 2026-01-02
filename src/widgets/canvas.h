// canvas.h
#ifndef CANVAS_H
#define CANVAS_H

#include <QApplication>
#include <QClipboard>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QMimeData>
#include <QMouseEvent>
#include <QPen>
#include <QVector>
#include <QWheelEvent>

#include "../core/action.h"

class Canvas : public QGraphicsView {
  Q_OBJECT

public:
  explicit Canvas(QWidget *parent = nullptr);
  ~Canvas();

  int getCurrentBrushSize() const;
  QColor getCurrentColor() const;

signals:
  void brushSizeChanged(int size);
  void colorChanged(const QColor &color);

public slots:
  void setShape(const QString &shapeType);
  void setPenTool();
  void setEraserTool();
  void setPenColor(const QColor &color);
  void increaseBrushSize();
  void decreaseBrushSize();
  void clearCanvas();
  void undoLastAction();
  void redoLastAction();
  void copySelectedItems();
  void cutSelectedItems();
  void pasteItems();
  void zoomIn();
  void zoomOut();
  void saveToFile();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  // Enums
  enum ShapeType { Line, Rectangle, Circle, Pen, Eraser, Selection };

  // Member variables
  QGraphicsScene *scene;
  QPen currentPen;
  QPen eraserPen;
  ShapeType currentShape;
  QPointF startPoint;
  QGraphicsItem *tempShapeItem;
  QGraphicsPathItem *currentPath;
  QColor backgroundColor;
  QGraphicsEllipseItem *eraserPreview;

  static constexpr int MAX_BRUSH_SIZE = 150;
  static constexpr int MIN_BRUSH_SIZE = 1;
  static constexpr int BRUSH_SIZE_STEP = 2;
  static constexpr double ZOOM_FACTOR = 1.15;
  static constexpr double MAX_ZOOM = 10.0;
  static constexpr double MIN_ZOOM = 0.1;

  double currentZoom = 1.0;

  QVector<QPointF> pointBuffer;
  QPointF previousPoint;

  QList<Action *> undoStack;
  QList<Action *> redoStack;

  // Private methods
  void updateEraserPreview(const QPointF &position);
  void hideEraserPreview();
  void addPoint(const QPointF &point);
  void eraseAt(const QPointF &point);
  void applyZoom(double factor);
};

#endif // CANVAS_H
