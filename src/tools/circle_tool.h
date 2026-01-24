/**
 * @file circle_tool.h
 * @brief Circle/ellipse drawing tool.
 */
#ifndef CIRCLE_TOOL_H
#define CIRCLE_TOOL_H

#include "shape_tool.h"
#include <QGraphicsEllipseItem>

/**
 * @brief Tool for drawing circles and ellipses.
 *
 * Draws ellipses with optional fill based on the fillShapes setting.
 */
class CircleTool : public ShapeTool {
public:
  explicit CircleTool(SceneRenderer *renderer);
  ~CircleTool() override;

  QString name() const override { return "Circle"; }

protected:
  QGraphicsItem *createShape(const QPointF &startPos) override;
  void updateShape(const QPointF &startPos, const QPointF &currentPos) override;
};

#endif // CIRCLE_TOOL_H
