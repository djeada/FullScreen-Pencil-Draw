// fill_tool.cpp
#include "fill_tool.h"
#include "../widgets/canvas.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>

FillTool::FillTool(Canvas *canvas) : Tool(canvas) {}

FillTool::~FillTool() = default;

void FillTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->button() == Qt::LeftButton) {
    fillAt(scenePos);
  }
}

void FillTool::mouseMoveEvent(QMouseEvent * /*event*/,
                               const QPointF & /*scenePos*/) {
  // Nothing to do on move
}

void FillTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                  const QPointF & /*scenePos*/) {
  // Nothing to do on release
}

void FillTool::fillAt(const QPointF &point) {
  QGraphicsScene *scene = canvas_->scene();
  QColor fillColor = canvas_->currentPen().color();

  for (QGraphicsItem *item : scene->items(point)) {
    // Skip background image
    if (item == canvas_->backgroundImageItem())
      continue;

    // Fill rectangles
    if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item)) {
      rect->setBrush(fillColor);
      return;
    }

    // Fill ellipses
    if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      ellipse->setBrush(fillColor);
      return;
    }

    // Fill polygons
    if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item)) {
      polygon->setBrush(fillColor);
      return;
    }
  }
}
