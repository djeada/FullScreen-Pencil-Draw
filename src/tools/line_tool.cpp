/**
 * @file line_tool.cpp
 * @brief Straight line drawing tool implementation.
 */
#include "line_tool.h"
#include "../widgets/canvas.h"

LineTool::LineTool(Canvas *canvas) : ShapeTool(canvas) {}

LineTool::~LineTool() = default;

QGraphicsItem *LineTool::createShape(const QPointF &startPos) {
  auto *line = new QGraphicsLineItem(QLineF(startPos, startPos));
  line->setPen(canvas_->currentPen());
  return line;
}

void LineTool::updateShape(const QPointF &startPos,
                            const QPointF &currentPos) {
  if (auto *line = dynamic_cast<QGraphicsLineItem *>(tempShape_)) {
    line->setLine(QLineF(startPos, currentPos));
  }
}
