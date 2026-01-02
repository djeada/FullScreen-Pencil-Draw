// circle_tool.cpp
#include "circle_tool.h"
#include "../widgets/canvas.h"

CircleTool::CircleTool(Canvas *canvas) : ShapeTool(canvas) {}

CircleTool::~CircleTool() = default;

QGraphicsItem *CircleTool::createShape(const QPointF &startPos) {
  auto *ellipse = new QGraphicsEllipseItem(QRectF(startPos, startPos));
  ellipse->setPen(canvas_->currentPen());
  if (canvas_->isFilledShapes()) {
    ellipse->setBrush(canvas_->currentPen().color());
  }
  return ellipse;
}

void CircleTool::updateShape(const QPointF &startPos,
                              const QPointF &currentPos) {
  if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(tempShape_)) {
    ellipse->setRect(QRectF(startPos, currentPos).normalized());
  }
}
