/**
 * @file architecture_elements.cpp
 * @brief Implementation of custom vector-drawn architecture diagram elements.
 */
#include "architecture_elements.h"
#include <QPainterPath>
#include <QPolygonF>
#include <QtMath>

// ===========================================================================
// ArchitectureElementItem  (base)
// ===========================================================================

ArchitectureElementItem::ArchitectureElementItem(const QString &label,
                                                 QGraphicsItem *parent)
    : QGraphicsItem(parent), label_(label) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
}

QRectF ArchitectureElementItem::boundingRect() const {
  return QRectF(0, 0, ELEM_W, ELEM_H);
}

void ArchitectureElementItem::paint(QPainter *painter,
                                    const QStyleOptionGraphicsItem * /*option*/,
                                    QWidget * /*widget*/) {
  painter->setRenderHint(QPainter::Antialiasing, true);

  // --- background ---
  QPainterPath bg;
  bg.addRoundedRect(0, 0, ELEM_W, ELEM_H, CORNER, CORNER);
  painter->setPen(QPen(QColor("#3b82f6"), 2));
  painter->setBrush(QColor("#23232a"));
  painter->drawPath(bg);

  // --- selection highlight ---
  if (isSelected()) {
    painter->setPen(QPen(QColor("#60a5fa"), 2, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(QRectF(1, 1, ELEM_W - 2, ELEM_H - 2), CORNER,
                             CORNER);
  }

  // --- icon area (centred, upper portion) ---
  qreal iconX = (ELEM_W - ICON_SIZE) / 2.0;
  qreal iconY = 8.0;
  QRectF iconRect(iconX, iconY, ICON_SIZE, ICON_SIZE);
  paintIcon(painter, iconRect);

  // --- label ---
  painter->setPen(QColor("#a0a0a8"));
  QFont font("Arial", 10, QFont::Bold);
  painter->setFont(font);
  QRectF labelRect(0, ELEM_H - 26, ELEM_W, 20);
  painter->drawText(labelRect, Qt::AlignCenter, label_);
}

// ===========================================================================
// ClientElement – monitor / laptop shape
// ===========================================================================

ClientElement::ClientElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Client", parent) {}

void ClientElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#60a5fa"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#1e3a5f"));

  // Screen (wider aspect ratio for a modern monitor)
  qreal sw = r.width() * 0.82;
  qreal sh = r.height() * 0.52;
  qreal sx = r.x() + (r.width() - sw) / 2.0;
  qreal sy = r.y() + 2;
  p->drawRoundedRect(QRectF(sx, sy, sw, sh), 3, 3);

  // Inner screen bezel highlight
  p->setPen(Qt::NoPen);
  p->setBrush(QColor("#2563eb"));
  p->drawRoundedRect(QRectF(sx + 2, sy + 2, sw - 4, sh - 4), 2, 2);

  // Neck
  p->setPen(pen);
  p->setBrush(QColor("#1e3a5f"));
  qreal cx = r.x() + r.width() / 2.0;
  qreal standTop = sy + sh;
  qreal neckW = r.width() * 0.10;
  qreal neckH = r.height() * 0.16;
  p->drawRect(QRectF(cx - neckW / 2, standTop, neckW, neckH));

  // Base (elliptical for realism)
  qreal baseY = standTop + neckH;
  qreal bw = r.width() * 0.44;
  qreal bh = r.height() * 0.08;
  p->drawEllipse(QRectF(cx - bw / 2, baseY, bw, bh));
  p->restore();
}

// ===========================================================================
// LoadBalancerElement – scale / balance icon
// ===========================================================================

LoadBalancerElement::LoadBalancerElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Load Balancer", parent) {}

void LoadBalancerElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#f59e0b"), 1.6);
  p->setPen(pen);
  p->setBrush(Qt::NoBrush);

  qreal cx = r.center().x();
  qreal top = r.top() + 2;
  qreal bot = r.bottom() - 3;

  // Pivot triangle at top
  qreal triH = 5;
  QPolygonF pivot;
  pivot << QPointF(cx, top) << QPointF(cx - 4, top + triH)
        << QPointF(cx + 4, top + triH);
  p->setBrush(QColor("#f59e0b"));
  p->drawPolygon(pivot);
  p->setBrush(Qt::NoBrush);

  // Vertical pillar
  qreal pillarTop = top + triH;
  p->drawLine(QPointF(cx, pillarTop), QPointF(cx, bot));

  // Horizontal beam
  qreal beamY = pillarTop + 3;
  qreal half = r.width() * 0.38;
  p->setPen(QPen(QColor("#f59e0b"), 2.0));
  p->drawLine(QPointF(cx - half, beamY), QPointF(cx + half, beamY));

  // Left pan (arc + chains)
  p->setPen(pen);
  qreal panW = 14;
  qreal panH = 10;
  p->drawLine(QPointF(cx - half, beamY), QPointF(cx - half, beamY + 8));
  QPainterPath leftArc;
  QRectF leftR(cx - half - panW / 2, beamY + 8, panW, panH);
  leftArc.arcMoveTo(leftR, 0);
  leftArc.arcTo(leftR, 0, -180);
  p->drawPath(leftArc);

  // Right pan (arc + chains)
  p->drawLine(QPointF(cx + half, beamY), QPointF(cx + half, beamY + 8));
  QPainterPath rightArc;
  QRectF rightR(cx + half - panW / 2, beamY + 8, panW, panH);
  rightArc.arcMoveTo(rightR, 0);
  rightArc.arcTo(rightR, 0, -180);
  p->drawPath(rightArc);

  // Base
  qreal bw = r.width() * 0.22;
  qreal bh = 3;
  p->setBrush(QColor("#f59e0b"));
  p->drawRoundedRect(QRectF(cx - bw, bot, bw * 2, bh), 1, 1);
  p->restore();
}

