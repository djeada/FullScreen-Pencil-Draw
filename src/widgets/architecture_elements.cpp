/**
 * @file architecture_elements.cpp
 * @brief Implementation of custom vector-drawn architecture diagram elements.
 */
#include "architecture_elements.h"
#include <QFont>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>
#include <QtMath>

namespace {

QColor mixColor(const QColor &a, const QColor &b, qreal ratio) {
  const qreal clamped = qBound(0.0, ratio, 1.0);
  return QColor::fromRgbF(a.redF() * (1.0 - clamped) + b.redF() * clamped,
                          a.greenF() * (1.0 - clamped) + b.greenF() * clamped,
                          a.blueF() * (1.0 - clamped) + b.blueF() * clamped,
                          a.alphaF() * (1.0 - clamped) + b.alphaF() * clamped);
}

QColor withAlpha(const QColor &color, int alpha) {
  QColor out = color;
  out.setAlpha(alpha);
  return out;
}

QPointF uv(const QRectF &r, qreal x, qreal y) {
  return QPointF(r.left() + x * r.width(), r.top() + y * r.height());
}

QRectF uvRect(const QRectF &r, qreal x, qreal y, qreal w, qreal h) {
  return QRectF(r.left() + x * r.width(), r.top() + y * r.height(),
                w * r.width(), h * r.height());
}

void drawArrow(QPainter *p, const QPointF &from, const QPointF &to,
               qreal headSize) {
  p->drawLine(from, to);

  const qreal dx = to.x() - from.x();
  const qreal dy = to.y() - from.y();
  const qreal len = std::hypot(dx, dy);
  if (len < 0.01)
    return;

  const qreal ux = dx / len;
  const qreal uy = dy / len;
  const QPointF normal(-uy, ux);

  const QPointF tip = to;
  const QPointF base = QPointF(to.x() - ux * headSize, to.y() - uy * headSize);
  const QPointF left = QPointF(base.x() + normal.x() * headSize * 0.58,
                               base.y() + normal.y() * headSize * 0.58);
  const QPointF right = QPointF(base.x() - normal.x() * headSize * 0.58,
                                base.y() - normal.y() * headSize * 0.58);

  const QBrush oldBrush = p->brush();
  p->setBrush(p->pen().color());
  p->drawPolygon(QPolygonF{tip, left, right});
  p->setBrush(oldBrush);
}

void drawClientIcon(QPainter *p, const QRectF &r, const QColor &accent,
                    qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 38));
  const QRectF screen = uvRect(r, 0.12, 0.13, 0.76, 0.5);
  p->drawRoundedRect(screen, stroke * 1.7, stroke * 1.7);

  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.5, 0.64), uv(r, 0.5, 0.80));
  p->drawLine(uv(r, 0.34, 0.82), uv(r, 0.66, 0.82));
  p->drawLine(uv(r, 0.31, 0.89), uv(r, 0.69, 0.89));
}

void drawLoadBalancerIcon(QPainter *p, const QRectF &r, const QColor &accent,
                          qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  const QPointF core = uv(r, 0.5, 0.5);
  const QPointF lt = uv(r, 0.23, 0.28);
  const QPointF lb = uv(r, 0.23, 0.72);
  const QPointF rt = uv(r, 0.77, 0.28);
  const QPointF rb = uv(r, 0.77, 0.72);

  p->drawLine(core, lt);
  p->drawLine(core, lb);
  p->drawLine(core, rt);
  p->drawLine(core, rb);
  p->drawEllipse(core, stroke * 1.2, stroke * 1.2);

  p->setBrush(withAlpha(accent, 48));
  const qreal nodeR = stroke * 1.05;
  p->drawEllipse(lt, nodeR, nodeR);
  p->drawEllipse(lb, nodeR, nodeR);
  p->drawEllipse(rt, nodeR, nodeR);
  p->drawEllipse(rb, nodeR, nodeR);
}

void drawGatewayIcon(QPainter *p, const QRectF &r, const QColor &accent,
                     qreal stroke) {
  const qreal s = stroke * 0.9;
  p->setPen(QPen(accent, s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

  // Gateway body.
  p->setBrush(withAlpha(accent, 30));
  const QRectF body = uvRect(r, 0.30, 0.26, 0.40, 0.48);
  p->drawRoundedRect(body, s * 1.5, s * 1.5);

  // Gate separator.
  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.50, 0.31), uv(r, 0.50, 0.69));

  // One clean in/out flow path.
  const qreal head = s * 2.1;
  drawArrow(p, uv(r, 0.05, 0.50), uv(r, 0.30, 0.50), head);
  drawArrow(p, uv(r, 0.70, 0.50), uv(r, 0.95, 0.50), head);
}

