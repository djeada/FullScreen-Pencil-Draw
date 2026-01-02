/**
 * @file eraser_tool.h
 * @brief Eraser tool for removing items from canvas.
 */
#ifndef ERASER_TOOL_H
#define ERASER_TOOL_H

#include "tool.h"
#include <QGraphicsEllipseItem>

/**
 * @brief Eraser tool for removing items from the canvas.
 *
 * The eraser tool removes any items that intersect with the eraser's
 * circular area. It displays a preview cursor showing the eraser size.
 */
class EraserTool : public Tool {
public:
  explicit EraserTool(Canvas *canvas);
  ~EraserTool() override;

  QString name() const override { return "Eraser"; }
  QCursor cursor() const override { return Qt::BlankCursor; }

  void activate() override;
  void deactivate() override;

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

  bool itemsSelectable() const override { return false; }

private:
  void eraseAt(const QPointF &point);
  void updatePreview(const QPointF &position);
  void hidePreview();

  QGraphicsEllipseItem *eraserPreview_;
};

#endif // ERASER_TOOL_H
