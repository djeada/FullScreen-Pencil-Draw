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
  qreal iconY = 6.0;
  QRectF iconRect(iconX, iconY, ICON_SIZE, ICON_SIZE);
  paintIcon(painter, iconRect);

  // --- label ---
  painter->setPen(QColor("#a0a0a8"));
  QFont font("Arial", 9, QFont::Bold);
  painter->setFont(font);
  QRectF labelRect(0, ELEM_H - 22, ELEM_W, 18);
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

  // Screen
  qreal sw = r.width() * 0.78;
  qreal sh = r.height() * 0.55;
  qreal sx = r.x() + (r.width() - sw) / 2.0;
  qreal sy = r.y() + 2;
  p->drawRoundedRect(QRectF(sx, sy, sw, sh), 2, 2);

  // Stand
  qreal cx = r.x() + r.width() / 2.0;
  qreal standTop = sy + sh;
  qreal standBot = standTop + r.height() * 0.18;
  p->drawLine(QPointF(cx, standTop), QPointF(cx, standBot));

  // Base
  qreal bw = r.width() * 0.4;
  p->drawLine(QPointF(cx - bw / 2, standBot), QPointF(cx + bw / 2, standBot));
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
  qreal top = r.top() + 3;
  qreal bot = r.bottom() - 3;

  // Vertical pillar
  p->drawLine(QPointF(cx, top), QPointF(cx, bot));

  // Horizontal beam
  qreal beamY = top + 5;
  qreal half = r.width() * 0.4;
  p->drawLine(QPointF(cx - half, beamY), QPointF(cx + half, beamY));

  // Left pan (arc)
  QPainterPath leftArc;
  QRectF leftR(cx - half - 6, beamY, 12, 14);
  leftArc.arcMoveTo(leftR, 0);
  leftArc.arcTo(leftR, 0, -180);
  p->drawPath(leftArc);

  // Right pan (arc)
  QPainterPath rightArc;
  QRectF rightR(cx + half - 6, beamY, 12, 14);
  rightArc.arcMoveTo(rightR, 0);
  rightArc.arcTo(rightR, 0, -180);
  p->drawPath(rightArc);

  // Base triangle
  qreal bw = r.width() * 0.25;
  QPolygonF base;
  base << QPointF(cx, bot) << QPointF(cx - bw, bot + 2)
       << QPointF(cx + bw, bot + 2);
  p->drawPolyline(base);
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
  qreal aw = r.width() * 0.8;
  qreal ah = r.height() * 0.7;
  qreal ax = r.x() + (r.width() - aw) / 2.0;
  qreal ay = r.y() + 2;

  arch.moveTo(ax, ay + ah);
  arch.lineTo(ax, ay + ah * 0.4);
  arch.arcTo(QRectF(ax, ay, aw, ah * 0.8), 180, -180);
  arch.lineTo(ax + aw, ay + ah);

  // Columns
  qreal colW = aw * 0.15;
  arch.addRect(ax, ay + ah * 0.4, colW, ah * 0.6);
  arch.addRect(ax + aw - colW, ay + ah * 0.4, colW, ah * 0.6);

  p->drawPath(arch);

  // Arrow through the gate
  p->setPen(QPen(QColor("#a78bfa"), 1.4));
  qreal arrowY = r.y() + r.height() * 0.55;
  qreal al = ax + aw * 0.25;
  qreal ar2 = ax + aw * 0.75;
  p->drawLine(QPointF(al, arrowY), QPointF(ar2, arrowY));
  // Arrowhead
  p->drawLine(QPointF(ar2, arrowY), QPointF(ar2 - 4, arrowY - 3));
  p->drawLine(QPointF(ar2, arrowY), QPointF(ar2 - 4, arrowY + 3));
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
  qreal rad = qMin(r.width(), r.height()) * 0.42;

  QPolygonF hex;
  for (int i = 0; i < 6; ++i) {
    qreal angle = M_PI / 6.0 + i * M_PI / 3.0;
    hex << QPointF(cx + rad * qCos(angle), cy + rad * qSin(angle));
  }
  p->drawPolygon(hex);

  // Gear-like inner circle
  p->setPen(QPen(QColor("#34d399"), 1.2));
  p->setBrush(Qt::NoBrush);
  p->drawEllipse(QPointF(cx, cy), rad * 0.35, rad * 0.35);

  // Inner dot
  p->setBrush(QColor("#34d399"));
  p->drawEllipse(QPointF(cx, cy), 2.5, 2.5);
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
  p->drawRoundedRect(QRectF(bx, by, bw, bh), 4, 4);

  // Lightning bolt
  p->setPen(QPen(QColor("#fde047"), 2));
  p->setBrush(QColor("#fde047"));
  qreal cx = r.center().x();
  qreal top = r.top() + 7;
  qreal bot = r.bottom() - 7;
  qreal mid = (top + bot) / 2.0;
  QPolygonF bolt;
  bolt << QPointF(cx + 2, top) << QPointF(cx - 5, mid + 1)
       << QPointF(cx, mid + 1) << QPointF(cx - 2, bot)
       << QPointF(cx + 5, mid - 1) << QPointF(cx, mid - 1);
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
  qreal py = r.center().y() - 8;
  qreal pw = r.width() - 6;
  qreal ph = 16;
  p->drawRoundedRect(QRectF(px, py, pw, ph), 6, 6);

  // Enqueue arrow (left)
  p->setPen(QPen(QColor("#f9a8d4"), 1.4));
  qreal ay = r.center().y();
  p->drawLine(QPointF(px - 2, ay), QPointF(px + 7, ay));
  p->drawLine(QPointF(px + 7, ay), QPointF(px + 4, ay - 3));
  p->drawLine(QPointF(px + 7, ay), QPointF(px + 4, ay + 3));

  // Dequeue arrow (right)
  qreal rx = px + pw;
  p->drawLine(QPointF(rx - 7, ay), QPointF(rx + 2, ay));
  p->drawLine(QPointF(rx + 2, ay), QPointF(rx - 1, ay - 3));
  p->drawLine(QPointF(rx + 2, ay), QPointF(rx - 1, ay + 3));

  // Messages inside pipe
  p->setPen(Qt::NoPen);
  p->setBrush(QColor("#f9a8d4"));
  for (int i = 0; i < 3; ++i) {
    qreal mx = px + 9 + i * 8;
    p->drawRect(QRectF(mx, py + 4, 5, 8));
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
  qreal cw = r.width() * 0.7;
  qreal ch = r.height() * 0.75;
  qreal cLeft = cx - cw / 2;
  qreal cTop = r.top() + 3;
  qreal ellH = ch * 0.22;

  // Body
  p->drawRect(QRectF(cLeft, cTop + ellH / 2, cw, ch - ellH));

  // Top ellipse
  p->setBrush(QColor("#0e7490"));
  p->drawEllipse(QRectF(cLeft, cTop, cw, ellH));

  // Bottom ellipse
  p->setBrush(QColor("#164e63"));
  QPainterPath bottomArc;
  QRectF bottomR(cLeft, cTop + ch - ellH, cw, ellH);
  bottomArc.arcMoveTo(bottomR, 0);
  bottomArc.arcTo(bottomR, 0, -180);
  p->drawPath(bottomArc);

  // Middle line
  p->setPen(QPen(QColor("#06b6d4"), 1.0));
  qreal midY = cTop + ch * 0.4;
  QPainterPath midArc;
  QRectF midR(cLeft, midY - ellH / 2, cw, ellH);
  midArc.arcMoveTo(midR, 0);
  midArc.arcTo(midR, 0, -180);
  p->drawPath(midArc);
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

  // Bucket shape
  qreal bx = r.x() + 6;
  qreal by = r.y() + 3;
  qreal bw = r.width() - 12;
  qreal bh = r.height() - 6;

  QPainterPath bucket;
  bucket.moveTo(bx + 3, by);
  bucket.lineTo(bx + bw - 3, by);
  bucket.lineTo(bx + bw, by + bh);
  bucket.lineTo(bx, by + bh);
  bucket.closeSubpath();
  p->drawPath(bucket);

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
  p->drawEllipse(QPointF(r.center().x() - 5, by + bh * 0.5), 2.5, 2.5);
  p->drawEllipse(QPointF(r.center().x() + 5, by + bh * 0.5), 2.5, 2.5);
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
  qreal sw = r.width() * 0.7;
  qreal sh = r.height() * 0.82;
  qreal sx = cx - sw / 2;
  qreal sy = r.top() + 2;

  QPainterPath shield;
  shield.moveTo(cx, sy);
  shield.lineTo(sx + sw, sy + sh * 0.18);
  shield.quadTo(sx + sw, sy + sh * 0.7, cx, sy + sh);
  shield.quadTo(sx, sy + sh * 0.7, sx, sy + sh * 0.18);
  shield.closeSubpath();
  p->drawPath(shield);

  // Keyhole
  p->setPen(QPen(QColor("#fca5a5"), 1.4));
  p->setBrush(QColor("#fca5a5"));
  qreal kcy = r.center().y() - 1;
  p->drawEllipse(QPointF(cx, kcy), 3.5, 3.5);
  QPolygonF keySlot;
  keySlot << QPointF(cx - 2.2, kcy + 2) << QPointF(cx + 2.2, kcy + 2)
          << QPointF(cx + 1.2, kcy + 8) << QPointF(cx - 1.2, kcy + 8);
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
  qreal cx = r.x() + 4;
  qreal cy = r.y() + 2;
  qreal cw = r.width() - 8;
  qreal ch = r.height() - 4;
  p->drawRoundedRect(QRectF(cx, cy, cw, ch), 3, 3);

  // Axes
  p->setPen(QPen(QColor("#5eead4"), 1.2));
  qreal axLeft = cx + 5;
  qreal axBot = cy + ch - 5;
  qreal axRight = cx + cw - 5;
  qreal axTop = cy + 5;
  p->drawLine(QPointF(axLeft, axTop), QPointF(axLeft, axBot));
  p->drawLine(QPointF(axLeft, axBot), QPointF(axRight, axBot));

  // Line chart
  p->setPen(QPen(QColor("#2dd4bf"), 1.6));
  QVector<QPointF> points;
  qreal dx = (axRight - axLeft) / 5.0;
  qreal heights[] = {0.6, 0.3, 0.7, 0.2, 0.5, 0.35};
  for (int i = 0; i <= 5; ++i) {
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
    p->drawEllipse(pt, 2, 2);
  }
  p->restore();
}
