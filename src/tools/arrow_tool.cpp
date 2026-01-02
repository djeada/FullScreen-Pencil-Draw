// arrow_tool.cpp
#include "arrow_tool.h"
#include "../widgets/canvas.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ArrowTool::ArrowTool(Canvas *canvas) : ShapeTool(canvas) {}

ArrowTool::~ArrowTool() = default;

QGraphicsItem *ArrowTool::createShape(const QPointF &startPos) {
  // Create a temporary rectangle to show the arrow preview
  auto *rect = new QGraphicsRectItem(QRectF(startPos, startPos));
  rect->setPen(canvas_->currentPen());
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
    // Remove the temporary preview rectangle
    canvas_->scene()->removeItem(tempShape_);
    delete tempShape_;
    tempShape_ = nullptr;

    // Draw the actual arrow
    drawArrow(startPos, endPos);
  }
}

void ArrowTool::drawArrow(const QPointF &start, const QPointF &end) {
  // Create line
  auto *line = new QGraphicsLineItem(QLineF(start, end));
  line->setPen(canvas_->currentPen());
  line->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
  canvas_->scene()->addItem(line);

  // Calculate arrowhead
  double angle = std::atan2(-(end.y() - start.y()), end.x() - start.x());
  double arrowSize = canvas_->currentPen().width() * 4;

  QPolygonF arrowHead;
  arrowHead << end
            << end + QPointF(std::sin(angle + M_PI / 3) * arrowSize,
                             std::cos(angle + M_PI / 3) * arrowSize)
            << end + QPointF(std::sin(angle + M_PI - M_PI / 3) * arrowSize,
                             std::cos(angle + M_PI - M_PI / 3) * arrowSize);

  auto *arrowHeadItem = new QGraphicsPolygonItem(arrowHead);
  arrowHeadItem->setPen(canvas_->currentPen());
  arrowHeadItem->setBrush(canvas_->currentPen().color());
  arrowHeadItem->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
  canvas_->scene()->addItem(arrowHeadItem);

  canvas_->addDrawAction(line);
  canvas_->addDrawAction(arrowHeadItem);
}
