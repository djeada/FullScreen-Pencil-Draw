/**
 * @file line_tool.cpp
 * @brief Straight line drawing tool implementation.
 */
#include "line_tool.h"
#include "../core/scene_renderer.h"

LineTool::LineTool(SceneRenderer *renderer) : ShapeTool(renderer) {}

LineTool::~LineTool() = default;

QGraphicsItem *LineTool::createShape(const QPointF &startPos) {
  auto *line = new QGraphicsLineItem(QLineF(startPos, startPos));
  line->setPen(renderer_->currentPen());
  return line;
}

void LineTool::updateShape(const QPointF &startPos,
                            const QPointF &currentPos) {
  if (auto *line = dynamic_cast<QGraphicsLineItem *>(tempShape_)) {
    line->setLine(QLineF(startPos, currentPos));
  }
}