void drawAppServerIcon(QPainter *p, const QRectF &r, const QColor &accent,
                       qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 32));
  p->drawRoundedRect(uvRect(r, 0.22, 0.18, 0.56, 0.62), stroke * 1.8,
                     stroke * 1.8);

  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.29, 0.34), uv(r, 0.71, 0.34));
  p->drawLine(uv(r, 0.29, 0.49), uv(r, 0.71, 0.49));
  p->drawLine(uv(r, 0.29, 0.64), uv(r, 0.55, 0.64));
  p->setBrush(accent);
  p->drawEllipse(uv(r, 0.67, 0.64), stroke * 0.8, stroke * 0.8);
}

void drawCacheIcon(QPainter *p, const QRectF &r, const QColor &accent,
                   qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 34));
  p->drawRoundedRect(uvRect(r, 0.24, 0.24, 0.52, 0.52), stroke, stroke);

  p->setBrush(Qt::NoBrush);
  for (int i = 0; i < 4; ++i) {
    const qreal t = 0.30 + i * 0.13;
    p->drawLine(uv(r, t, 0.18), uv(r, t, 0.24));
    p->drawLine(uv(r, t, 0.76), uv(r, t, 0.82));
    p->drawLine(uv(r, 0.18, t), uv(r, 0.24, t));
    p->drawLine(uv(r, 0.76, t), uv(r, 0.82, t));
  }

  QPolygonF bolt;
  bolt << uv(r, 0.54, 0.29) << uv(r, 0.43, 0.50) << uv(r, 0.52, 0.50)
       << uv(r, 0.46, 0.71) << uv(r, 0.60, 0.47) << uv(r, 0.51, 0.47);
  p->setBrush(withAlpha(accent.lighter(130), 220));
  p->drawPolygon(bolt);
}

void drawQueueIcon(QPainter *p, const QRectF &r, const QColor &accent,
                   qreal stroke) {
  const qreal s = stroke * 0.74;
  p->setPen(QPen(accent, s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

  // Kafka-like event-log cylinder: straight body + vertical oval ends.
  const QRectF barrel = uvRect(r, 0.18, 0.35, 0.64, 0.30);
  const qreal capW = barrel.height() * 0.56;
  const QRectF leftCap(barrel.left() - capW * 0.5, barrel.top(), capW,
                       barrel.height());
  const QRectF rightCap(barrel.right() - capW * 0.5, barrel.top(), capW,
                        barrel.height());

  QLinearGradient bodyGrad(barrel.topLeft(), barrel.bottomLeft());
  bodyGrad.setColorAt(0.0, withAlpha(accent.lighter(136), 40));
  bodyGrad.setColorAt(1.0, withAlpha(accent.darker(122), 20));
  p->setBrush(bodyGrad);
  p->drawRect(barrel);
  p->drawEllipse(leftCap);
  p->drawEllipse(rightCap);

  // Segment slots inside the event log.
  p->setBrush(withAlpha(accent, 28));
  const qreal slotH = barrel.height() * 0.50;
  const qreal slotY = barrel.center().y() - slotH * 0.5;
  const qreal startX = barrel.left() + barrel.width() * 0.08;
  const qreal usableW = barrel.width() * 0.84;
  const int slotCount = 5;
  const qreal gap = barrel.width() * 0.04;
  const qreal slotW = (usableW - gap * (slotCount - 1)) / slotCount;
  for (int i = 0; i < slotCount; ++i) {
    const qreal x = startX + i * (slotW + gap);
    p->drawRect(QRectF(x, slotY, slotW, slotH));
  }
}

void drawDatabaseIcon(QPainter *p, const QRectF &r, const QColor &accent,
                      qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 34));

  const QRectF body = uvRect(r, 0.21, 0.24, 0.58, 0.52);
  const qreal ellH = body.height() * 0.34;

  p->drawRect(QRectF(body.left(), body.top() + ellH * 0.5, body.width(),
                     body.height() - ellH));
  p->drawEllipse(QRectF(body.left(), body.top(), body.width(), ellH));
  p->drawEllipse(QRectF(body.left(), body.bottom() - ellH, body.width(), ellH));

  p->setBrush(Qt::NoBrush);
  QPainterPath mid1;
  QRectF m1(body.left(), body.top() + body.height() * 0.44 - ellH * 0.5,
            body.width(), ellH);
  mid1.arcMoveTo(m1, 0);
  mid1.arcTo(m1, 0, -180);
  p->drawPath(mid1);

  QPainterPath mid2;
  QRectF m2(body.left(), body.top() + body.height() * 0.62 - ellH * 0.5,
            body.width(), ellH);
  mid2.arcMoveTo(m2, 0);
  mid2.arcTo(m2, 0, -180);
  p->drawPath(mid2);
}