// ===========================================================================
// ApiGatewayElement – gateway / arch shape
// ===========================================================================

ApiGatewayElement::ApiGatewayElement(QGraphicsItem *parent)
    : ArchitectureElementItem("API Gateway", parent) {}

void ApiGatewayElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#8b5cf6"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#2e1065"));

  // Arch
  QPainterPath arch;
  qreal aw = r.width() * 0.82;
  qreal ah = r.height() * 0.75;
  qreal ax = r.x() + (r.width() - aw) / 2.0;
  qreal ay = r.y() + 2;

  arch.moveTo(ax, ay + ah);
  arch.lineTo(ax, ay + ah * 0.38);
  arch.arcTo(QRectF(ax, ay, aw, ah * 0.76), 180, -180);
  arch.lineTo(ax + aw, ay + ah);

  // Columns
  qreal colW = aw * 0.14;
  arch.addRect(ax, ay + ah * 0.38, colW, ah * 0.62);
  arch.addRect(ax + aw - colW, ay + ah * 0.38, colW, ah * 0.62);

  p->drawPath(arch);

  // Step / base
  p->setPen(QPen(QColor("#8b5cf6"), 1.2));
  p->drawLine(QPointF(ax - 2, ay + ah), QPointF(ax + aw + 2, ay + ah));

  // Arrow through the gate
  p->setPen(QPen(QColor("#a78bfa"), 1.6));
  qreal arrowY = r.y() + r.height() * 0.52;
  qreal al = ax + aw * 0.24;
  qreal ar2 = ax + aw * 0.76;
  p->drawLine(QPointF(al, arrowY), QPointF(ar2, arrowY));
  // Arrowhead
  p->drawLine(QPointF(ar2, arrowY), QPointF(ar2 - 5, arrowY - 3.5));
  p->drawLine(QPointF(ar2, arrowY), QPointF(ar2 - 5, arrowY + 3.5));
  p->restore();
}

// ===========================================================================
// AppServerElement – hexagonal service node
// ===========================================================================

AppServerElement::AppServerElement(QGraphicsItem *parent)
    : ArchitectureElementItem("App Server", parent) {}

void AppServerElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#10b981"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#064e3b"));

  qreal cx = r.center().x();
  qreal cy = r.center().y();
  qreal rad = qMin(r.width(), r.height()) * 0.44;

  QPolygonF hex;
  for (int i = 0; i < 6; ++i) {
    qreal angle = M_PI / 6.0 + i * M_PI / 3.0;
    hex << QPointF(cx + rad * qCos(angle), cy + rad * qSin(angle));
  }
  p->drawPolygon(hex);

  // Gear-like inner circle
  p->setPen(QPen(QColor("#34d399"), 1.4));
  p->setBrush(Qt::NoBrush);
  p->drawEllipse(QPointF(cx, cy), rad * 0.42, rad * 0.42);

  // Gear teeth (small lines radiating outward from the inner circle)
  qreal innerR = rad * 0.42;
  qreal toothLen = rad * 0.14;
  for (int i = 0; i < 8; ++i) {
    qreal angle = i * M_PI / 4.0;
    p->drawLine(QPointF(cx + innerR * qCos(angle), cy + innerR * qSin(angle)),
                QPointF(cx + (innerR + toothLen) * qCos(angle),
                        cy + (innerR + toothLen) * qSin(angle)));
  }

  // Inner dot
  p->setBrush(QColor("#34d399"));
  p->drawEllipse(QPointF(cx, cy), 3.0, 3.0);
  p->restore();
}

// ===========================================================================
// CacheElement – lightning bolt / fast memory
// ===========================================================================

CacheElement::CacheElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Cache", parent) {}

void CacheElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#eab308"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#854d0e"));

  // Rounded square background
  qreal bx = r.x() + 3;
  qreal by = r.y() + 2;
  qreal bw = r.width() - 6;
  qreal bh = r.height() - 4;
  p->drawRoundedRect(QRectF(bx, by, bw, bh), 5, 5);

  // Lightning bolt
  p->setPen(QPen(QColor("#fde047"), 2.2));
  p->setBrush(QColor("#fde047"));
  qreal cx = r.center().x();
  qreal top = r.top() + 8;
  qreal bot = r.bottom() - 8;
  qreal mid = (top + bot) / 2.0;
  QPolygonF bolt;
  bolt << QPointF(cx + 3, top) << QPointF(cx - 6, mid + 1.5)
       << QPointF(cx, mid + 1.5) << QPointF(cx - 3, bot)
       << QPointF(cx + 6, mid - 1.5) << QPointF(cx, mid - 1.5);
  p->drawPolygon(bolt);
  p->restore();
}

// ===========================================================================
// MessageQueueElement – queue / pipe with arrows
// ===========================================================================

MessageQueueElement::MessageQueueElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Queue", parent) {}

void MessageQueueElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#ec4899"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#831843"));

  // Pipe body
  qreal px = r.x() + 3;
  qreal py = r.center().y() - 10;
  qreal pw = r.width() - 6;
  qreal ph = 20;
  p->drawRoundedRect(QRectF(px, py, pw, ph), 8, 8);

  // Enqueue arrow (left)
  p->setPen(QPen(QColor("#f9a8d4"), 1.6));
  qreal ay = r.center().y();
  p->drawLine(QPointF(px - 3, ay), QPointF(px + 8, ay));
  p->drawLine(QPointF(px + 8, ay), QPointF(px + 4, ay - 3.5));
  p->drawLine(QPointF(px + 8, ay), QPointF(px + 4, ay + 3.5));

  // Dequeue arrow (right)
  qreal rx = px + pw;
  p->drawLine(QPointF(rx - 8, ay), QPointF(rx + 3, ay));
  p->drawLine(QPointF(rx + 3, ay), QPointF(rx - 1, ay - 3.5));
  p->drawLine(QPointF(rx + 3, ay), QPointF(rx - 1, ay + 3.5));

  // Messages inside pipe
  p->setPen(Qt::NoPen);
  p->setBrush(QColor("#f9a8d4"));
  for (int i = 0; i < 4; ++i) {
    qreal mx = px + 10 + i * 8;
    p->drawRoundedRect(QRectF(mx, py + 5, 5, 10), 1, 1);
  }
  p->restore();
}

