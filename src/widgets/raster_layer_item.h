/**
 * @file raster_layer_item.h
 * @brief Scene item presenting a RasterSurface, and undoable pixel edits.
 *
 * A RasterLayerItem is the editable pixel content of one layer. It lives
 * in the layer next to vector items (paths, shapes, text), so both media
 * stay editable in one document: painting and pixel-erasing change only
 * the touched tiles, never the vector objects or the layer itself.
 */
#ifndef RASTER_LAYER_ITEM_H
#define RASTER_LAYER_ITEM_H

#include "../core/action.h"
#include "../core/item_id.h"
#include "../core/raster_surface.h"
#include <QGraphicsItem>
#include <QImage>

class ItemStore;

class RasterLayerItem : public QGraphicsItem {
public:
  enum { Type = UserType + 300 };
  int type() const override { return Type; }

  explicit RasterLayerItem(QGraphicsItem *parent = nullptr);

  RasterSurface &surface() { return surface_; }
  const RasterSurface &surface() const { return surface_; }

  /// Call after changing the surface outside of paint/erase helpers so the
  /// scene learns about the new bounds and repaints.
  void surfaceChanged();

  /// Paint a round-capped line (item coordinates) onto the surface.
  void paintStroke(const QPointF &from, const QPointF &to, const QPen &pen);
  /// Erase an ellipse dab of @p diameter centred at @p center.
  void eraseDab(const QPointF &center, qreal diameter, qreal opacity,
                qreal hardness, const QPainterPath &clip = QPainterPath());

  QRectF boundingRect() const override;
  QPainterPath shape() const override;

protected:
  /// Keeps the content non-selectable and non-movable whatever other code
  /// (tool switches, layer unlock) sets: pixels are edited in place.
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

public:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  RasterSurface surface_;
  QRectF bounds_;
};

/**
 * @brief Undo step for a pixel edit: a tile diff on a RasterLayerItem, or
 *        a before/after image of a pixmap / brush-stroke item.
 *
 * Items are resolved through the ItemStore on every undo/redo (never a
 * cached pointer), so a deleted item simply makes the step a no-op.
 */
class PixelEditAction : public Action {
public:
  PixelEditAction(const ItemId &id, ItemStore *store, RasterTileDiff diff,
                  QString description = QStringLiteral("Pixel Edit"));
  PixelEditAction(const ItemId &id, ItemStore *store, const QImage &before,
                  const QImage &after,
                  QString description = QStringLiteral("Pixel Edit"));

  void undo() override { apply(false); }
  void redo() override { apply(true); }
  QString description() const override { return description_; }
  std::size_t memoryCost() const override;

  /// Read/write the raster pixels of a pixmap or brush-stroke item.
  static QImage itemImage(QGraphicsItem *item);
  static bool setItemImage(QGraphicsItem *item, const QImage &image);
  /// Whether @p dab (scene coordinates) covers visible (non-transparent)
  /// pixels of a pixmap or brush-stroke item.
  static bool hitsOpaquePixels(QGraphicsItem *item, const QPainterPath &dab);

private:
  void apply(bool forward);

  ItemId itemId_;
  ItemStore *itemStore_;
  bool tiled_;
  RasterTileDiff diff_;
  QImage before_;
  QImage after_;
  QString description_;
};

#endif // RASTER_LAYER_ITEM_H
