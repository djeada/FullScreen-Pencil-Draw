/**
 * @file eraser_tool.cpp
 * @brief Eraser tool implementation.
 */
#include "eraser_tool.h"
#include "../core/scene_renderer.h"
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
  }
  eraserPreview_->show();
}

void EraserTool::deactivate() {
  if (eraserPreview_) {
    eraserPreview_->hide();
    // Don't delete - item is owned by the scene
    // Just hide it and clear our pointer reference
    eraserPreview_ = nullptr;
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
  QList<QGraphicsItem *> itemsToRemove;

  for (QGraphicsItem *item : scene->items(eraseRect)) {
    // Skip the eraser preview and background image
    if (item == eraserPreview_ || item == renderer_->backgroundImageItem())
      continue;

    QPainterPath itemShape = item->shape();

    // Check if eraser intersects either the item's shape (for filled items like
    // pixmaps) or the stroked outline (for line-based items like paths)
    QPainterPathStroker stroker;
    stroker.setWidth(1);
    if (erasePath.intersects(itemShape) ||
        erasePath.intersects(stroker.createStroke(itemShape))) {
      itemsToRemove.append(item);
    }
  }

  for (QGraphicsItem *item : itemsToRemove) {
    renderer_->addDeleteAction(item);
    scene->removeItem(item);
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