void drawStorageIcon(QPainter *p, const QRectF &r, const QColor &accent,
                     qreal stroke) {
  const qreal s = stroke * 0.9;
  p->setPen(QPen(accent, s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

  // Storage container body.
  p->setBrush(withAlpha(accent, 34));
  const QRectF body = uvRect(r, 0.18, 0.34, 0.64, 0.42);
  p->drawRoundedRect(body, s * 1.4, s * 1.4);

  // Slot (where objects are written/read).
  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.28, 0.46), uv(r, 0.72, 0.46));

  // Down arrow into storage.
  const qreal head = s * 2.2;
  drawArrow(p, uv(r, 0.50, 0.16), uv(r, 0.50, 0.42), head);

  // Stored objects.
  p->setBrush(withAlpha(accent, 92));
  p->drawRoundedRect(uvRect(r, 0.27, 0.55, 0.10, 0.12), s * 0.75, s * 0.75);
  p->drawRoundedRect(uvRect(r, 0.45, 0.55, 0.10, 0.12), s * 0.75, s * 0.75);
  p->drawRoundedRect(uvRect(r, 0.63, 0.55, 0.10, 0.12), s * 0.75, s * 0.75);
}

void drawAuthIcon(QPainter *p, const QRectF &r, const QColor &accent,
                  qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 34));

  QPainterPath shield;
  shield.moveTo(uv(r, 0.50, 0.16));
  shield.lineTo(uv(r, 0.75, 0.27));
  shield.quadTo(uv(r, 0.75, 0.65), uv(r, 0.50, 0.83));
  shield.quadTo(uv(r, 0.25, 0.65), uv(r, 0.25, 0.27));
  shield.closeSubpath();
  p->drawPath(shield);

  p->setBrush(Qt::NoBrush);
  p->drawRoundedRect(uvRect(r, 0.39, 0.43, 0.22, 0.18), stroke * 1.0,
                     stroke * 1.0);
  p->drawArc(uvRect(r, 0.42, 0.33, 0.16, 0.14), 0, 180 * 16);
}

void drawMonitoringIcon(QPainter *p, const QRectF &r, const QColor &accent,
                        qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 28));
  p->drawRoundedRect(uvRect(r, 0.15, 0.18, 0.70, 0.64), stroke * 1.3,
                     stroke * 1.3);

  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.23, 0.72), uv(r, 0.23, 0.30));
  p->drawLine(uv(r, 0.23, 0.72), uv(r, 0.76, 0.72));

  p->drawLine(uv(r, 0.28, 0.66), uv(r, 0.36, 0.54));
  p->drawLine(uv(r, 0.36, 0.54), uv(r, 0.47, 0.62));
  p->drawLine(uv(r, 0.47, 0.62), uv(r, 0.57, 0.43));
  p->drawLine(uv(r, 0.57, 0.43), uv(r, 0.70, 0.50));

  p->setBrush(accent);
  p->drawEllipse(uv(r, 0.36, 0.54), stroke * 0.7, stroke * 0.7);
  p->drawEllipse(uv(r, 0.57, 0.43), stroke * 0.7, stroke * 0.7);
}

} // namespace

// ===========================================================================
// ArchitectureElementItem  (base)
// ===========================================================================

