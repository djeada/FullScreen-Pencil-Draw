/**
 * @file text_on_path_tool.h
 * @brief Tool for placing text along a user-drawn path.
 */
#ifndef TEXT_ON_PATH_TOOL_H
#define TEXT_ON_PATH_TOOL_H

#include "../core/item_id.h"
#include "tool.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QVector>

class TextOnPathItem;

/**
 * @brief Tool that lets users draw a path and then type text along it.
 *
 * Usage:
 *  1. Click to place anchor points (drag to set tangent handles).
 *  2. Double-click or press Enter/Escape to finish the path.
 *  3. A dialog prompts for the text to render along the path.
 */
class TextOnPathTool : public Tool {
public:
  explicit TextOnPathTool(SceneRenderer *renderer);
  ~TextOnPathTool() override;

  QString name() const override { return "TextOnPath"; }
  QCursor cursor() const override { return Qt::CrossCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

  void deactivate() override;

private:
  struct AnchorPoint {
    QPointF position;
    QPointF handleOut;
    bool hasHandle;
  };

  void finalizePath();
  void updatePreview(const QPointF &mousePos);
  void rebuildPath();
  void clearPreviewItems();

  QGraphicsPathItem *previewPath_;
  QVector<AnchorPoint> anchors_;

  QGraphicsPathItem *previewSegment_;
  QVector<QGraphicsEllipseItem *> anchorMarkers_;

  bool isDragging_;
  QPointF dragStart_;

  static constexpr qreal ANCHOR_MARKER_SIZE = 6.0;
};

#endif // TEXT_ON_PATH_TOOL_H
