#include "canvas.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>

Canvas::Canvas(QWidget *parent) : QGraphicsView(parent), currentPen(Qt::black, 3) {
  scene = new QGraphicsScene(this);
  this->setScene(scene);
  this->setRenderHint(QPainter::Antialiasing);
  scene->setSceneRect(0, 0, 800, 600);
}

Canvas::~Canvas() { delete scene; }

void Canvas::setPenTool() {
  currentPen.setColor(Qt::black);
  currentPen.setWidth(3);
}

void Canvas::setEraserTool() {
  currentPen.setColor(Qt::white);
  currentPen.setWidth(10);
}

void Canvas::setPenColor(const QColor &color) {
  currentPen.setColor(color);
}

void Canvas::drawRectangle() {
  QGraphicsRectItem *rect = scene->addRect(50, 50, 100, 100, currentPen);
  itemsStack.append(rect);
}

void Canvas::drawCircle() {
  QGraphicsEllipseItem *ellipse = scene->addEllipse(50, 50, 100, 100, currentPen);
  itemsStack.append(ellipse);
}

void Canvas::drawLine() {
  QGraphicsLineItem *line = scene->addLine(50, 50, 200, 200, currentPen);
  itemsStack.append(line);
}

void Canvas::increaseBrushSize() {
  int size = currentPen.width();
  currentPen.setWidth(size + 2);
}

void Canvas::decreaseBrushSize() {
  int size = currentPen.width();
  if (size > 2) {
    currentPen.setWidth(size - 2);
  }
}

void Canvas::clearCanvas() {
  scene->clear();
  itemsStack.clear();
}

void Canvas::undoLastAction() {
  if (!itemsStack.isEmpty()) {
    QGraphicsItem *lastItem = itemsStack.takeLast();
    scene->removeItem(lastItem);
    delete lastItem;
  }
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    currentPath = new QGraphicsPathItem();
    currentPath->setPen(currentPen);
    QPainterPath p;
    p.moveTo(mapToScene(event->pos()));
    currentPath->setPath(p);
    scene->addItem(currentPath);
    itemsStack.append(currentPath);
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    QPainterPath p = currentPath->path();
    p.lineTo(mapToScene(event->pos()));
    currentPath->setPath(p);
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    // Additional logic on release if needed
  }
}
