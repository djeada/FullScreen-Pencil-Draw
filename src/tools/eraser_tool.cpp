// eraser_tool.cpp
#include "eraser_tool.h"
#include "../widgets/canvas.h"
#include <QPainterPathStroker>

EraserTool::EraserTool(Canvas *canvas)
    : Tool(canvas), eraserPreview_(nullptr) {}

EraserTool::~EraserTool() {
  // Preview is owned by the scene, don't delete here
}

void EraserTool::activate() {
  if (!eraserPreview_) {
    int size = canvas_->eraserPen().width();
    eraserPreview_ = canvas_->scene()->addEllipse(0, 0, size, size,
                                                   QPen(Qt::gray),
                                                   QBrush(Qt::NoBrush));
    eraserPreview_->setZValue(1000);
  }
  eraserPreview_->show();
}

void EraserTool::deactivate() {
  hidePreview();
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
  qreal size = canvas_->eraserPen().width();
  QRectF eraseRect(point.x() - size / 2, point.y() - size / 2, size, size);
  QPainterPath erasePath;
  erasePath.addEllipse(eraseRect);

  QGraphicsScene *scene = canvas_->scene();
  QList<QGraphicsItem *> itemsToRemove;

  for (QGraphicsItem *item : scene->items(eraseRect)) {
    // Skip the eraser preview and background image
    if (item == eraserPreview_ || item == canvas_->backgroundImageItem())
      continue;

    QPainterPathStroker stroker;
    stroker.setWidth(1);
    if (erasePath.intersects(stroker.createStroke(item->shape()))) {
      itemsToRemove.append(item);
    }
  }

  for (QGraphicsItem *item : itemsToRemove) {
    canvas_->addDeleteAction(item);
    scene->removeItem(item);
  }
}

void EraserTool::updatePreview(const QPointF &pos) {
  if (!eraserPreview_)
    return;

  qreal radius = canvas_->eraserPen().width() / 2.0;
  eraserPreview_->setRect(pos.x() - radius, pos.y() - radius,
                          canvas_->eraserPen().width(),
                          canvas_->eraserPen().width());
  if (!eraserPreview_->isVisible()) {
    eraserPreview_->show();
  }
}

void EraserTool::hidePreview() {
  if (eraserPreview_) {
    eraserPreview_->hide();
  }
}
