/**
 * @file brush_stroke_item.cpp
 * @brief BrushStrokeItem implementation.
 */
#include "brush_stroke_item.h"
#include <QPainter>
#include <QtMath>
#include <algorithm>

BrushStrokeItem::BrushStrokeItem(const BrushTip &tip, qreal size,
                                 const QColor &color, qreal opacity,
                                 QGraphicsItem *parent)
    : QGraphicsItem(parent), tip_(tip), brushSize_(size), color_(color),
      opacity_(opacity) {
  tipImage_ = tip_.renderTip(brushSize_, color_, opacity_);
}

BrushStrokeItem::~BrushStrokeItem() = default;

void BrushStrokeItem::addPoint(const QPointF &scenePoint) {
  // Convert scene-point to item-local coordinates
  QPointF local = mapFromScene(scenePoint);

  // Determine spacing: distance between consecutive stamps
  qreal spacing = qMax(1.0, brushSize_ * tip_.stampSpacing());

  if (!points_.isEmpty()) {
    QPointF prev = points_.last();
    qreal dx = local.x() - prev.x();
    qreal dy = local.y() - prev.y();
    qreal dist = qSqrt(dx * dx + dy * dy);
    if (dist < spacing)
      return; // not far enough for a new stamp
  }

  points_.append(local);
  rebuildImage();
  update();
}

QRectF BrushStrokeItem::boundingRect() const { return bounds_; }

void BrushStrokeItem::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem * /*option*/,
                            QWidget * /*widget*/) {
  if (buffer_.isNull())
    return;
  painter->drawImage(bounds_.topLeft(), buffer_);
}

void BrushStrokeItem::rebuildImage() {
  if (points_.isEmpty())
    return;

  // Compute bounding rect of all points + brush radius + margin
  qreal half = brushSize_ / 2.0 + MARGIN;
  qreal minX = points_.first().x();
  qreal maxX = minX;
  qreal minY = points_.first().y();
  qreal maxY = minY;
  for (const auto &pt : points_) {
    minX = qMin(minX, pt.x());
    maxX = qMax(maxX, pt.x());
    minY = qMin(minY, pt.y());
    maxY = qMax(maxY, pt.y());
  }
  bounds_ = QRectF(minX - half, minY - half, (maxX - minX) + 2 * half,
                   (maxY - minY) + 2 * half);

  int w = qMax(1, static_cast<int>(qCeil(bounds_.width())));
  int h = qMax(1, static_cast<int>(qCeil(bounds_.height())));

  buffer_ = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
  buffer_.fill(Qt::transparent);

  QPainter p(&buffer_);
  p.setRenderHint(QPainter::Antialiasing);

  int tipW = tipImage_.width();
  int tipH = tipImage_.height();

  for (const auto &pt : points_) {
    qreal px = pt.x() - bounds_.left() - tipW / 2.0;
    qreal py = pt.y() - bounds_.top() - tipH / 2.0;
    p.drawImage(QPointF(px, py), tipImage_);
  }
  p.end();

  prepareGeometryChange();
}
