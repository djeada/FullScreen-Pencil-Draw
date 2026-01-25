/**
 * @file arrow_tool.cpp
 * @brief Arrow drawing tool implementation.
 */
#include "arrow_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ArrowTool::ArrowTool(SceneRenderer *renderer) : ShapeTool(renderer) {}

ArrowTool::~ArrowTool() = default;

QGraphicsItem *ArrowTool::createShape(const QPointF &startPos) {
  // Create a temporary rectangle to show the arrow preview
  auto *rect = new QGraphicsRectItem(QRectF(startPos, startPos));
  rect->setPen(renderer_->currentPen());
  return rect;
}

void ArrowTool::updateShape(const QPointF &startPos,
                             const QPointF &currentPos) {
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(tempShape_)) {
    rect->setRect(QRectF(startPos, currentPos).normalized());
  }
}

void ArrowTool::finalizeShape(const QPointF &startPos, const QPointF &endPos) {
  if (tempShape_) {
    // Remove the temporary preview rectangle using SceneController if available
    SceneController *controller = renderer_->sceneController();
    if (controller && tempShapeId_.isValid()) {
      controller->removeItem(tempShapeId_, false);  // Don't keep for undo
    } else {
      renderer_->scene()->removeItem(tempShape_);
      delete tempShape_;
    }
    tempShape_ = nullptr;
    tempShapeId_ = ItemId();

    // Draw the actual arrow
    drawArrow(startPos, endPos);
  }
}

void ArrowTool::drawArrow(const QPointF &start, const QPointF &end) {
  SceneController *controller = renderer_->sceneController();
  
  // Create line
  auto *line = new QGraphicsLineItem(QLineF(start, end));
  line->setPen(renderer_->currentPen());
  line->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
  
  if (controller) {
    controller->addItem(line);
  } else {
    renderer_->scene()->addItem(line);
  }

  // Calculate arrowhead
  double angle = std::atan2(-(end.y() - start.y()), end.x() - start.x());
  double arrowSize = renderer_->currentPen().width() * 4;

  QPolygonF arrowHead;
  arrowHead << end
            << end + QPointF(std::sin(angle + M_PI / 3) * arrowSize,
                             std::cos(angle + M_PI / 3) * arrowSize)
            << end + QPointF(std::sin(angle + M_PI - M_PI / 3) * arrowSize,
                             std::cos(angle + M_PI - M_PI / 3) * arrowSize);

  auto *arrowHeadItem = new QGraphicsPolygonItem(arrowHead);
  arrowHeadItem->setPen(renderer_->currentPen());
  arrowHeadItem->setBrush(renderer_->currentPen().color());
  arrowHeadItem->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
  
  if (controller) {
    controller->addItem(arrowHeadItem);
  } else {
    renderer_->scene()->addItem(arrowHeadItem);
  }

  renderer_->addDrawAction(line);
  renderer_->addDrawAction(arrowHeadItem);
}
