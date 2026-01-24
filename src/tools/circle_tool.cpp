/**
 * @file circle_tool.cpp
 * @brief Circle/ellipse drawing tool implementation.
 */
#include "circle_tool.h"
#include "../core/scene_renderer.h"

CircleTool::CircleTool(SceneRenderer *renderer) : ShapeTool(renderer) {}

CircleTool::~CircleTool() = default;

QGraphicsItem *CircleTool::createShape(const QPointF &startPos) {
  auto *ellipse = new QGraphicsEllipseItem(QRectF(startPos, startPos));
  ellipse->setPen(renderer_->currentPen());
  if (renderer_->isFilledShapes()) {
    ellipse->setBrush(renderer_->currentPen().color());
  }
  return ellipse;
}

void CircleTool::updateShape(const QPointF &startPos,
                              const QPointF &currentPos) {
  if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(tempShape_)) {
    ellipse->setRect(QRectF(startPos, currentPos).normalized());
  }
}