ArchitectureElementItem::ArchitectureElementItem(const QString &label,
                                                 IconKind iconKind,
                                                 const QColor &accentColor,
                                                 QGraphicsItem *parent)
    : QGraphicsItem(parent), label_(label), iconKind_(iconKind),
      accentColor_(accentColor) {
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
  painter->setRenderHint(QPainter::TextAntialiasing, true);

  const QColor accent =
      accentColor_.isValid() ? accentColor_ : QColor("#3b82f6");
  const QRectF cardRect = boundingRect().adjusted(1.0, 1.0, -1.0, -1.0);

  // Soft shadow for depth.
  painter->setPen(Qt::NoPen);
  painter->setBrush(QColor(0, 0, 0, 55));
  painter->drawRoundedRect(cardRect.translated(0.0, 2.0), CORNER, CORNER);

  // Main card fill with a slight tint from the element accent color.
  const QColor topBase("#242a35");
  const QColor bottomBase("#171c24");
  QLinearGradient bg(cardRect.topLeft(), cardRect.bottomLeft());
  bg.setColorAt(0.0, mixColor(topBase, accent, 0.18));
  bg.setColorAt(1.0, mixColor(bottomBase, accent, 0.08));

  QPainterPath cardPath;
  cardPath.addRoundedRect(cardRect, CORNER, CORNER);
  painter->setBrush(bg);
  painter->setPen(QPen(withAlpha(accent, 150), 1.35));
  painter->drawPath(cardPath);

  // Thin top accent band.
  painter->save();
  painter->setClipPath(cardPath);
  const QRectF bandRect(cardRect.x() + 1.0, cardRect.y() + 1.0,
                        cardRect.width() - 2.0, 11.0);
  QLinearGradient band(bandRect.topLeft(), bandRect.bottomLeft());
  band.setColorAt(0.0, withAlpha(accent.lighter(135), 110));
  band.setColorAt(1.0, QColor(0, 0, 0, 0));
  painter->setPen(Qt::NoPen);
  painter->setBrush(band);
  painter->drawRect(bandRect);
  painter->restore();

  // Icon badge.
  const QRectF badgeRect((ELEM_W - ICON_SIZE) / 2.0, 12.0, ICON_SIZE,
                         ICON_SIZE);
  QRadialGradient badgeGrad(badgeRect.center(), ICON_SIZE / 2.0);
  badgeGrad.setColorAt(0.0, withAlpha(accent.lighter(145), 90));
  badgeGrad.setColorAt(0.9, withAlpha(accent.darker(140), 120));
  badgeGrad.setColorAt(1.0, withAlpha(accent.darker(180), 140));
  painter->setBrush(badgeGrad);
  painter->setPen(QPen(withAlpha(accent.lighter(150), 180), 1.15));
  painter->drawEllipse(badgeRect);

  const QRectF iconRect = badgeRect.adjusted(10.0, 10.0, -10.0, -10.0);
  paintIcon(painter, iconRect);

  // Label.
  painter->setPen(QColor("#ebeff7"));
  QFont font("Segoe UI", 9, QFont::DemiBold);
  font.setLetterSpacing(QFont::PercentageSpacing, 102);
  painter->setFont(font);
  painter->drawText(QRectF(8.0, ELEM_H - 30.0, ELEM_W - 16.0, 20.0),
                    Qt::AlignCenter, label_);

  if (isSelected()) {
    QPen selectPen(withAlpha(accent.lighter(150), 235), 1.8, Qt::DashLine);
    selectPen.setDashPattern({3.0, 2.0});
    painter->setPen(selectPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(cardRect.adjusted(1.5, 1.5, -1.5, -1.5),
                             CORNER - 1.5, CORNER - 1.5);
  }
}

void ArchitectureElementItem::paintIcon(QPainter *painter,
                                        const QRectF &rect) const {
  const QColor stroke = accentColor_.lighter(225);
  const qreal width = qMax(1.25, rect.width() * 0.09);

  switch (iconKind_) {
  case IconKind::Client:
    drawClientIcon(painter, rect, stroke, width);
    break;
  case IconKind::LoadBalancer:
    drawLoadBalancerIcon(painter, rect, stroke, width);
    break;
  case IconKind::ApiGateway:
    drawGatewayIcon(painter, rect, stroke, width);
    break;
  case IconKind::AppServer:
    drawAppServerIcon(painter, rect, stroke, width);
    break;
  case IconKind::Cache:
    drawCacheIcon(painter, rect, stroke, width);
    break;
  case IconKind::MessageQueue:
    drawQueueIcon(painter, rect, stroke, width);
    break;
  case IconKind::Database:
    drawDatabaseIcon(painter, rect, stroke, width);
    break;
  case IconKind::ObjectStorage:
    drawStorageIcon(painter, rect, stroke, width);
    break;
  case IconKind::Auth:
    drawAuthIcon(painter, rect, stroke, width);
    break;
  case IconKind::Monitoring:
    drawMonitoringIcon(painter, rect, stroke, width);
    break;
  }
}

// ===========================================================================
// Concrete element constructors
// ===========================================================================

ClientElement::ClientElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Client", IconKind::Client, QColor("#3b82f6"),
                              parent) {}

LoadBalancerElement::LoadBalancerElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Load Balancer", IconKind::LoadBalancer,
                              QColor("#f59e0b"), parent) {}

ApiGatewayElement::ApiGatewayElement(QGraphicsItem *parent)
    : ArchitectureElementItem("API Gateway", IconKind::ApiGateway,
                              QColor("#8b5cf6"), parent) {}

AppServerElement::AppServerElement(QGraphicsItem *parent)
    : ArchitectureElementItem("App Server", IconKind::AppServer,
                              QColor("#10b981"), parent) {}

CacheElement::CacheElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Cache", IconKind::Cache, QColor("#eab308"),
                              parent) {}

MessageQueueElement::MessageQueueElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Queue", IconKind::MessageQueue,
                              QColor("#ec4899"), parent) {}

DatabaseElement::DatabaseElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Database", IconKind::Database, QColor("#06b6d4"),
                              parent) {}

ObjectStorageElement::ObjectStorageElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Storage", IconKind::ObjectStorage,
                              QColor("#f97316"), parent) {}

AuthElement::AuthElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Auth", IconKind::Auth, QColor("#ef4444"),
                              parent) {}

MonitoringElement::MonitoringElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Monitor", IconKind::Monitoring,
                              QColor("#14b8a6"), parent) {}
