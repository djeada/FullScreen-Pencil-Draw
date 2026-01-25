/**
 * @file eraser_tool.cpp
 * @brief Eraser tool implementation.
 */
#include "eraser_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"
#include "../widgets/transform_handle_item.h"
#include <QPainterPathStroker>

EraserTool::EraserTool(SceneRenderer *renderer)
    : Tool(renderer), eraserPreview_(nullptr) {}

EraserTool::~EraserTool() {
  // Preview is owned by the scene, don't delete here
}

void EraserTool::activate() {
  if (!eraserPreview_) {
    int size = renderer_->eraserPen().width();
    eraserPreview_ = renderer_->scene()->addEllipse(0, 0, size, size,
                                                   QPen(Qt::gray),
                                                   QBrush(Qt::NoBrush));
    eraserPreview_->setZValue(1000);
    // Do NOT register with ItemStore - this is a UI helper, not user content.
    // Registering would cause it to be deleted by SceneController::clearAll()
    // while this tool still holds a raw pointer to it.
  }
  eraserPreview_->show();
}

void EraserTool::deactivate() {
  if (eraserPreview_) {
    eraserPreview_->hide();
    // Keep the item in the scene for reuse - don't delete it
    // The scene will clean it up when destroyed
  }
}

void EraserTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->buttons() & Qt::LeftButton) {
    eraseAt(scenePos);
  }
}

void EraserTool::mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) {
  updatePreview(scenePos);
  if (event->buttons() & Qt::LeftButton) {
    eraseAt(scenePos);
  }
}

void EraserTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                    const QPointF & /*scenePos*/) {
  // Nothing to do on release
}

void EraserTool::eraseAt(const QPointF &point) {
  qreal size = renderer_->eraserPen().width();
  QRectF eraseRect(point.x() - size / 2, point.y() - size / 2, size, size);
  QPainterPath erasePath;
  erasePath.addEllipse(eraseRect);

  QGraphicsScene *scene = renderer_->scene();
  SceneController *controller = renderer_->sceneController();
  QList<QGraphicsItem *> itemsToRemove;

  // Use Qt::IntersectsItemBoundingRect for reliable detection of filled items like pixmaps
  // The default Qt::IntersectsItemShape can fail for QGraphicsPixmapItem because its shape()
  // returns a complex outline of non-transparent pixels, making hit-testing unreliable
  for (QGraphicsItem *item : scene->items(eraseRect, Qt::IntersectsItemBoundingRect)) {
    // Skip the eraser preview and background image
    if (item == eraserPreview_ || item == renderer_->backgroundImageItem())
      continue;

    // Skip TransformHandleItems - they are UI elements, not user content
    if (item->type() == TransformHandleItem::Type)
      continue;

    // Get bounding rect in scene coordinates - this is always reliable
    QRectF itemSceneBounds = item->sceneBoundingRect();
    
    // Simple and reliable: if eraser point is inside item's scene bounding rect, erase it
    // This works for filled items like pixmaps, rectangles, ellipses
    if (itemSceneBounds.contains(point)) {
      itemsToRemove.append(item);
      continue;
    }
    
    // For line-based items (paths), check if eraser touches the stroked shape
    // Transform click point to item-local coordinates for shape check
    QPointF localPoint = item->mapFromScene(point);
    QPainterPath itemShape = item->shape();
    
    // Stroke the shape with eraser size to create a "hit area" around lines
    QPainterPathStroker stroker;
    stroker.setWidth(size);
    QPainterPath strokedShape = stroker.createStroke(itemShape);
    if (strokedShape.contains(localPoint)) {
      itemsToRemove.append(item);
    }
  }

  for (QGraphicsItem *item : itemsToRemove) {
    // First add to undo stack
    renderer_->addDeleteAction(item);
    
    // Then actually remove the item
    if (controller) {
      controller->removeItem(item, true);  // Keep for undo
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
