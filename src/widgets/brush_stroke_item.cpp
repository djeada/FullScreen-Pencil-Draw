/**
 * @file brush_stroke_item.cpp
 * @brief BrushStrokeItem implementation.
 */
#include "brush_stroke_item.h"
#include <QPainter>
#include <QtMath>
#include <algorithm>

namespace {
constexpr int MAX_BUFFER_DIM = 16384;
constexpr qreal GROW_FACTOR = 1.5;
} // namespace

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

  if (points_.isEmpty()) {
    points_.append(local);
    initBuffer(local);
    stampPoint(local);
    update();
    return;
  }

  // Grow the raster buffer only when the new stamp would leave it.
  if (!bounds_
           .adjusted(tipImage_.width() / 2.0, tipImage_.height() / 2.0,
                     -tipImage_.width() / 2.0, -tipImage_.height() / 2.0)
           .contains(local)) {
    expandBuffer(local);
  }

  points_.append(local);
  stampPoint(local);
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

qreal BrushStrokeItem::stampRadius() const {
  return qMax<qreal>(tipImage_.width(), tipImage_.height()) / 2.0 + MARGIN;
}

void BrushStrokeItem::initBuffer(const QPointF &p) {
  qreal r = stampRadius();
  QRectF rect(p.x() - r, p.y() - r, 2 * r, 2 * r);
  allocateBuffer(rect);
}

void BrushStrokeItem::expandBuffer(const QPointF &p) {
  qreal r = stampRadius();
  QRectF needed(
      qMin(p.x(), bounds_.left()) - r, qMin(p.y(), bounds_.top()) - r,
      qMax(p.x(), bounds_.right()) - qMin(p.x(), bounds_.left()) + 2 * r,
      qMax(p.y(), bounds_.bottom()) - qMin(p.y(), bounds_.top()) + 2 * r);

  // Grow generously so expansions stay rare.
  qreal extraW = needed.width() * (GROW_FACTOR - 1.0);
  qreal extraH = needed.height() * (GROW_FACTOR - 1.0);
  needed.adjust(-extraW / 2.0, -extraH / 2.0, extraW / 2.0, extraH / 2.0);

  allocateBuffer(needed);
}

void BrushStrokeItem::allocateBuffer(const QRectF &desired) {
  // Clamp to the maximum buffer dimension; content beyond the clamp is
  // cropped consistently because bounds_ always mirrors the buffer rect.
  QRectF rect = desired;
  if (rect.width() > MAX_BUFFER_DIM) {
    qreal left =
        qMax(desired.left(), bounds_.center().x() - MAX_BUFFER_DIM / 2.0);
    left = qMin(left, desired.right() - MAX_BUFFER_DIM);
    rect.setLeft(left);
    rect.setWidth(MAX_BUFFER_DIM);
  }
  if (rect.height() > MAX_BUFFER_DIM) {
    qreal top =
        qMax(desired.top(), bounds_.center().y() - MAX_BUFFER_DIM / 2.0);
    top = qMin(top, desired.bottom() - MAX_BUFFER_DIM);
    rect.setTop(top);
    rect.setHeight(MAX_BUFFER_DIM);
  }

  int w = qMax(1, static_cast<int>(qCeil(rect.width())));
  int h = qMax(1, static_cast<int>(qCeil(rect.height())));

  // Qt requires prepareGeometryChange BEFORE the bounding rect changes.
  prepareGeometryChange();
  QImage previous = buffer_;
  QRectF previousBounds = bounds_;

  bounds_ = QRectF(rect.topLeft(), QSizeF(w, h));
  buffer_ = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
  buffer_.fill(Qt::transparent);

  if (!previous.isNull()) {
    QPainter p(&buffer_);
    p.drawImage(previousBounds.topLeft() - bounds_.topLeft(), previous);
    p.end();
  }
}

void BrushStrokeItem::stampPoint(const QPointF &p) {
  if (buffer_.isNull())
    return;
  QPainter pnt(&buffer_);
  pnt.setRenderHint(QPainter::Antialiasing);
  qreal px = p.x() - bounds_.left() - tipImage_.width() / 2.0;
  qreal py = p.y() - bounds_.top() - tipImage_.height() / 2.0;
  pnt.drawImage(QPointF(px, py), tipImage_);
}
