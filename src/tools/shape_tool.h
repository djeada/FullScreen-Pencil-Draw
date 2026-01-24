/**
 * @file shape_tool.h
 * @brief Abstract base class for shape drawing tools.
 */
#ifndef SHAPE_TOOL_H
#define SHAPE_TOOL_H

#include "tool.h"

/**
 * @brief Abstract base class for shape drawing tools.
 *
 * This class provides common functionality for tools that draw
 * geometric shapes like rectangles, circles, and lines.
 */
class ShapeTool : public Tool {
public:
  explicit ShapeTool(SceneRenderer *renderer);
  ~ShapeTool() override;

  QCursor cursor() const override { return Qt::CrossCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

protected:
  /**
   * @brief Create the initial shape item at the start position
   * @param startPos The starting position
   * @return The created graphics item
   */
  virtual QGraphicsItem *createShape(const QPointF &startPos) = 0;

  /**
   * @brief Update the shape during mouse drag
   * @param startPos The starting position
   * @param currentPos The current mouse position
   */
  virtual void updateShape(const QPointF &startPos,
                           const QPointF &currentPos) = 0;

  /**
   * @brief Finalize the shape on mouse release
   * @param startPos The starting position
   * @param endPos The ending position
   */
  virtual void finalizeShape(const QPointF &startPos, const QPointF &endPos);

  QGraphicsItem *tempShape_;
  QPointF startPoint_;
};

#endif // SHAPE_TOOL_H
