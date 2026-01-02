// pan_tool.h
#ifndef PAN_TOOL_H
#define PAN_TOOL_H

#include "tool.h"

/**
 * @brief Tool for panning/scrolling the canvas.
 *
 * Allows the user to drag the canvas view to navigate around.
 */
class PanTool : public Tool {
public:
  explicit PanTool(Canvas *canvas);
  ~PanTool() override;

  QString name() const override { return "Pan"; }
  QCursor cursor() const override;

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

private:
  bool isPanning_;
  QPoint lastPanPoint_;
};

#endif // PAN_TOOL_H
