/**
 * @file rectangle_tool.cpp
 * @brief Rectangle drawing tool implementation.
 */
#include "rectangle_tool.h"
#include "../core/scene_renderer.h"

RectangleTool::RectangleTool(SceneRenderer *renderer) : ShapeTool(renderer) {}

RectangleTool::~RectangleTool() = default;

QGraphicsItem *RectangleTool::createShape(const QPointF &startPos) {
  auto *rect = new QGraphicsRectItem(QRectF(startPos, startPos));
  rect->setPen(renderer_->currentPen());
  if (renderer_->isFilledShapes()) {
    rect->setBrush(renderer_->currentBrush());
  }
  return rect;
}

void RectangleTool::updateShape(const QPointF &startPos,
                                const QPointF &currentPos) {
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(tempShape_)) {
    rect->setRect(QRectF(startPos, currentPos).normalized());
  }
}
