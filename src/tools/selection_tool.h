/**
 * @file selection_tool.h
 * @brief Selection tool for selecting and moving items.
 */
#ifndef SELECTION_TOOL_H
#define SELECTION_TOOL_H

#include "tool.h"

/**
 * @brief Tool for selecting and moving items on the canvas.
 *
 * Uses rubber band selection for selecting multiple items.
 * Selected items can be moved by dragging.
 */
class SelectionTool : public Tool {
public:
  explicit SelectionTool(SceneRenderer *renderer);
  ~SelectionTool() override;

  QString name() const override { return "Select"; }
  QCursor cursor() const override { return Qt::ArrowCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

  bool usesRubberBandSelection() const override { return true; }
};

#endif // SELECTION_TOOL_H
