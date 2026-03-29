/**
 * @file wire_item.h
 * @brief A QGraphicsPathItem that represents an electrical wire between two
 *        connection pins on ElectronicsElementItem components.
 *
 * The wire automatically follows the connected elements when they are moved.
 */
#ifndef WIRE_ITEM_H
#define WIRE_ITEM_H

#include <QGraphicsPathItem>
#include <QPainter>
#include <QPen>

class ElectronicsElementItem;

/**
 * @brief Graphics item representing a wire between two electronics pins.
 *
 * A WireItem connects a source element/pin to a destination element/pin
 * and draws either a straight line or a simple Manhattan-routed path
 * between the two pin positions. When either element is moved the wire
 * updates automatically (driven by ElectronicsElementItem::itemChange).
 */
class WireItem : public QGraphicsPathItem {
public:
  /**
   * @brief Construct a wire between two pins.
   * @param srcElem  Source element.
   * @param srcPin   Index into srcElem->pins().
   * @param dstElem  Destination element.
   * @param dstPin   Index into dstElem->pins().
   * @param parent   Optional parent item.
   */
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

  /// Custom type for qgraphicsitem_cast.
  enum { Type = UserType + 200 };
  int type() const override { return Type; }

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ElectronicsElementItem *srcElem_;
  int srcPin_;
  ElectronicsElementItem *dstElem_;
  int dstPin_;
  bool cachedDark_ = false;
};

#endif // WIRE_ITEM_H
