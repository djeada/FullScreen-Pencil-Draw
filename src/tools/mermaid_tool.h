/**
 * @file mermaid_tool.h
 * @brief Mermaid diagram tool.
 */
#ifndef MERMAID_TOOL_H
#define MERMAID_TOOL_H

#include "../core/item_id.h"
#include "tool.h"

class MermaidTextItem;

/**
 * @brief Tool for adding Mermaid diagrams to the canvas.
 *
 * Clicking on the canvas creates a new Mermaid diagram item.
 * Enter Mermaid code to create flowcharts, sequence diagrams, etc.
 * Clicking on an existing Mermaid item allows editing.
 * Double-clicking on a Mermaid item also allows editing.
 */
class MermaidTool : public Tool {
public:
  explicit MermaidTool(SceneRenderer *renderer);
  ~MermaidTool() override;

  QString name() const override { return "Mermaid"; }
  QCursor cursor() const override { return Qt::CrossCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

  void deactivate() override;

private:
  void createMermaidItem(const QPointF &position);
  MermaidTextItem *currentEditingItem_;
  ItemId currentEditingItemId_;
};

#endif // MERMAID_TOOL_H
