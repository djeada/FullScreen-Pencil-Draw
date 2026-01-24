/**
 * @file arrow_tool.h
 * @brief Arrow drawing tool implementation.
 */
#ifndef ARROW_TOOL_H
#define ARROW_TOOL_H

#include "shape_tool.h"
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>

/**
 * @brief Tool for drawing arrows.
 *
 * Draws arrows consisting of a line and a triangular arrowhead.
 */
class ArrowTool : public ShapeTool {
public:
  explicit ArrowTool(SceneRenderer *renderer);
  ~ArrowTool() override;

  QString name() const override { return "Arrow"; }

protected:
  QGraphicsItem *createShape(const QPointF &startPos) override;
  void updateShape(const QPointF &startPos, const QPointF &currentPos) override;
  void finalizeShape(const QPointF &startPos, const QPointF &endPos) override;

private:
  void drawArrow(const QPointF &start, const QPointF &end);
};

#endif // ARROW_TOOL_H
