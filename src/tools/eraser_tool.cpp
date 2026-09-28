/**
 * @file eraser_tool.cpp
 * @brief Eraser tool implementation.
 */
#include "eraser_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"
#include "../widgets/brush_stroke_item.h"
#include "../widgets/raster_layer_item.h"
#include "../widgets/transform_handle_item.h"
#include <QGraphicsItemGroup>
#include <QGraphicsPixmapItem>

EraserTool::EraserTool(SceneRenderer *renderer)
    : Tool(renderer), eraserPreview_(nullptr) {}

EraserTool::~EraserTool() {
  // Preview is owned by the scene, don't delete here
}

void EraserTool::activate() {
  if (!eraserPreview_) {
    int size = renderer_->eraserPen().width();
    eraserPreview_ = renderer_->scene()->addEllipse(
        0, 0, size, size, QPen(Qt::gray), QBrush(Qt::NoBrush));
    eraserPreview_->setZValue(1e9); // above all layer content
    // Do NOT register with ItemStore - this is a UI helper, not user content.
    // Registering would cause it to be deleted by SceneController::clearAll()
    // while this tool still holds a raw pointer to it.
  }
  eraserPreview_->show();
}

void EraserTool::deactivate() {
  renderer_->endActionGroup(); // switching tools mid-drag closes the step
  if (eraserPreview_) {
    eraserPreview_->hide();
    // Keep the item in the scene for reuse - don't delete it
    // The scene will clean it up when destroyed
  }
}

void EraserTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->button() == Qt::LeftButton) {
    // Everything one drag erases becomes a single undo step (instead of one
    // entry per item flooding the history).
    renderer_->beginActionGroup();
    eraseAt(scenePos);
  }
}

void EraserTool::mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) {
  updatePreview(scenePos);
  if (event->buttons() & Qt::LeftButton) {
    eraseAt(scenePos);
  }
}

void EraserTool::mouseReleaseEvent(QMouseEvent *event,
                                   const QPointF & /*scenePos*/) {
  if (!event || event->button() == Qt::LeftButton)
    renderer_->endActionGroup();
}

void EraserTool::eraseAt(const QPointF &point) {
  qreal size = renderer_->eraserPen().width();
  QRectF eraseRect(point.x() - size / 2, point.y() - size / 2, size, size);
  QPainterPath erasePath;
  erasePath.addEllipse(eraseRect);

  QGraphicsScene *scene = renderer_->scene();
  SceneController *controller = renderer_->sceneController();
  QList<QGraphicsItem *> itemsToRemove;

  QGraphicsItem *background = renderer_->backgroundImageItem();
  for (QGraphicsItem *item :
       scene->items(eraseRect, Qt::IntersectsItemBoundingRect)) {
    // A group's own shape is its whole bounding box: test its children's
    // real shapes instead (they are in this list too).
    if (item->type() == QGraphicsItemGroup::Type)
      continue;
    // Erase whole top-level items: removing a group child on its own would
    // detach it from its arrow/group and undo would restore it misplaced.
    QGraphicsItem *target = item->topLevelItem();
    if (target == eraserPreview_ || target == background ||
        target->type() == TransformHandleItem::Type ||
        target->type() == RasterLayerItem::Type ||
        itemsToRemove.contains(target))
      continue;
    // Hidden or locked objects are never erased.
    if (!target->isVisible() ||
        target->data(0).toString() == QLatin1String("locked"))
      continue;

    bool hit = false;
    if (dynamic_cast<QGraphicsPixmapItem *>(item) ||
        dynamic_cast<BrushStrokeItem *>(item)) {
      // Raster objects: only their visible pixels count, so touching the
      // transparent part of a large image does not delete it.
      hit = erasePath.intersects(item->sceneBoundingRect()) &&
            PixelEditAction::hitsOpaquePixels(item, erasePath);
    } else {
      // Test the actual (stroked) shape, not the bounding box: touching the
      // empty inside of a diagonal line's box must not erase it.
      hit = erasePath.intersects(item->sceneTransform().map(item->shape()));
    }
    if (hit)
      itemsToRemove.append(target);
  }

  for (QGraphicsItem *item : itemsToRemove) {
    // First add to undo stack
    renderer_->addDeleteAction(item);

    // Then actually remove the item
    if (controller) {
      controller->removeItem(item, true); // Keep for undo
    } else {
      scene->removeItem(item);
      renderer_->onItemRemoved(item);
    }
  }
}

void EraserTool::updatePreview(const QPointF &pos) {
  if (!eraserPreview_)
    return;

  qreal radius = renderer_->eraserPen().width() / 2.0;
  eraserPreview_->setRect(pos.x() - radius, pos.y() - radius,
                          renderer_->eraserPen().width(),
                          renderer_->eraserPen().width());
  if (!eraserPreview_->isVisible()) {
    eraserPreview_->show();
  }
}

void EraserTool::hidePreview() {
  if (eraserPreview_) {
    eraserPreview_->hide();
  }
}
