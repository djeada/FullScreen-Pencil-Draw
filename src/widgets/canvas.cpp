// canvas.cpp
#include "canvas.h"
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QMimeData>
#include <QMouseEvent>

// Constructor and other methods remain unchanged up to setShape

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

  // Enable item selection by default
  scene->setItemIndexMethod(QGraphicsScene::NoIndex);
  // Removed: scene->setSelectionArea(QPainterPath());

  // Connect selectionChanged signal to handle any additional logic if needed
  connect(scene, &QGraphicsScene::selectionChanged, this, [this]() {
    // Optional: Add any additional logic when selection changes
  });
}

Canvas::~Canvas() {
  // No need to delete scene; it will be deleted automatically
}

void Canvas::setShape(const QString &shapeType) {
  if (shapeType == "Line") {
    currentShape = Line;
    this->setDragMode(QGraphicsView::NoDrag); // Disable drag for drawing
  } else if (shapeType == "Rectangle") {
    currentShape = Rectangle;
    this->setDragMode(QGraphicsView::NoDrag); // Disable drag for drawing
  } else if (shapeType == "Circle") {
    currentShape = Circle;
    this->setDragMode(QGraphicsView::NoDrag); // Disable drag for drawing
  }
  // Handle Selection Tool
  else if (shapeType == "Selection") {
    currentShape = Selection;
    this->setDragMode(
        QGraphicsView::RubberBandDrag); // Enable selection drag mode
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

  // Disable selection drag mode
  this->setDragMode(QGraphicsView::NoDrag);

  // Hide eraser preview
  hideEraserPreview();
}

void Canvas::setEraserTool() {
  currentShape = Eraser; // Set to eraser mode
  tempShapeItem = nullptr;

  // Set eraser pen color to background color
  eraserPen.setColor(backgroundColor);

  // Disable selection drag mode
  this->setDragMode(QGraphicsView::NoDrag);

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
  QPointF scenePos = mapToScene(event->pos());

  if (currentShape == Selection) {
    // Let QGraphicsView handle selection
    QGraphicsView::mousePressEvent(event);
    return;
  }

  startPoint = scenePos; // Capture the start point of the shape

  switch (currentShape) {
  case Pen:
  case Eraser: {
    currentPath = new QGraphicsPathItem();
    if (currentShape == Eraser) {
      currentPath->setPen(eraserPen);
    } else {
      currentPath->setPen(currentPen);
    }
    // Make the path selectable and movable
    currentPath->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
    QPainterPath p;
    p.moveTo(startPoint);
    currentPath->setPath(p);
    scene->addItem(currentPath);    // Add to the scene
    itemsStack.append(currentPath); // Add to undo stack
    break;
  }
  case Rectangle: {
    QGraphicsRectItem *rectItem =
        scene->addRect(QRectF(startPoint, startPoint), currentPen);
    rectItem->setFlags(QGraphicsItem::ItemIsSelectable |
                       QGraphicsItem::ItemIsMovable);
    tempShapeItem = rectItem;
    break;
  }
  case Circle: {
    QGraphicsEllipseItem *ellipseItem =
        scene->addEllipse(QRectF(startPoint, startPoint), currentPen);
    ellipseItem->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
    tempShapeItem = ellipseItem;
    break;
  }
  case Line: {
    QGraphicsLineItem *lineItem =
        scene->addLine(QLineF(startPoint, startPoint), currentPen);
    lineItem->setFlags(QGraphicsItem::ItemIsSelectable |
                       QGraphicsItem::ItemIsMovable);
    tempShapeItem = lineItem;
    break;
  }
  default:
    QGraphicsView::mousePressEvent(event);
    break;
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  QPointF currentPoint = mapToScene(event->pos()); // Track mouse movement

  if (currentShape == Selection) {
    // Let QGraphicsView handle selection
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

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
  default:
    QGraphicsView::mouseMoveEvent(event);
    break;
  }

  // Update eraser preview position if in Eraser mode
  if (currentShape == Eraser) {
    QPointF scenePos = mapToScene(event->pos());
    updateEraserPreview(scenePos);
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  if (currentShape == Selection) {
    // Let QGraphicsView handle selection
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }

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

// ----------------- New Methods for Copy, Cut, Paste -----------------

void Canvas::copySelectedItems() {
  QList<QGraphicsItem *> selectedItems = scene->selectedItems();
  if (selectedItems.isEmpty())
    return;

  // Create a QMimeData object to store the serialized items
  QMimeData *mimeData = new QMimeData();

  // Serialize the selected items
  QByteArray byteArray;
  QDataStream dataStream(&byteArray, QIODevice::WriteOnly);

  for (QGraphicsItem *item : selectedItems) {
    // For simplicity, handle only specific item types
    if (auto rectItem = dynamic_cast<QGraphicsRectItem *>(item)) {
      dataStream << QString("Rectangle");
      QRectF rect = rectItem->rect();
      QPointF pos = rectItem->pos();
      QPen pen = rectItem->pen();
      QBrush brush = rectItem->brush();
      dataStream << rect << pos << pen << brush;
    } else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      dataStream << QString("Ellipse");
      QRectF rect = ellipseItem->rect();
      QPointF pos = ellipseItem->pos();
      QPen pen = ellipseItem->pen();
      QBrush brush = ellipseItem->brush();
      dataStream << rect << pos << pen << brush;
    } else if (auto lineItem = dynamic_cast<QGraphicsLineItem *>(item)) {
      dataStream << QString("Line");
      QLineF line = lineItem->line();
      QPointF pos = lineItem->pos();
      QPen pen = lineItem->pen();
      dataStream << line << pos << pen;
    } else if (auto pathItem = dynamic_cast<QGraphicsPathItem *>(item)) {
      dataStream << QString("Path");
      QPainterPath path = pathItem->path();
      QPointF pos = pathItem->pos();
      QPen pen = pathItem->pen();
      dataStream << path << pos << pen;
    }
    // Add more item types as needed
  }

  mimeData->setData("application/x-canvas-items", byteArray);

  // Set the mime data to the clipboard
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setMimeData(mimeData);
}

void Canvas::cutSelectedItems() {
  QList<QGraphicsItem *> selectedItems = scene->selectedItems();
  if (selectedItems.isEmpty())
    return;

  // First, copy the selected items
  copySelectedItems();

  // Then, remove the selected items from the scene
  for (QGraphicsItem *item : selectedItems) {
    itemsStack.removeAll(item); // Remove from undo stack if present
    scene->removeItem(item);
    delete item;
  }
}

void Canvas::pasteItems() {
  QClipboard *clipboard = QApplication::clipboard();
  const QMimeData *mimeData = clipboard->mimeData();

  if (mimeData->hasFormat("application/x-canvas-items")) {
    QByteArray byteArray = mimeData->data("application/x-canvas-items");
    QDataStream dataStream(&byteArray, QIODevice::ReadOnly);

    QList<QGraphicsItem *> pastedItems;

    while (!dataStream.atEnd()) {
      QString itemType;
      dataStream >> itemType;

      if (itemType == "Rectangle") {
        QRectF rect;
        QPointF pos;
        QPen pen;
        QBrush brush;
        dataStream >> rect >> pos >> pen >> brush;

        QGraphicsRectItem *newRect = scene->addRect(rect, pen, brush);
        newRect->setPos(pos + QPointF(10, 10)); // Offset to avoid overlap
        newRect->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
        itemsStack.append(newRect);
        pastedItems.append(newRect);
      } else if (itemType == "Ellipse") {
        QRectF rect;
        QPointF pos;
        QPen pen;
        QBrush brush;
        dataStream >> rect >> pos >> pen >> brush;

        QGraphicsEllipseItem *newEllipse = scene->addEllipse(rect, pen, brush);
        newEllipse->setPos(pos + QPointF(10, 10)); // Offset to avoid overlap
        newEllipse->setFlags(QGraphicsItem::ItemIsSelectable |
                             QGraphicsItem::ItemIsMovable);
        itemsStack.append(newEllipse);
        pastedItems.append(newEllipse);
      } else if (itemType == "Line") {
        QLineF line;
        QPointF pos;
        QPen pen;
        dataStream >> line >> pos >> pen;

        QGraphicsLineItem *newLine = scene->addLine(line, pen);
        newLine->setPos(pos + QPointF(10, 10)); // Offset to avoid overlap
        newLine->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
        itemsStack.append(newLine);
        pastedItems.append(newLine);
      } else if (itemType == "Path") {
        QPainterPath path;
        QPointF pos;
        QPen pen;
        dataStream >> path >> pos >> pen;

        QGraphicsPathItem *newPath = scene->addPath(path, pen);
        newPath->setPos(pos + QPointF(10, 10)); // Offset to avoid overlap
        newPath->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
        itemsStack.append(newPath);
        pastedItems.append(newPath);
      }
      // Add more item types as needed
    }

    // Select all pasted items
    for (QGraphicsItem *item : pastedItems) {
      item->setSelected(true);
    }
  }
}
