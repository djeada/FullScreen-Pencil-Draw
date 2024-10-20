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

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent), scene(new QGraphicsScene(this)),
      tempShapeItem(nullptr), currentShape(Line), currentPen(Qt::white, 3),
      eraserPen(Qt::black, 10), currentPath(nullptr),
      backgroundColor(Qt::black), eraserPreview(nullptr) {

  this->setScene(scene);
  this->setRenderHint(QPainter::Antialiasing);
  scene->setSceneRect(0, 0, 800, 600);

  scene->setBackgroundBrush(backgroundColor);

  eraserPen.setColor(backgroundColor);

  eraserPreview = scene->addEllipse(0, 0, eraserPen.width(), eraserPen.width(),
                                    QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview->setZValue(1);
  eraserPreview->hide();

  this->setMouseTracking(true);

  scene->setItemIndexMethod(QGraphicsScene::NoIndex);

  connect(scene, &QGraphicsScene::selectionChanged, this, [this]() {});
}

Canvas::~Canvas() {}

void Canvas::setShape(const QString &shapeType) {
  if (shapeType == "Line") {
    currentShape = Line;
    this->setDragMode(QGraphicsView::NoDrag);
  } else if (shapeType == "Rectangle") {
    currentShape = Rectangle;
    this->setDragMode(QGraphicsView::NoDrag);
  } else if (shapeType == "Circle") {
    currentShape = Circle;
    this->setDragMode(QGraphicsView::NoDrag);
  } else if (shapeType == "Selection") {
    currentShape = Selection;
    this->setDragMode(QGraphicsView::RubberBandDrag);
  }
  tempShapeItem = nullptr;

  if (currentShape != Eraser) {
    hideEraserPreview();
  }

  if (currentShape != Selection) {
    scene->clearSelection();
  }
}

void Canvas::setPenTool() {
  currentShape = Pen;
  tempShapeItem = nullptr;

  this->setDragMode(QGraphicsView::NoDrag);

  hideEraserPreview();

  scene->clearSelection();
}

void Canvas::setEraserTool() {
  currentShape = Eraser;
  tempShapeItem = nullptr;

  eraserPen.setColor(backgroundColor);

  this->setDragMode(QGraphicsView::NoDrag);

  hideEraserPreview();

  scene->clearSelection();
}

void Canvas::setPenColor(const QColor &color) { currentPen.setColor(color); }

void Canvas::increaseBrushSize() {
  int size = currentPen.width();
  if (size < MAX_BRUSH_SIZE) {
    currentPen.setWidth(size + 2);
    eraserPen.setWidth(eraserPen.width() + 2);
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
    currentPen.setWidth(size - 2);
    eraserPen.setWidth(eraserPen.width() - 2);
    if (currentShape == Eraser) {
      eraserPreview->setRect(eraserPreview->rect().x(),
                             eraserPreview->rect().y(), eraserPen.width(),
                             eraserPen.width());
    }
  }
}

void Canvas::clearCanvas() {
  scene->clear();
  undoStack.clear();
  redoStack.clear();

  scene->setBackgroundBrush(backgroundColor);

  eraserPreview = scene->addEllipse(0, 0, eraserPen.width(), eraserPen.width(),
                                    QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview->setZValue(1);
  eraserPreview->hide();
}

void Canvas::undoLastAction() {
  if (!undoStack.isEmpty()) {
    Action *lastAction = undoStack.takeLast();
    lastAction->undo();
    redoStack.append(lastAction);
  }
}

void Canvas::redoLastAction() {
  if (!redoStack.isEmpty()) {
    Action *nextAction = redoStack.takeLast();
    nextAction->redo();
    undoStack.append(nextAction);
  }
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  QPointF scenePos = mapToScene(event->pos());

  if (currentShape == Selection) {
    QGraphicsView::mousePressEvent(event);
    return;
  }

  startPoint = scenePos;

  switch (currentShape) {
  case Eraser: {
    eraseAt(scenePos);
    break;
  }
  case Pen: {
    currentPath = new QGraphicsPathItem();
    currentPath->setPen(currentPen);
    currentPath->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
    QPainterPath p;
    p.moveTo(scenePos);
    currentPath->setPath(p);
    scene->addItem(currentPath);

    pointBuffer.clear();
    pointBuffer.append(scenePos);

    DrawAction *action = new DrawAction(currentPath);
    undoStack.append(action);
    redoStack.clear();
    break;
  }
  case Rectangle: {
    QGraphicsRectItem *rectItem =
        new QGraphicsRectItem(QRectF(startPoint, startPoint));
    rectItem->setPen(currentPen);
    rectItem->setFlags(QGraphicsItem::ItemIsSelectable |
                       QGraphicsItem::ItemIsMovable);
    scene->addItem(rectItem);
    tempShapeItem = rectItem;

    DrawAction *action = new DrawAction(rectItem);
    undoStack.append(action);
    redoStack.clear();
    break;
  }
  case Circle: {
    QGraphicsEllipseItem *ellipseItem =
        new QGraphicsEllipseItem(QRectF(startPoint, startPoint));
    ellipseItem->setPen(currentPen);
    ellipseItem->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
    scene->addItem(ellipseItem);
    tempShapeItem = ellipseItem;

    DrawAction *action = new DrawAction(ellipseItem);
    undoStack.append(action);
    redoStack.clear();
    break;
  }
  case Line: {
    QGraphicsLineItem *lineItem =
        new QGraphicsLineItem(QLineF(startPoint, startPoint));
    lineItem->setPen(currentPen);
    lineItem->setFlags(QGraphicsItem::ItemIsSelectable |
                       QGraphicsItem::ItemIsMovable);
    scene->addItem(lineItem);
    tempShapeItem = lineItem;

    DrawAction *action = new DrawAction(lineItem);
    undoStack.append(action);
    redoStack.clear();
    break;
  }
  default:
    QGraphicsView::mousePressEvent(event);
    break;
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  QPointF currentPoint = mapToScene(event->pos());

  if (currentShape == Selection) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  switch (currentShape) {
  case Pen:
    addPoint(currentPoint);
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

  if (currentShape == Eraser) {
    QPointF scenePos = mapToScene(event->pos());
    updateEraserPreview(scenePos);
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  if (currentShape == Selection) {
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }

  Q_UNUSED(event);
  if (currentShape != Pen && currentShape != Eraser && tempShapeItem) {
    tempShapeItem = nullptr;
  } else if (currentShape == Pen) {
    currentPath = nullptr;
    pointBuffer.clear();
  }
}

void Canvas::updateEraserPreview(const QPointF &position) {
  if (!eraserPreview)
    return;

  qreal radius = eraserPen.width() / 2.0;
  eraserPreview->setRect(position.x() - radius, position.y() - radius,
                         eraserPen.width(), eraserPen.width());

  if (!eraserPreview->isVisible()) {
    eraserPreview->show();
  }
}

void Canvas::hideEraserPreview() {
  if (eraserPreview) {
    eraserPreview->hide();
  }
}

void Canvas::copySelectedItems() {
  QList<QGraphicsItem *> selectedItems = scene->selectedItems();
  if (selectedItems.isEmpty())
    return;

  QMimeData *mimeData = new QMimeData();

  QByteArray byteArray;
  QDataStream dataStream(&byteArray, QIODevice::WriteOnly);

  for (QGraphicsItem *item : selectedItems) {
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
  }

  mimeData->setData("application/x-canvas-items", byteArray);

  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setMimeData(mimeData);
}

void Canvas::cutSelectedItems() {
  QList<QGraphicsItem *> selectedItems = scene->selectedItems();
  if (selectedItems.isEmpty())
    return;

  copySelectedItems();

  for (QGraphicsItem *item : selectedItems) {
    DeleteAction *action = new DeleteAction(item);
    undoStack.append(action);
    redoStack.clear();
    scene->removeItem(item);
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

        QGraphicsRectItem *newRect = new QGraphicsRectItem(rect);
        newRect->setPen(pen);
        newRect->setBrush(brush);
        newRect->setPos(pos + QPointF(10, 10));
        newRect->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
        scene->addItem(newRect);
        pastedItems.append(newRect);

        DrawAction *action = new DrawAction(newRect);
        undoStack.append(action);
        redoStack.clear();
      } else if (itemType == "Ellipse") {
        QRectF rect;
        QPointF pos;
        QPen pen;
        QBrush brush;
        dataStream >> rect >> pos >> pen >> brush;

        QGraphicsEllipseItem *newEllipse = new QGraphicsEllipseItem(rect);
        newEllipse->setPen(pen);
        newEllipse->setBrush(brush);
        newEllipse->setPos(pos + QPointF(10, 10));
        newEllipse->setFlags(QGraphicsItem::ItemIsSelectable |
                             QGraphicsItem::ItemIsMovable);
        scene->addItem(newEllipse);
        pastedItems.append(newEllipse);

        DrawAction *action = new DrawAction(newEllipse);
        undoStack.append(action);
        redoStack.clear();
      } else if (itemType == "Line") {
        QLineF line;
        QPointF pos;
        QPen pen;
        dataStream >> line >> pos >> pen;

        QGraphicsLineItem *newLine = new QGraphicsLineItem(line);
        newLine->setPen(pen);
        newLine->setPos(pos + QPointF(10, 10));
        newLine->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
        scene->addItem(newLine);
        pastedItems.append(newLine);

        DrawAction *action = new DrawAction(newLine);
        undoStack.append(action);
        redoStack.clear();
      } else if (itemType == "Path") {
        QPainterPath path;
        QPointF pos;
        QPen pen;
        dataStream >> path >> pos >> pen;

        QGraphicsPathItem *newPath = new QGraphicsPathItem(path);
        newPath->setPen(pen);
        newPath->setPos(pos + QPointF(10, 10));
        newPath->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
        scene->addItem(newPath);
        pastedItems.append(newPath);

        DrawAction *action = new DrawAction(newPath);
        undoStack.append(action);
        redoStack.clear();
      }
    }

    for (QGraphicsItem *item : pastedItems) {
      item->setSelected(true);
    }
  }
}

void Canvas::addPoint(const QPointF &point) {
  if (!currentPath)
    return;

  pointBuffer.append(point);

  const int minPointsRequired = 4;

  if (pointBuffer.size() >= minPointsRequired) {
    QPointF p0 = pointBuffer.at(pointBuffer.size() - minPointsRequired);
    QPointF p1 = pointBuffer.at(pointBuffer.size() - minPointsRequired + 1);
    QPointF p2 = pointBuffer.at(pointBuffer.size() - minPointsRequired + 2);
    QPointF p3 = pointBuffer.at(pointBuffer.size() - minPointsRequired + 3);

    QPointF controlPoint1 = p1 + (p2 - p0) / 6.0;
    QPointF controlPoint2 = p2 - (p3 - p1) / 6.0;

    QPainterPath path = currentPath->path();
    path.cubicTo(controlPoint1, controlPoint2, p2);
    currentPath->setPath(path);
  }

  if (pointBuffer.size() > minPointsRequired) {
    pointBuffer.removeFirst();
  }
}

void Canvas::eraseAt(const QPointF &point) {
  qreal eraserSize = eraserPen.width();
  QRectF eraserRect(point.x() - eraserSize / 2, point.y() - eraserSize / 2,
                    eraserSize, eraserSize);
  QList<QGraphicsItem *> itemsToErase = scene->items(eraserRect);
  for (QGraphicsItem *item : itemsToErase) {
    if (item != eraserPreview) {
      DeleteAction *action = new DeleteAction(item);
      undoStack.append(action);
      redoStack.clear();
      scene->removeItem(item);
    }
  }
}
