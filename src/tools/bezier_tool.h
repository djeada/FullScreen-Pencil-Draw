/**
 * @file bezier_tool.h
 * @brief Bezier path drawing tool with click-to-place control points.
 */
#ifndef BEZIER_TOOL_H
#define BEZIER_TOOL_H

#include "../core/item_id.h"
#include "tool.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QVector>

/**
 * @brief Tool for drawing cubic Bezier curves.
 *
 * The Bezier tool lets users create precise vector paths by clicking
 * to place anchor points. Dragging while clicking sets the tangent
 * handles for each anchor, producing smooth cubic Bezier segments.
 * Double-click or press Enter/Escape to finish the path.
 */
class BezierTool : public Tool {
public:
  explicit BezierTool(SceneRenderer *renderer);
  ~BezierTool() override;

  QString name() const override { return "Bezier"; }
  QCursor cursor() const override { return Qt::CrossCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

  void deactivate() override;

private:
  /** @brief Information stored for each anchor point. */
  struct AnchorPoint {
    QPointF position;    ///< The anchor position on the path
    QPointF handleOut;   ///< The outgoing control handle (tangent)
    bool hasHandle;      ///< True if a handle was set by dragging
  };

  void finalizePath();
  void updatePreview(const QPointF &mousePos);
  void rebuildPath();
  void clearPreviewItems();

  QGraphicsPathItem *currentPath_;
  ItemId currentPathId_;
  QVector<AnchorPoint> anchors_;

  // Preview items (not part of the final path)
  QGraphicsPathItem *previewSegment_;
  QVector<QGraphicsEllipseItem *> anchorMarkers_;

  bool isDragging_;       ///< True while dragging to set a handle
  QPointF dragStart_;     ///< Where the drag started (anchor position)

  static constexpr qreal ANCHOR_MARKER_SIZE = 6.0;
};

#endif // BEZIER_TOOL_H
