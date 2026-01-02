/**
 * @file rectangle_tool.cpp
 * @brief Rectangle drawing tool implementation.
 */
#include "rectangle_tool.h"
#include "../widgets/canvas.h"

RectangleTool::RectangleTool(Canvas *canvas) : ShapeTool(canvas) {}

RectangleTool::~RectangleTool() = default;

QGraphicsItem *RectangleTool::createShape(const QPointF &startPos) {
  auto *rect = new QGraphicsRectItem(QRectF(startPos, startPos));
  rect->setPen(canvas_->currentPen());
  if (canvas_->isFilledShapes()) {
    rect->setBrush(canvas_->currentPen().color());
  }
  return rect;
}

void RectangleTool::updateShape(const QPointF &startPos,
                                 const QPointF &currentPos) {
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(tempShape_)) {
    rect->setRect(QRectF(startPos, currentPos).normalized());
  }
}
