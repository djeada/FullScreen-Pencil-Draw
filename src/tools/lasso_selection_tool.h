/**
 * @file lasso_selection_tool.h
 * @brief Lasso (polygon) selection tool for flexible item selection.
 */
#ifndef LASSO_SELECTION_TOOL_H
#define LASSO_SELECTION_TOOL_H

#include "tool.h"
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QVector>

/**
 * @brief Tool for selecting items using a freehand lasso/polygon path.
 *
 * More flexible than rectangle-only selection. The user draws a freehand
 * closed region; on mouse release the path is closed and all items
 * contained within or intersecting the polygon are selected.
 */
class LassoSelectionTool : public Tool {
public:
  explicit LassoSelectionTool(SceneRenderer *renderer);
  ~LassoSelectionTool() override;

  QString name() const override { return "LassoSelect"; }
  QCursor cursor() const override { return Qt::CrossCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

  void deactivate() override;

  bool itemsSelectable() const override { return true; }

private:
  void removeLassoPath();

  QGraphicsPathItem *lassoPath_;
  QVector<QPointF> points_;
  bool drawing_;
};

#endif // LASSO_SELECTION_TOOL_H
