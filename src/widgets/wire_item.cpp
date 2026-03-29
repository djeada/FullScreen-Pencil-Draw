/**
 * @file wire_item.cpp
 * @brief Implementation of WireItem – the graphical wire connecting two
 *        electronics element pins.
 */
#include "wire_item.h"
#include "../core/theme_manager.h"
#include "electronics_elements.h"
#include <QPainterPath>
#include <QtMath>

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WireItem::WireItem(ElectronicsElementItem *srcElem, int srcPin,
                   ElectronicsElementItem *dstElem, int dstPin,
                   QGraphicsItem *parent)
    : QGraphicsPathItem(parent), srcElem_(srcElem), srcPin_(srcPin),
      dstElem_(dstElem), dstPin_(dstPin) {
  setFlags(QGraphicsItem::ItemIsSelectable);
  cachedDark_ = ThemeManager::instance().isDarkTheme();
  const QColor wireColor = cachedDark_ ? Qt::white : Qt::black;
  setPen(QPen(wireColor, 1.2, Qt::SolidLine, Qt::SquareCap,
              Qt::MiterJoin));
  setZValue(-1.0); // draw behind elements

  if (srcElem_)
    srcElem_->addWire(this);
  if (dstElem_)
    dstElem_->addWire(this);

  updatePath();
}

WireItem::~WireItem() {
  if (srcElem_)
    srcElem_->removeWire(this);
  if (dstElem_)
    dstElem_->removeWire(this);
}

void WireItem::detachElement(ElectronicsElementItem *elem) {
  if (srcElem_ == elem)
    srcElem_ = nullptr;
  if (dstElem_ == elem)
    dstElem_ = nullptr;
}

// ---------------------------------------------------------------------------
// Path calculation
// ---------------------------------------------------------------------------

void WireItem::updatePath() {
  if (!srcElem_ || !dstElem_)
    return;

  const QPointF p1 = srcElem_->pinScenePos(srcPin_);
  const QPointF p2 = dstElem_->pinScenePos(dstPin_);

  QPainterPath pp;
  pp.moveTo(p1);

  // Simple Manhattan routing: horizontal from source, vertical, then
  // horizontal to destination.
  const qreal midX = (p1.x() + p2.x()) / 2.0;
  pp.lineTo(midX, p1.y());
  pp.lineTo(midX, p2.y());
  pp.lineTo(p2);

  setPath(pp);
}

void WireItem::paint(QPainter *painter,
                     const QStyleOptionGraphicsItem *option,
                     QWidget *widget) {
  const bool dark = ThemeManager::instance().isDarkTheme();
  if (dark != cachedDark_) {
    cachedDark_ = dark;
    QPen p = pen();
    p.setColor(dark ? Qt::white : Qt::black);
    setPen(p);
  }
  QGraphicsPathItem::paint(painter, option, widget);
}
