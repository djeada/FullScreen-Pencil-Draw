// canvas.cpp
#include "canvas.h"
#include <QColorDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QMouseEvent>

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent), scene(new QGraphicsScene(this)),
      tempShapeItem(nullptr), currentShape(Line),
      currentPen(Qt::white, 3), // Default drawing color: white
      eraserPen(Qt::black, 10), // Eraser color will match background
      currentPath(nullptr),
      backgroundColor(Qt::black), // Set background color: black
      eraserPreview(nullptr) {

  this->setScene(scene);
  this->setRenderHint(QPainter::Antialiasing);
  scene->setSceneRect(0, 0, 800, 600); // Set canvas size

  // Set the scene background to backgroundColor
  scene->setBackgroundBrush(backgroundColor);

  // Configure the eraserPen to match the background color
  eraserPen.setColor(backgroundColor);

  // Initialize the eraser preview circle
  eraserPreview = scene->addEllipse(0, 0, eraserPen.width(), eraserPen.width(),
                                    QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview->setZValue(1); // Ensure it's on top
  eraserPreview->hide();       // Hide initially

  // Enable mouse tracking to capture mouse move events without pressing buttons
  this->setMouseTracking(true);
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
  tempShapeItem = nullptr; // Reset temporary shape

  // Hide eraser preview if not in eraser mode
  if (currentShape != Eraser) {
    hideEraserPreview();
  }
}

void Canvas::setPenTool() {
  currentShape = Pen; // Set to freehand drawing mode
  tempShapeItem = nullptr;

  // Ensure the pen color is white when the pen tool is selected
  currentPen.setColor(Qt::white);

  // Hide eraser preview
  hideEraserPreview();
}

void Canvas::setEraserTool() {
  currentShape = Eraser; // Set to eraser mode
  tempShapeItem = nullptr;

  // Set eraser pen color to background color
  eraserPen.setColor(backgroundColor);

  // Show eraser preview
  // Initial position will be set in mouseMoveEvent
}

void Canvas::setPenColor(const QColor &color) {
  currentPen.setColor(color); // Set pen color
}

void Canvas::increaseBrushSize() {
  int size = currentPen.width();
  if (size < MAX_BRUSH_SIZE) {
    currentPen.setWidth(size + 2); // Increase brush size
    eraserPen.setWidth(eraserPen.width() +
                       2); // Optionally increase eraser size
    if (currentShape == Eraser) {
      eraserPreview->setRect(eraserPreview->rect().x(),
                             eraserPreview->rect().y(), eraserPen.width(),
                             eraserPen.width());
    }
  }
}

void Canvas::decreaseBrushSize() {
  int size = currentPen.width();
  if (size > MIN_BRUSH_SIZE) {
    currentPen.setWidth(size - 2); // Decrease brush size
    eraserPen.setWidth(eraserPen.width() -
                       2); // Optionally decrease eraser size
    if (currentShape == Eraser) {
      eraserPreview->setRect(eraserPreview->rect().x(),
                             eraserPreview->rect().y(), eraserPen.width(),
                             eraserPen.width());
    }
  }
}

void Canvas::clearCanvas() {
  scene->clear();     // Clear the entire canvas
  itemsStack.clear(); // Clear the undo stack

  // Reset the background to backgroundColor after clearing
  scene->setBackgroundBrush(backgroundColor);

  // Re-add the eraser preview
  eraserPreview = scene->addEllipse(0, 0, eraserPen.width(), eraserPen.width(),
                                    QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview->setZValue(1); // Ensure it's on top
  eraserPreview->hide();       // Hide initially
}

void Canvas::undoLastAction() {
  if (!itemsStack.isEmpty()) {
    QGraphicsItem *lastItem = itemsStack.takeLast(); // Remove last item
    scene->removeItem(lastItem);                     // Remove it from the scene
    delete lastItem;                                 // Free memory
  }
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  startPoint = mapToScene(event->pos()); // Capture the start point of the shape

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
    itemsStack.append(currentPath); // Add to undo stack
    break;
  }
  case Rectangle:
    tempShapeItem = scene->addRect(QRectF(startPoint, startPoint), currentPen);
    break;
  case Circle:
    tempShapeItem =
        scene->addEllipse(QRectF(startPoint, startPoint), currentPen);
    break;
  case Line:
    tempShapeItem = scene->addLine(QLineF(startPoint, startPoint), currentPen);
    break;
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  QPointF currentPoint = mapToScene(event->pos()); // Track mouse movement

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
      QGraphicsRectItem *rectItem =
          static_cast<QGraphicsRectItem *>(tempShapeItem);
      rectItem->setRect(QRectF(startPoint, currentPoint).normalized());
    }
    break;
  case Circle:
    if (tempShapeItem) {
      QGraphicsEllipseItem *ellipseItem =
          static_cast<QGraphicsEllipseItem *>(tempShapeItem);
      ellipseItem->setRect(QRectF(startPoint, currentPoint).normalized());
    }
    break;
  case Line:
    if (tempShapeItem) {
      QGraphicsLineItem *lineItem =
          static_cast<QGraphicsLineItem *>(tempShapeItem);
      lineItem->setLine(QLineF(startPoint, currentPoint));
    }
    break;
  }

  // Update eraser preview position if in Eraser mode
  if (currentShape == Eraser) {
    QPointF scenePos = mapToScene(event->pos());
    updateEraserPreview(scenePos);
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  Q_UNUSED(event);
  if (currentShape != Pen && currentShape != Eraser && tempShapeItem) {
    itemsStack.append(tempShapeItem); // Add shape to undo stack
    tempShapeItem = nullptr;          // Clear temporary item
  } else if (currentShape == Pen || currentShape == Eraser) {
    currentPath = nullptr; // Reset current path
  }
}

void Canvas::updateEraserPreview(const QPointF &position) {
  if (!eraserPreview)
    return;

  // Center the preview circle on the cursor
  qreal radius = eraserPen.width() / 2.0;
  eraserPreview->setRect(position.x() - radius, position.y() - radius,
                         eraserPen.width(), eraserPen.width());

  // Show the preview if it's not already visible
  if (!eraserPreview->isVisible()) {
    eraserPreview->show();
  }
}

void Canvas::hideEraserPreview() {
  if (eraserPreview) {
    eraserPreview->hide();
  }
}
