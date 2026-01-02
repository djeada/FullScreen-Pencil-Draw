// rectangle_tool.h
#ifndef RECTANGLE_TOOL_H
#define RECTANGLE_TOOL_H

#include "shape_tool.h"
#include <QGraphicsRectItem>

/**
 * @brief Tool for drawing rectangles.
 *
 * Draws rectangles with optional fill based on the fillShapes setting.
 */
class RectangleTool : public ShapeTool {
public:
  explicit RectangleTool(Canvas *canvas);
  ~RectangleTool() override;

  QString name() const override { return "Rectangle"; }

protected:
  QGraphicsItem *createShape(const QPointF &startPos) override;
  void updateShape(const QPointF &startPos, const QPointF &currentPos) override;
};

#endif // RECTANGLE_TOOL_H
