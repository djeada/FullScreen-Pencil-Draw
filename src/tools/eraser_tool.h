/**
 * @file eraser_tool.h
 * @brief Eraser tool for removing items from canvas.
 */
#ifndef ERASER_TOOL_H
#define ERASER_TOOL_H

#include "tool.h"
#include <QGraphicsEllipseItem>

/**
 * @brief Object Eraser: removes whole objects touched by its circular area.
 *
 * Only visible, unlocked objects whose real shape (or visible pixels, for
 * images and brush strokes) is touched are removed; a whole drag is one
 * undo step. Raster layer pixels are left to the Pixel Eraser.
 */
class EraserTool : public Tool {
public:
  explicit EraserTool(SceneRenderer *renderer);
  ~EraserTool() override;

  QString name() const override { return "Object Eraser"; }
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
  // Note: eraserPreview_ is a UI helper, NOT registered with ItemStore.
  // This prevents SceneController::clearAll() from deleting it while
  // this tool still holds a pointer to it.
};

#endif // ERASER_TOOL_H
