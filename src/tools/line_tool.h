/**
 * @file line_tool.h
 * @brief Straight line drawing tool.
 */
#ifndef LINE_TOOL_H
#define LINE_TOOL_H

#include "shape_tool.h"
#include <QGraphicsLineItem>

/**
 * @brief Tool for drawing straight lines.
 */
class LineTool : public ShapeTool {
public:
  explicit LineTool(SceneRenderer *renderer);
  ~LineTool() override;

  QString name() const override { return "Line"; }

protected:
  QGraphicsItem *createShape(const QPointF &startPos) override;
  void updateShape(const QPointF &startPos, const QPointF &currentPos) override;
};

#endif // LINE_TOOL_H
