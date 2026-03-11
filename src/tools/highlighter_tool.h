/**
 * @file highlighter_tool.h
 * @brief Semi-transparent freehand highlighter tool.
 */
#ifndef HIGHLIGHTER_TOOL_H
#define HIGHLIGHTER_TOOL_H

#include "../core/item_id.h"
#include "tool.h"
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QVector>

/**
 * @brief Freehand highlight tool for marking text and strokes.
 *
 * The highlighter behaves like a wide translucent pen so it works on both
 * the canvas and the PDF annotation overlay.
 */
class HighlighterTool : public Tool {
public:
  explicit HighlighterTool(SceneRenderer *renderer);
  ~HighlighterTool() override;

  QString name() const override { return "Highlighter"; }
  QCursor cursor() const override { return Qt::CrossCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

  void deactivate() override;

private:
  void addPoint(const QPointF &point);
  QPen highlighterPen() const;

  QGraphicsPathItem *currentPath_;
  ItemId currentItemId_;
  QVector<QPointF> pointBuffer_;
  static constexpr int MIN_POINTS_FOR_SPLINE = 4;
};

#endif // HIGHLIGHTER_TOOL_H
