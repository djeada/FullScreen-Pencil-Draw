#include "canvas.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QColorDialog>

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent),
      scene(new QGraphicsScene(this)),
      tempShapeItem(nullptr),
      currentShape(Line),
      currentPen(Qt::black, 3),
      eraserPen(Qt::white, 10),
      currentPath(nullptr) {
  this->setScene(scene);
  this->setRenderHint(QPainter::Antialiasing);
  scene->setSceneRect(0, 0, 800, 600);  // Set canvas size
}

Canvas::~Canvas() {
  // No need to delete scene; it will be deleted automatically
}

void Canvas::setShape(const QString &shapeType) {
  if (shapeType == "Line") {
    currentShape = Line;
  } else if (shapeType == "Rectangle") {
    currentShape = Rectangle;
  } else if (shapeType == "Circle") {
    currentShape = Circle;
  }
  tempShapeItem = nullptr;  // Reset temporary shape
}

void Canvas::setPenTool() {
  currentShape = Pen;  // Set to freehand drawing mode
  tempShapeItem = nullptr;
}

void Canvas::setEraserTool() {
  currentShape = Eraser;  // Set to eraser mode
  tempShapeItem = nullptr;
}

void Canvas::setPenColor(const QColor &color) {
  currentPen.setColor(color);  // Set pen color
}

void Canvas::increaseBrushSize() {
  int size = currentPen.width();
  if (size < MAX_BRUSH_SIZE) {
    currentPen.setWidth(size + 2);  // Increase brush size
  }
}

void Canvas::decreaseBrushSize() {
  int size = currentPen.width();
  if (size > MIN_BRUSH_SIZE) {
    currentPen.setWidth(size - 2);  // Decrease brush size
  }
}

void Canvas::clearCanvas() {
  scene->clear();       // Clear the entire canvas
  itemsStack.clear();   // Clear the undo stack
}

void Canvas::undoLastAction() {
  if (!itemsStack.isEmpty()) {
    QGraphicsItem *lastItem = itemsStack.takeLast();  // Remove last item
    scene->removeItem(lastItem);                      // Remove it from the scene
    delete lastItem;                                  // Free memory
  }
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  startPoint = mapToScene(event->pos());  // Capture the start point of the shape

  switch (currentShape) {
    case Pen:
    case Eraser: {
      currentPath = new QGraphicsPathItem();
      if (currentShape == Eraser) {
        currentPath->setPen(eraserPen);
      } else {
        currentPath->setPen(currentPen);
      }
      QPainterPath p;
      p.moveTo(startPoint);
      currentPath->setPath(p);
      scene->addItem(currentPath);    // Add to the scene
      itemsStack.append(currentPath);  // Add to undo stack
      break;
    }
    case Rectangle:
      tempShapeItem = scene->addRect(QRectF(startPoint, startPoint), currentPen);
      break;
    case Circle:
      tempShapeItem = scene->addEllipse(QRectF(startPoint, startPoint), currentPen);
      break;
    case Line:
      tempShapeItem = scene->addLine(QLineF(startPoint, startPoint), currentPen);
      break;
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  QPointF currentPoint = mapToScene(event->pos());  // Track mouse movement

  switch (currentShape) {
    case Pen:
    case Eraser:
      if (currentPath) {
        QPainterPath p = currentPath->path();
        p.lineTo(currentPoint);
        currentPath->setPath(p);
      }
      break;
    case Rectangle:
      if (tempShapeItem) {
        QGraphicsRectItem *rectItem = static_cast<QGraphicsRectItem *>(tempShapeItem);
        rectItem->setRect(QRectF(startPoint, currentPoint).normalized());
      }
      break;
    case Circle:
      if (tempShapeItem) {
        QGraphicsEllipseItem *ellipseItem = static_cast<QGraphicsEllipseItem *>(tempShapeItem);
        ellipseItem->setRect(QRectF(startPoint, currentPoint).normalized());
      }
      break;
    case Line:
      if (tempShapeItem) {
        QGraphicsLineItem *lineItem = static_cast<QGraphicsLineItem *>(tempShapeItem);
        lineItem->setLine(QLineF(startPoint, currentPoint));
      }
      break;
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  Q_UNUSED(event);
  if (currentShape != Pen && currentShape != Eraser && tempShapeItem) {
    itemsStack.append(tempShapeItem);  // Add shape to undo stack
    tempShapeItem = nullptr;           // Clear temporary item
  } else if (currentShape == Pen || currentShape == Eraser) {
    currentPath = nullptr;  // Reset current path
  }
}
