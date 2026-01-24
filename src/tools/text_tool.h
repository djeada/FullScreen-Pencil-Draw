/**
 * @file text_tool.h
 * @brief Text annotation tool.
 */
#ifndef TEXT_TOOL_H
#define TEXT_TOOL_H

#include "tool.h"

class LatexTextItem;

/**
 * @brief Tool for adding text annotations to the canvas.
 *
 * Clicking on the canvas opens a dialog to enter text.
 * Text enclosed by $...$ will be rendered as LaTeX math expressions.
 * Clicking on an existing text item opens a dialog to edit it.
 * Double-clicking on a text item also allows editing.
 */
class TextTool : public Tool {
public:
  explicit TextTool(SceneRenderer *renderer);
  ~TextTool() override;

  QString name() const override { return "Text"; }
  QCursor cursor() const override { return Qt::IBeamCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

private:
  void createTextItem(const QPointF &position);
  LatexTextItem *currentEditingItem_;
};

#endif // TEXT_TOOL_H
