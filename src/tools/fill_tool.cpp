/**
 * @file fill_tool.cpp
 * @brief Fill tool implementation.
 */
#include "fill_tool.h"
#include "../core/action.h"
#include "../core/scene_renderer.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>

FillTool::FillTool(SceneRenderer *renderer) : Tool(renderer) {}

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
  QGraphicsScene *scene = renderer_->scene();
  QColor fillColor = renderer_->currentPen().color();
  QBrush newBrush(fillColor);

  for (QGraphicsItem *item : scene->items(point)) {
    // Skip background image
    if (item == renderer_->backgroundImageItem())
      continue;

    // Fill rectangles
    if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item)) {
      QBrush oldBrush = rect->brush();
      rect->setBrush(newBrush);
      renderer_->addAction(std::make_unique<FillAction>(item, oldBrush, newBrush));
      return;
    }

    // Fill ellipses
    if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      QBrush oldBrush = ellipse->brush();
      ellipse->setBrush(newBrush);
      renderer_->addAction(std::make_unique<FillAction>(item, oldBrush, newBrush));
      return;
    }

    // Fill polygons
    if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item)) {
      QBrush oldBrush = polygon->brush();
      polygon->setBrush(newBrush);
      renderer_->addAction(std::make_unique<FillAction>(item, oldBrush, newBrush));
      return;
    }
  }
}
