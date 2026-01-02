/**
 * @file text_tool.h
 * @brief Text annotation tool.
 */
#ifndef TEXT_TOOL_H
#define TEXT_TOOL_H

#include "tool.h"

/**
 * @brief Tool for adding text annotations to the canvas.
 *
 * Clicking on the canvas opens a dialog to enter text, which is
 * then placed at the clicked position.
 */
class TextTool : public Tool {
public:
  explicit TextTool(Canvas *canvas);
  ~TextTool() override;

  QString name() const override { return "Text"; }
  QCursor cursor() const override { return Qt::IBeamCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

private:
  void createTextItem(const QPointF &position);
};

#endif // TEXT_TOOL_H
