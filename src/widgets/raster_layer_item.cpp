/**
 * @file raster_layer_item.cpp
 * @brief RasterLayerItem and PixelEditAction implementation.
 */
#include "raster_layer_item.h"
#include "../core/item_store.h"
#include "brush_stroke_item.h"
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

RasterLayerItem::RasterLayerItem(QGraphicsItem *parent)
    : QGraphicsItem(parent) {
  // Pixels are edited in place with the raster tools; dragging the whole
  // layer content around by accident would be surprising.
  setFlag(QGraphicsItem::ItemIsSelectable, false);
  setFlag(QGraphicsItem::ItemIsMovable, false);
}

void RasterLayerItem::surfaceChanged() {
  const QRectF newBounds = QRectF(surface_.boundingRect());
  if (newBounds != bounds_) {
    prepareGeometryChange();
    bounds_ = newBounds;
  }
  update();
}

void RasterLayerItem::paintStroke(const QPointF &from, const QPointF &to,
                                  const QPen &pen) {
  const qreal pad = pen.widthF() / 2.0 + 2.0;
  const QRect area = QRectF(from, to)
                         .normalized()
                         .adjusted(-pad, -pad, pad, pad)
                         .toAlignedRect();
  QPen roundPen = pen;
  roundPen.setCapStyle(Qt::RoundCap);
  roundPen.setJoinStyle(Qt::RoundJoin);
  surface_.paint(area, [&](QPainter &p) {
    p.setPen(roundPen);
    if (from == to)
      p.drawPoint(from);
    else
      p.drawLine(from, to);
  });
  surfaceChanged();
}

void RasterLayerItem::eraseDab(const QPointF &center, qreal diameter,
                               qreal opacity, qreal hardness,
                               const QPainterPath &clip) {
  QPainterPath dab;
  dab.addEllipse(center, diameter / 2.0, diameter / 2.0);
  surface_.erase(dab, opacity, hardness, clip);
  surfaceChanged();
}

QVariant RasterLayerItem::itemChange(GraphicsItemChange change,
                                     const QVariant &value) {
  if (change == ItemFlagsChange) {
    auto flags = GraphicsItemFlags(value.toUInt());
    flags &= ~(ItemIsSelectable | ItemIsMovable);
    return QVariant(static_cast<uint>(flags));
  }
  return QGraphicsItem::itemChange(change, value);
}

QRectF RasterLayerItem::boundingRect() const { return bounds_; }

QPainterPath RasterLayerItem::shape() const {
  // Hit-testing by allocated tiles would make empty areas "solid"; only
  // tiles with pixels exist, which is close enough for selection tools
  // that skip this item anyway (it is not selectable).
  QPainterPath path;
  for (const RasterTileKey &key : surface_.tileKeys())
    path.addRect(RasterSurface::tileRect(key));
  return path;
}

void RasterLayerItem::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *option,
                            QWidget * /*widget*/) {
  surface_.render(painter, option ? option->exposedRect : QRectF());
}

// ==================== PixelEditAction ====================

PixelEditAction::PixelEditAction(const ItemId &id, ItemStore *store,
                                 RasterTileDiff diff, QString description)
    : itemId_(id), itemStore_(store), tiled_(true), diff_(std::move(diff)),
      description_(std::move(description)) {}

PixelEditAction::PixelEditAction(const ItemId &id, ItemStore *store,
                                 const QImage &before, const QImage &after,
                                 QString description)
    : itemId_(id), itemStore_(store), tiled_(false), before_(before),
      after_(after), description_(std::move(description)) {}

std::size_t PixelEditAction::memoryCost() const {
  if (tiled_)
    return kBaseActionCost + diff_.byteSize();
  return kBaseActionCost + static_cast<std::size_t>(before_.sizeInBytes()) +
         static_cast<std::size_t>(after_.sizeInBytes());
}

QImage PixelEditAction::itemImage(QGraphicsItem *item) {
  if (auto *stroke = dynamic_cast<BrushStrokeItem *>(item))
    return stroke->image();
  if (auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item)) {
    QImage image = pixmapItem->pixmap().toImage().convertToFormat(
        QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(pixmapItem->pixmap().devicePixelRatio());
    return image;
  }
  return QImage();
}

bool PixelEditAction::setItemImage(QGraphicsItem *item, const QImage &image) {
  if (image.isNull())
    return false;
  if (auto *stroke = dynamic_cast<BrushStrokeItem *>(item)) {
    stroke->setImage(image);
    return true;
  }
  if (auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item)) {
    QPixmap pm = QPixmap::fromImage(image);
    pm.setDevicePixelRatio(image.devicePixelRatio());
    pixmapItem->setPixmap(pm);
    return true;
  }
  return false;
}

// Whether the dab touches visible pixels of a raster item, so erasing the
// transparent part of a big image (or brush stroke) never deletes it.
bool PixelEditAction::hitsOpaquePixels(QGraphicsItem *item,
                                       const QPainterPath &dab) {
  const QImage image = itemImage(item);
  if (image.isNull())
    return true;
  QPointF offset;
  if (auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item))
    offset = pixmapItem->offset();
  else if (auto *stroke = dynamic_cast<BrushStrokeItem *>(item))
    offset = stroke->imageRect().topLeft();
  const qreal dpr = image.devicePixelRatio();
  bool invertible = false;
  const QTransform toItem = item->sceneTransform().inverted(&invertible);
  if (!invertible)
    return true;
  const QTransform toImage =
      toItem * QTransform::fromTranslate(-offset.x(), -offset.y()) *
      QTransform::fromScale(dpr, dpr);
  const QPainterPath local = toImage.map(dab);
  const QRect area =
      local.boundingRect().toAlignedRect().intersected(image.rect());
  // Sample on a grid so huge dabs over huge images stay cheap.
  const int step = qMax(1, qMax(area.width(), area.height()) / 48);
  for (int y = area.top(); y <= area.bottom(); y += step) {
    for (int x = area.left(); x <= area.right(); x += step) {
      if (qAlpha(image.pixel(x, y)) > 8 &&
          local.contains(QPointF(x + 0.5, y + 0.5)))
        return true;
    }
  }
  return false;
}
void PixelEditAction::apply(bool forward) {
  if (!itemStore_ || !itemId_.isValid())
    return;
  QGraphicsItem *item = itemStore_->item(itemId_);
  if (!item)
    return;
  if (tiled_) {
    auto *raster = dynamic_cast<RasterLayerItem *>(item);
    if (!raster)
      return;
    raster->surface().applyDiff(diff_, forward);
    raster->surfaceChanged();
    return;
  }
  setItemImage(item, forward ? after_ : before_);
}
