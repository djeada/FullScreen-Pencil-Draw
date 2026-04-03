/**
 * @file wire_item.cpp
 * @brief Implementation of WireItem – the graphical wire connecting two
 *        electronics element pins.
 */
#include "wire_item.h"
#include "../core/theme_manager.h"
#include "electronics_elements.h"
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#include <QTimer>
#include <QtMath>

// ---------------------------------------------------------------------------
// Routing helpers
// ---------------------------------------------------------------------------

static constexpr qreal STUB = 20.0; // length of the stub leaving each pin
static constexpr qreal DOT_R = 2.8; // junction-dot radius

/// Unit vector for a PinDir.
static QPointF dirVec(PinDir d) {
  switch (d) {
  case PinDir::Left:  return {-1.0,  0.0};
  case PinDir::Right: return { 1.0,  0.0};
  case PinDir::Up:    return { 0.0, -1.0};
  case PinDir::Down:  return { 0.0,  1.0};
  }
  return {1.0, 0.0};
}

static bool isHoriz(PinDir d) {
  return d == PinDir::Left || d == PinDir::Right;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WireItem::WireItem(ElectronicsElementItem *srcElem, int srcPin,
                   ElectronicsElementItem *dstElem, int dstPin,
                   QGraphicsItem *parent)
    : QGraphicsPathItem(parent), srcElem_(srcElem), srcPin_(srcPin),
      dstElem_(dstElem), dstPin_(dstPin) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable |
           QGraphicsItem::ItemSendsGeometryChanges);
  cachedDark_ = ThemeManager::instance().isDarkTheme();
  const QColor wireColor = cachedDark_ ? Qt::white : Qt::black;
  setPen(QPen(wireColor, 1.2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
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
// Interaction
// ---------------------------------------------------------------------------

QVariant WireItem::itemChange(GraphicsItemChange change,
                              const QVariant &value) {
  if (change == ItemSelectedHasChanged && value.toBool()) {
    // Defer co-selection to avoid mutating scene selection mid-iteration.
    ElectronicsElementItem *src = srcElem_;
    ElectronicsElementItem *dst = dstElem_;
    QTimer::singleShot(0, [src, dst]() {
      if (src)
        src->setSelected(true);
      if (dst)
        dst->setSelected(true);
    });
  }
  if (change == ItemPositionChange) {
    // The wire must stay at the scene origin – its path is in scene
    // coordinates and is recalculated from pin positions by updatePath().
    // Qt's built-in drag moves the co-selected elements, which trigger
    // updatePath() via their own itemChange, keeping the wire in sync.
    return QPointF(0, 0);
  }
  return QGraphicsPathItem::itemChange(change, value);
}

QPainterPath WireItem::shape() const {
  QPainterPathStroker stroker;
  stroker.setWidth(8.0);
  return stroker.createStroke(path());
}

// ---------------------------------------------------------------------------
// Static routing algorithm
// ---------------------------------------------------------------------------

/**
 * Build a clean Manhattan path between two directed pin endpoints.
 *
 * Each pin has a stub that extends outward in its PinDir by STUB pixels.
 * The routing then connects the two stub endpoints differently depending
 * on the direction combination:
 *
 *   - Perpendicular: single elbow.
 *   - Converging (facing each other) with room: Z-step at midpoint.
 *   - Converging without room (stubs cross): U-route above/below.
 *   - Same direction: jog at the farthest extent.
 *   - Diverging (facing away): U-route above/below.
 */
QPainterPath WireItem::routeManhattan(const QPointF &p1, PinDir d1,
                                      const QPointF &p2, PinDir d2) {
  const QPointF s1 = p1 + dirVec(d1) * STUB;
  const QPointF s2 = p2 + dirVec(d2) * STUB;

  QPainterPath pp;
  pp.moveTo(p1);
  pp.lineTo(s1);

  const bool h1 = isHoriz(d1);
  const bool h2 = isHoriz(d2);

  if (h1 != h2) {
    // ── Perpendicular: single elbow ──
    if (h1)
      pp.lineTo(s2.x(), s1.y());
    else
      pp.lineTo(s1.x(), s2.y());

  } else if (h1 /* && h2 */) {
    // ── Both horizontal ──
    const bool sameDir = (d1 == d2);

    if (sameDir) {
      // Same direction: jog at the farthest stub extent.
      // e.g. both Right → connect at the rightmost X of the two stubs.
      const qreal jx = (d1 == PinDir::Right)
                            ? qMax(s1.x(), s2.x())
                            : qMin(s1.x(), s2.x());
      pp.lineTo(jx, s1.y());
      pp.lineTo(jx, s2.y());

    } else {
      // Opposing directions (Right↔Left).
      // "Converging" when stubs point toward each other with room.
      const bool converging =
          (d1 == PinDir::Right) ? (s1.x() + 1 < s2.x())
                                : (s2.x() + 1 < s1.x());
      if (converging) {
        // Z-step through midpoint between stubs.
        const qreal mx = (s1.x() + s2.x()) / 2.0;
        pp.lineTo(mx, s1.y());
        pp.lineTo(mx, s2.y());
      } else {
        // Stubs crossed or diverging: bypass via a Y-offset channel.
        const qreal yMin = qMin(p1.y(), p2.y());
        const qreal yMax = qMax(p1.y(), p2.y());
        const qreal above = yMin - STUB * 2.5;
        const qreal below = yMax + STUB * 2.5;
        const qreal mid = (p1.y() + p2.y()) / 2.0;
        const qreal by = (qAbs(above - mid) <= qAbs(below - mid))
                              ? above : below;
        pp.lineTo(s1.x(), by);
        pp.lineTo(s2.x(), by);
      }
    }

  } else {
    // ── Both vertical ──
    const bool sameDir = (d1 == d2);

    if (sameDir) {
      const qreal jy = (d1 == PinDir::Down)
                            ? qMax(s1.y(), s2.y())
                            : qMin(s1.y(), s2.y());
      pp.lineTo(s1.x(), jy);
      pp.lineTo(s2.x(), jy);

    } else {
      const bool converging =
          (d1 == PinDir::Down) ? (s1.y() + 1 < s2.y())
                               : (s2.y() + 1 < s1.y());
      if (converging) {
        const qreal my = (s1.y() + s2.y()) / 2.0;
        pp.lineTo(s1.x(), my);
        pp.lineTo(s2.x(), my);
      } else {
        const qreal xMin = qMin(p1.x(), p2.x());
        const qreal xMax = qMax(p1.x(), p2.x());
        const qreal left  = xMin - STUB * 2.5;
        const qreal right = xMax + STUB * 2.5;
        const qreal mid = (p1.x() + p2.x()) / 2.0;
        const qreal bx = (qAbs(left - mid) <= qAbs(right - mid))
                              ? left : right;
        pp.lineTo(bx, s1.y());
        pp.lineTo(bx, s2.y());
      }
    }
  }

  pp.lineTo(s2);
  pp.lineTo(p2);
  return pp;
}

// ---------------------------------------------------------------------------
// Path calculation
// ---------------------------------------------------------------------------

void WireItem::updatePath() {
  if (!srcElem_ || !dstElem_)
    return;

  const QPointF p1 = srcElem_->pinScenePos(srcPin_);
  const QPointF p2 = dstElem_->pinScenePos(dstPin_);
  const PinDir  d1 = srcElem_->pinDir(srcPin_);
  const PinDir  d2 = dstElem_->pinDir(dstPin_);

  setPath(routeManhattan(p1, d1, p2, d2));
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void WireItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                     QWidget *widget) {
  const bool dark = ThemeManager::instance().isDarkTheme();
  if (dark != cachedDark_) {
    cachedDark_ = dark;
    QPen p = pen();
    p.setColor(dark ? Qt::white : Qt::black);
    setPen(p);
  }

  // Draw a selection highlight glow behind the wire.
  if (option->state & QStyle::State_Selected) {
    QPen hlPen(QColor(34, 211, 238, 100), 6.0, Qt::SolidLine, Qt::RoundCap,
               Qt::RoundJoin);
    painter->setPen(hlPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());
  }

  // Suppress Qt's default dashed selection rect – paint the path manually.
  QStyleOptionGraphicsItem optionNoSel(*option);
  optionNoSel.state &= ~QStyle::State_Selected;
  QGraphicsPathItem::paint(painter, &optionNoSel, widget);

  // Junction dots at both endpoints.
  const QPainterPath &pp = path();
  if (pp.elementCount() < 2)
    return;
  const QColor dotColor = pen().color();
  painter->setPen(Qt::NoPen);
  painter->setBrush(dotColor);
  const QPointF a = pp.elementAt(0);
  const QPointF b = pp.elementAt(pp.elementCount() - 1);
  painter->drawEllipse(a, DOT_R, DOT_R);
  painter->drawEllipse(b, DOT_R, DOT_R);
}