// ===========================================================================
// DatabaseElement – cylinder
// ===========================================================================

DatabaseElement::DatabaseElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Database", parent) {}

void DatabaseElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#06b6d4"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#164e63"));

  qreal cx = r.center().x();
  qreal cw = r.width() * 0.72;
  qreal ch = r.height() * 0.80;
  qreal cLeft = cx - cw / 2;
  qreal cTop = r.top() + 2;
  qreal ellH = ch * 0.24;

  // Cylinder body (curved sides connecting top and bottom ellipses)
  p->drawRect(QRectF(cLeft, cTop + ellH / 2, cw, ch - ellH));

  // Bottom ellipse (full)
  p->setBrush(QColor("#164e63"));
  p->drawEllipse(QRectF(cLeft, cTop + ch - ellH, cw, ellH));

  // Top ellipse (full, drawn last so it covers the rect top)
  p->setBrush(QColor("#0e7490"));
  p->drawEllipse(QRectF(cLeft, cTop, cw, ellH));

  // Middle decorative line
  p->setPen(QPen(QColor("#06b6d4"), 1.0));
  qreal midY = cTop + ch * 0.38;
  QPainterPath midArc;
  QRectF midR(cLeft, midY - ellH / 2, cw, ellH);
  midArc.arcMoveTo(midR, 0);
  midArc.arcTo(midR, 0, -180);
  p->drawPath(midArc);

  // Second decorative line
  qreal midY2 = cTop + ch * 0.58;
  QPainterPath midArc2;
  QRectF midR2(cLeft, midY2 - ellH / 2, cw, ellH);
  midArc2.arcMoveTo(midR2, 0);
  midArc2.arcTo(midR2, 0, -180);
  p->drawPath(midArc2);
  p->restore();
}

// ===========================================================================
// ObjectStorageElement – folder / bucket icon
// ===========================================================================

ObjectStorageElement::ObjectStorageElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Storage", parent) {}

void ObjectStorageElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#f97316"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#7c2d12"));

  // Bucket shape with curved bottom
  qreal bx = r.x() + 5;
  qreal by = r.y() + 2;
  qreal bw = r.width() - 10;
  qreal bh = r.height() - 4;

  QPainterPath bucket;
  qreal topInset = 3;
  bucket.moveTo(bx + topInset, by);
  bucket.lineTo(bx + bw - topInset, by);
  bucket.lineTo(bx + bw, by + bh * 0.85);
  bucket.quadTo(bx + bw / 2.0, by + bh + 4, bx, by + bh * 0.85);
  bucket.closeSubpath();
  p->drawPath(bucket);

  // Rim at top
  p->setPen(QPen(QColor("#fb923c"), 1.4));
  p->drawLine(QPointF(bx + topInset - 1, by + 1),
              QPointF(bx + bw - topInset + 1, by + 1));

  // Horizontal bands
  p->setPen(QPen(QColor("#fb923c"), 1.0));
  for (int i = 1; i <= 2; ++i) {
    qreal ly = by + bh * i / 3.0;
    qreal inset = 3.0 * (1.0 - static_cast<qreal>(i) / 3.0);
    p->drawLine(QPointF(bx + inset, ly), QPointF(bx + bw - inset, ly));
  }

  // Small circles (objects)
  p->setPen(Qt::NoPen);
  p->setBrush(QColor("#fb923c"));
  p->drawEllipse(QPointF(r.center().x() - 6, by + bh * 0.48), 3.0, 3.0);
  p->drawEllipse(QPointF(r.center().x() + 6, by + bh * 0.48), 3.0, 3.0);
  p->drawEllipse(QPointF(r.center().x(), by + bh * 0.30), 2.5, 2.5);
  p->restore();
}

// ===========================================================================
// AuthElement – shield with keyhole
// ===========================================================================

AuthElement::AuthElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Auth", parent) {}

void AuthElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#ef4444"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#7f1d1d"));

  // Shield outline
  qreal cx = r.center().x();
  qreal sw = r.width() * 0.72;
  qreal sh = r.height() * 0.88;
  qreal sx = cx - sw / 2;
  qreal sy = r.top() + 1;

  QPainterPath shield;
  shield.moveTo(cx, sy);
  shield.lineTo(sx + sw, sy + sh * 0.16);
  shield.quadTo(sx + sw, sy + sh * 0.68, cx, sy + sh);
  shield.quadTo(sx, sy + sh * 0.68, sx, sy + sh * 0.16);
  shield.closeSubpath();
  p->drawPath(shield);

  // Inner shield highlight
  p->setPen(Qt::NoPen);
  p->setBrush(QColor("#991b1b"));
  qreal inset = 3;
  QPainterPath inner;
  inner.moveTo(cx, sy + inset);
  inner.lineTo(sx + sw - inset, sy + sh * 0.16 + inset * 0.5);
  inner.quadTo(sx + sw - inset, sy + sh * 0.68, cx, sy + sh - inset);
  inner.quadTo(sx + inset, sy + sh * 0.68, sx + inset,
               sy + sh * 0.16 + inset * 0.5);
  inner.closeSubpath();
  p->drawPath(inner);

  // Keyhole
  p->setPen(QPen(QColor("#fca5a5"), 1.4));
  p->setBrush(QColor("#fca5a5"));
  qreal kcy = r.center().y() - 2;
  p->drawEllipse(QPointF(cx, kcy), 4.0, 4.0);
  QPolygonF keySlot;
  keySlot << QPointF(cx - 2.5, kcy + 3) << QPointF(cx + 2.5, kcy + 3)
          << QPointF(cx + 1.5, kcy + 10) << QPointF(cx - 1.5, kcy + 10);
  p->drawPolygon(keySlot);
  p->restore();
}

// ===========================================================================
// MonitoringElement – chart / graph icon
// ===========================================================================

MonitoringElement::MonitoringElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Monitor", parent) {}

void MonitoringElement::paintIcon(QPainter *p, const QRectF &r) const {
  p->save();
  QPen pen(QColor("#14b8a6"), 1.6);
  p->setPen(pen);
  p->setBrush(QColor("#134e4a"));

  // Chart background
  qreal cx = r.x() + 3;
  qreal cy = r.y() + 2;
  qreal cw = r.width() - 6;
  qreal ch = r.height() - 4;
  p->drawRoundedRect(QRectF(cx, cy, cw, ch), 4, 4);

  // Axes
  p->setPen(QPen(QColor("#5eead4"), 1.4));
  qreal axLeft = cx + 6;
  qreal axBot = cy + ch - 6;
  qreal axRight = cx + cw - 6;
  qreal axTop = cy + 6;
  p->drawLine(QPointF(axLeft, axTop), QPointF(axLeft, axBot));
  p->drawLine(QPointF(axLeft, axBot), QPointF(axRight, axBot));

  // Grid lines (subtle)
  p->setPen(QPen(QColor("#5eead4"), 0.4));
  for (int i = 1; i <= 3; ++i) {
    qreal gy = axBot - (axBot - axTop) * i / 4.0;
    p->drawLine(QPointF(axLeft + 1, gy), QPointF(axRight, gy));
  }

  // Line chart
  p->setPen(QPen(QColor("#2dd4bf"), 1.8));
  QVector<QPointF> points;
  qreal dx = (axRight - axLeft) / 6.0;
  qreal heights[] = {0.55, 0.3, 0.72, 0.2, 0.55, 0.40, 0.62};
  for (int i = 0; i <= 6; ++i) {
    qreal x = axLeft + i * dx;
    qreal y = axBot - (axBot - axTop) * heights[i];
    points.append(QPointF(x, y));
  }
  for (int i = 0; i < points.size() - 1; ++i) {
    p->drawLine(points[i], points[i + 1]);
  }

  // Data dots
  p->setPen(Qt::NoPen);
  p->setBrush(QColor("#5eead4"));
  for (const auto &pt : points) {
    p->drawEllipse(pt, 2.5, 2.5);
  }
  p->restore();
}
