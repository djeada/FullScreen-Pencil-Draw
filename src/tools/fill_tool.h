/**
 * @file fill_tool.h
 * @brief Fill tool for coloring shapes.
 */
#ifndef FILL_TOOL_H
#define FILL_TOOL_H

#include "tool.h"

/**
 * @brief Tool for filling closed shapes with color.
 *
 * Clicking on a shape fills it with the current color.
 */
class FillTool : public Tool {
public:
  explicit FillTool(SceneRenderer *renderer);
  ~FillTool() override;

  QString name() const override { return "Fill"; }
  QCursor cursor() const override { return Qt::PointingHandCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

private:
  void fillAt(const QPointF &point);
};

#endif // FILL_TOOL_H
