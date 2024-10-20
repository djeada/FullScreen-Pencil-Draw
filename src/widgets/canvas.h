// canvas.h
#ifndef CANVAS_H
#define CANVAS_H

#include "../core/action.h"
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

class Canvas : public QGraphicsView {
  Q_OBJECT

public:
  explicit Canvas(QWidget *parent = nullptr);
  ~Canvas();

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

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  enum ShapeType { Line, Rectangle, Circle, Pen, Eraser, Selection };

  QGraphicsScene *scene;
  QPen currentPen;
  QPen eraserPen;
  ShapeType currentShape;
  QPointF startPoint;
  QGraphicsItem *tempShapeItem;
  QGraphicsPathItem *currentPath;
  QColor backgroundColor;
  QGraphicsEllipseItem *eraserPreview;
  const int MAX_BRUSH_SIZE = 150;
  const int MIN_BRUSH_SIZE = 1;
  QVector<QPointF> pointBuffer;
  const int smoothingFactor = 5;
  QPointF previousPoint;
  void updateEraserPreview(const QPointF &position);
  void hideEraserPreview();
  void addPoint(const QPointF &point);
  void eraseAt(const QPointF &point);
  QList<Action *> undoStack;
  QList<Action *> redoStack;
};

#endif // CANVAS_H
