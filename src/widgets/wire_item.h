/**
 * @file wire_item.h
 * @brief A QGraphicsPathItem that represents an electrical wire between two
 *        connection pins on ElectronicsElementItem components.
 *
 * The wire automatically follows the connected elements when they are moved.
 * Selecting a wire auto-selects its connected elements so that drags,
 * rotations, and scales operate on the whole subcircuit.
 */
#ifndef WIRE_ITEM_H
#define WIRE_ITEM_H

#include <QGraphicsPathItem>
#include <QPainter>
#include <QPen>

class ElectronicsElementItem;
enum class PinDir;

/**
 * @brief Graphics item representing a wire between two electronics pins.
 *
 * A WireItem connects a source element/pin to a destination element/pin
 * and draws a Manhattan-routed path between the two pin positions.
 * When either element is moved the wire updates automatically (driven by
 * ElectronicsElementItem::itemChange).
 *
 * Selecting a wire automatically co-selects the connected elements so
 * that drag, rotate, scale, and other transforms work on the subcircuit
 * as a unit.
 */
class WireItem : public QGraphicsPathItem {
public:
  WireItem(ElectronicsElementItem *srcElem, int srcPin,
           ElectronicsElementItem *dstElem, int dstPin,
           QGraphicsItem *parent = nullptr);
  ~WireItem() override;

  /// Re-calculate the wire path from the current pin scene positions.
  void updatePath();

  /// Called by an element's destructor to clear dangling pointers.
  void detachElement(ElectronicsElementItem *elem);

  /// Accessors ---------------------------------------------------------------
  ElectronicsElementItem *sourceElement() const { return srcElem_; }
  int sourcePin() const { return srcPin_; }
  ElectronicsElementItem *destElement() const { return dstElem_; }
  int destPin() const { return dstPin_; }

  /// Build a direction-aware Manhattan path between two points.
  static QPainterPath routeManhattan(const QPointF &p1, PinDir d1,
                                     const QPointF &p2, PinDir d2);

  /// Custom type for qgraphicsitem_cast.
  enum { Type = UserType + 200 };
  int type() const override { return Type; }

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  QPainterPath shape() const override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

private:
  ElectronicsElementItem *srcElem_;
  int srcPin_;
  ElectronicsElementItem *dstElem_;
  int dstPin_;
  bool cachedDark_ = false;
};

#endif // WIRE_ITEM_H
