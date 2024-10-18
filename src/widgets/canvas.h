#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPen>
#include <QGraphicsPathItem>
#include <QList>

class Canvas : public QGraphicsView {
  Q_OBJECT

public:
  explicit Canvas(QWidget *parent = nullptr);
  ~Canvas();

public slots:
  // Shape drawing and tool functions
  void setShape(const QString &shapeType);  // Set shape tool (Line, Circle, Rectangle)
  void setPenTool();                        // Set freehand drawing tool
  void setEraserTool();                     // Set eraser tool
  void setPenColor(const QColor &color);    // Set color for pen or shape
  void increaseBrushSize();                 // Increase brush size for pen/eraser
  void decreaseBrushSize();                 // Decrease brush size for pen/eraser
  void clearCanvas();                       // Clear the entire canvas
  void undoLastAction();                    // Undo last action (remove last drawn item)

protected:
  void mousePressEvent(QMouseEvent *event) override;    // Handle mouse press for starting shapes
  void mouseMoveEvent(QMouseEvent *event) override;     // Handle mouse movement for drawing shapes
  void mouseReleaseEvent(QMouseEvent *event) override;  // Handle mouse release for finalizing shapes

private:
  enum ShapeType { Line, Rectangle, Circle, Pen, Eraser };

  QGraphicsScene *scene;            // The graphics scene to draw on
  QPen currentPen;                  // Current pen settings
  QPen eraserPen;                   // Eraser pen settings
  ShapeType currentShape;           // The current shape being drawn
  QPointF startPoint;               // The point where the mouse was first pressed (shape start)
  QGraphicsItem *tempShapeItem;     // Temporary item for drawing shapes (until mouse release)
  QGraphicsPathItem *currentPath;   // Path for freehand drawing (Pen tool)
  QList<QGraphicsItem *> itemsStack;  // Stack of items for undo functionality

  const int MAX_BRUSH_SIZE = 30;  // Maximum brush size
  const int MIN_BRUSH_SIZE = 1;   // Minimum brush size
};

#endif  // CANVAS_H
