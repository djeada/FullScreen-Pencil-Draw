/**
 * @file architecture_elements.cpp
 * @brief Implementation of custom vector-drawn architecture diagram elements.
 */
#include "architecture_elements.h"
#include <QFont>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>
#include <cmath>
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
  const qreal s = stroke * 0.82;
  p->setPen(QPen(accent, s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

  // Horizontal rounded pipe / channel body.
  const QRectF pipe = uvRect(r, 0.12, 0.28, 0.76, 0.44);
  const qreal pipeCorner = pipe.height() * 0.38;
  QLinearGradient pipeGrad(pipe.topLeft(), pipe.bottomLeft());
  pipeGrad.setColorAt(0.0, withAlpha(accent.lighter(130), 38));
  pipeGrad.setColorAt(1.0, withAlpha(accent.darker(115), 18));
  p->setBrush(pipeGrad);
  p->drawRoundedRect(pipe, pipeCorner, pipeCorner);

  // Three message packets queued inside.
  p->setBrush(withAlpha(accent.lighter(120), 90));
  const qreal itemH = pipe.height() * 0.52;
  const qreal itemY = pipe.center().y() - itemH * 0.5;
  const qreal itemW = pipe.width() * 0.19;
  const qreal gap = pipe.width() * 0.06;
  const qreal totalW = 3.0 * itemW + 2.0 * gap;
  const qreal startX = pipe.center().x() - totalW * 0.5;
  const qreal itemR = s * 0.8;
  for (int i = 0; i < 3; ++i) {
    const qreal x = startX + i * (itemW + gap);
    p->drawRoundedRect(QRectF(x, itemY, itemW, itemH), itemR, itemR);
  }

  // Enqueue arrow entering from the left.
  const qreal head = s * 2.0;
  drawArrow(p, uv(r, 0.0, 0.50), uv(r, 0.14, 0.50), head);

  // Dequeue arrow exiting to the right.
  drawArrow(p, uv(r, 0.86, 0.50), uv(r, 1.0, 0.50), head);
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

void drawUserIcon(QPainter *p, const QRectF &r, const QColor &accent,
                  qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 38));

  // Head circle
  p->drawEllipse(uv(r, 0.50, 0.28), r.width() * 0.17, r.height() * 0.17);

  // Body / torso arc
  QPainterPath body;
  body.moveTo(uv(r, 0.22, 0.85));
  body.quadTo(uv(r, 0.22, 0.55), uv(r, 0.50, 0.52));
  body.quadTo(uv(r, 0.78, 0.55), uv(r, 0.78, 0.85));
  body.closeSubpath();
  p->drawPath(body);
}

void drawUserGroupIcon(QPainter *p, const QRectF &r, const QColor &accent,
                       qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 32));

  // Back person (slightly offset right)
  p->drawEllipse(uv(r, 0.62, 0.24), r.width() * 0.12, r.height() * 0.12);
  QPainterPath backBody;
  backBody.moveTo(uv(r, 0.44, 0.78));
  backBody.quadTo(uv(r, 0.44, 0.52), uv(r, 0.62, 0.44));
  backBody.quadTo(uv(r, 0.80, 0.52), uv(r, 0.80, 0.78));
  backBody.closeSubpath();
  p->drawPath(backBody);

  // Front person (slightly offset left)
  p->setBrush(withAlpha(accent, 48));
  p->drawEllipse(uv(r, 0.38, 0.28), r.width() * 0.13, r.height() * 0.13);
  QPainterPath frontBody;
  frontBody.moveTo(uv(r, 0.18, 0.84));
  frontBody.quadTo(uv(r, 0.18, 0.56), uv(r, 0.38, 0.48));
  frontBody.quadTo(uv(r, 0.58, 0.56), uv(r, 0.58, 0.84));
  frontBody.closeSubpath();
  p->drawPath(frontBody);
}

void drawCloudIcon(QPainter *p, const QRectF &r, const QColor &accent,
                   qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 32));

  QPainterPath cloud;
  cloud.moveTo(uv(r, 0.25, 0.68));
  cloud.quadTo(uv(r, 0.10, 0.68), uv(r, 0.12, 0.52));
  cloud.quadTo(uv(r, 0.10, 0.38), uv(r, 0.28, 0.36));
  cloud.quadTo(uv(r, 0.30, 0.18), uv(r, 0.50, 0.20));
  cloud.quadTo(uv(r, 0.70, 0.14), uv(r, 0.74, 0.34));
  cloud.quadTo(uv(r, 0.92, 0.34), uv(r, 0.88, 0.52));
  cloud.quadTo(uv(r, 0.92, 0.68), uv(r, 0.75, 0.68));
  cloud.closeSubpath();
  p->drawPath(cloud);
}

void drawCDNIcon(QPainter *p, const QRectF &r, const QColor &accent,
                 qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 30));

  // Globe circle
  p->drawEllipse(uvRect(r, 0.22, 0.18, 0.56, 0.56));

  // Horizontal latitude lines
  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.24, 0.46), uv(r, 0.76, 0.46));
  p->drawLine(uv(r, 0.30, 0.33), uv(r, 0.70, 0.33));
  p->drawLine(uv(r, 0.30, 0.59), uv(r, 0.70, 0.59));

  // Vertical meridian
  QPainterPath meridian;
  meridian.moveTo(uv(r, 0.50, 0.18));
  meridian.quadTo(uv(r, 0.36, 0.46), uv(r, 0.50, 0.74));
  p->drawPath(meridian);
  QPainterPath meridian2;
  meridian2.moveTo(uv(r, 0.50, 0.18));
  meridian2.quadTo(uv(r, 0.64, 0.46), uv(r, 0.50, 0.74));
  p->drawPath(meridian2);

  // Small scatter nodes around globe
  p->setBrush(withAlpha(accent, 90));
  p->drawEllipse(uv(r, 0.20, 0.80), stroke * 0.7, stroke * 0.7);
  p->drawEllipse(uv(r, 0.50, 0.82), stroke * 0.7, stroke * 0.7);
  p->drawEllipse(uv(r, 0.80, 0.80), stroke * 0.7, stroke * 0.7);
}

void drawDNSIcon(QPainter *p, const QRectF &r, const QColor &accent,
                 qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 30));

  // Address book / card shape
  p->drawRoundedRect(uvRect(r, 0.18, 0.20, 0.64, 0.60), stroke * 1.5,
                     stroke * 1.5);

  // Text lines representing domain names
  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.26, 0.36), uv(r, 0.54, 0.36));
  p->drawLine(uv(r, 0.58, 0.36), uv(r, 0.74, 0.36));
  p->drawLine(uv(r, 0.26, 0.50), uv(r, 0.48, 0.50));
  p->drawLine(uv(r, 0.52, 0.50), uv(r, 0.74, 0.50));
  p->drawLine(uv(r, 0.26, 0.64), uv(r, 0.50, 0.64));
  p->drawLine(uv(r, 0.54, 0.64), uv(r, 0.74, 0.64));

  // Arrow mapping symbol
  const qreal head = stroke * 1.8;
  drawArrow(p, uv(r, 0.36, 0.84), uv(r, 0.64, 0.84), head);
}

void drawFirewallIcon(QPainter *p, const QRectF &r, const QColor &accent,
                      qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 34));

  // Brick wall pattern
  const QRectF wall = uvRect(r, 0.16, 0.20, 0.68, 0.60);
  p->drawRoundedRect(wall, stroke, stroke);

  p->setBrush(Qt::NoBrush);
  // Horizontal lines (rows of bricks)
  p->drawLine(uv(r, 0.16, 0.40), uv(r, 0.84, 0.40));
  p->drawLine(uv(r, 0.16, 0.60), uv(r, 0.84, 0.60));

  // Vertical brick offsets - row 1
  p->drawLine(uv(r, 0.38, 0.20), uv(r, 0.38, 0.40));
  p->drawLine(uv(r, 0.62, 0.20), uv(r, 0.62, 0.40));

  // Vertical brick offsets - row 2 (offset)
  p->drawLine(uv(r, 0.27, 0.40), uv(r, 0.27, 0.60));
  p->drawLine(uv(r, 0.50, 0.40), uv(r, 0.50, 0.60));
  p->drawLine(uv(r, 0.73, 0.40), uv(r, 0.73, 0.60));

  // Vertical brick offsets - row 3
  p->drawLine(uv(r, 0.38, 0.60), uv(r, 0.38, 0.80));
  p->drawLine(uv(r, 0.62, 0.60), uv(r, 0.62, 0.80));
}

void drawContainerIcon(QPainter *p, const QRectF &r, const QColor &accent,
                       qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 30));

  // Container box (like a shipping container / Docker)
  const QRectF box = uvRect(r, 0.20, 0.24, 0.60, 0.52);
  p->drawRoundedRect(box, stroke * 1.2, stroke * 1.2);

  // Vertical container segments
  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.35, 0.24), uv(r, 0.35, 0.76));
  p->drawLine(uv(r, 0.50, 0.24), uv(r, 0.50, 0.76));
  p->drawLine(uv(r, 0.65, 0.24), uv(r, 0.65, 0.76));

  // Handle/lid at top
  p->drawLine(uv(r, 0.28, 0.18), uv(r, 0.72, 0.18));
  p->drawLine(uv(r, 0.28, 0.18), uv(r, 0.28, 0.24));
  p->drawLine(uv(r, 0.72, 0.18), uv(r, 0.72, 0.24));
}

void drawServerlessIcon(QPainter *p, const QRectF &r, const QColor &accent,
                        qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 34));

  // Lambda-like symbol inside a rounded frame
  p->drawRoundedRect(uvRect(r, 0.22, 0.18, 0.56, 0.64), stroke * 1.6,
                     stroke * 1.6);

  // Lambda (λ) drawn as lines
  p->setBrush(Qt::NoBrush);
  QPen thickPen(accent, stroke * 1.3, Qt::SolidLine, Qt::RoundCap,
                Qt::RoundJoin);
  p->setPen(thickPen);
  p->drawLine(uv(r, 0.34, 0.30), uv(r, 0.50, 0.62));
  p->drawLine(uv(r, 0.50, 0.62), uv(r, 0.66, 0.30));
  p->drawLine(uv(r, 0.42, 0.46), uv(r, 0.34, 0.72));
}

void drawVirtualMachineIcon(QPainter *p, const QRectF &r,
                            const QColor &accent, qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 28));

  // Outer box (physical host)
  p->drawRoundedRect(uvRect(r, 0.14, 0.16, 0.72, 0.68), stroke * 1.4,
                     stroke * 1.4);

  // Inner boxes (virtual machines)
  p->setBrush(withAlpha(accent, 50));
  p->drawRoundedRect(uvRect(r, 0.22, 0.26, 0.24, 0.22), stroke * 0.8,
                     stroke * 0.8);
  p->drawRoundedRect(uvRect(r, 0.54, 0.26, 0.24, 0.22), stroke * 0.8,
                     stroke * 0.8);
  p->drawRoundedRect(uvRect(r, 0.22, 0.54, 0.24, 0.22), stroke * 0.8,
                     stroke * 0.8);
  p->drawRoundedRect(uvRect(r, 0.54, 0.54, 0.24, 0.22), stroke * 0.8,
                     stroke * 0.8);
}

void drawMicroserviceIcon(QPainter *p, const QRectF &r, const QColor &accent,
                          qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 36));

  // Hexagon (microservice shape)
  const QPointF center = uv(r, 0.50, 0.50);
  const qreal rx = r.width() * 0.34;
  const qreal ry = r.height() * 0.34;
  QPolygonF hex;
  for (int i = 0; i < 6; ++i) {
    const qreal angle = M_PI / 6.0 + i * M_PI / 3.0;
    hex << QPointF(center.x() + rx * std::cos(angle),
                   center.y() + ry * std::sin(angle));
  }
  p->drawPolygon(hex);

  // Gear-like inner detail
  p->setBrush(Qt::NoBrush);
  p->drawEllipse(center, rx * 0.32, ry * 0.32);
  p->setBrush(accent);
  p->drawEllipse(center, stroke * 0.7, stroke * 0.7);
}

void drawAPIIcon(QPainter *p, const QRectF &r, const QColor &accent,
                 qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 30));

  // Bracket-like endpoints: { }
  QPainterPath leftBracket;
  leftBracket.moveTo(uv(r, 0.34, 0.18));
  leftBracket.quadTo(uv(r, 0.20, 0.18), uv(r, 0.20, 0.35));
  leftBracket.lineTo(uv(r, 0.20, 0.42));
  leftBracket.quadTo(uv(r, 0.20, 0.50), uv(r, 0.12, 0.50));
  leftBracket.quadTo(uv(r, 0.20, 0.50), uv(r, 0.20, 0.58));
  leftBracket.lineTo(uv(r, 0.20, 0.65));
  leftBracket.quadTo(uv(r, 0.20, 0.82), uv(r, 0.34, 0.82));
  p->setBrush(Qt::NoBrush);
  p->drawPath(leftBracket);

  QPainterPath rightBracket;
  rightBracket.moveTo(uv(r, 0.66, 0.18));
  rightBracket.quadTo(uv(r, 0.80, 0.18), uv(r, 0.80, 0.35));
  rightBracket.lineTo(uv(r, 0.80, 0.42));
  rightBracket.quadTo(uv(r, 0.80, 0.50), uv(r, 0.88, 0.50));
  rightBracket.quadTo(uv(r, 0.80, 0.50), uv(r, 0.80, 0.58));
  rightBracket.lineTo(uv(r, 0.80, 0.65));
  rightBracket.quadTo(uv(r, 0.80, 0.82), uv(r, 0.66, 0.82));
  p->drawPath(rightBracket);

  // Slash in the middle (representing /api)
  QPen slashPen(accent, stroke * 1.2, Qt::SolidLine, Qt::RoundCap);
  p->setPen(slashPen);
  p->drawLine(uv(r, 0.55, 0.32), uv(r, 0.45, 0.68));
}

void drawNotificationIcon(QPainter *p, const QRectF &r, const QColor &accent,
                          qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 34));

  // Bell shape
  QPainterPath bell;
  bell.moveTo(uv(r, 0.28, 0.62));
  bell.quadTo(uv(r, 0.28, 0.30), uv(r, 0.50, 0.22));
  bell.quadTo(uv(r, 0.72, 0.30), uv(r, 0.72, 0.62));
  bell.lineTo(uv(r, 0.78, 0.68));
  bell.lineTo(uv(r, 0.22, 0.68));
  bell.closeSubpath();
  p->drawPath(bell);

  // Clapper
  p->setBrush(accent);
  p->drawEllipse(uv(r, 0.50, 0.76), stroke * 1.0, stroke * 0.8);

  // Small ring at top
  p->setBrush(Qt::NoBrush);
  p->drawEllipse(uv(r, 0.50, 0.17), stroke * 0.7, stroke * 0.7);
}

void drawSearchIcon(QPainter *p, const QRectF &r, const QColor &accent,
                    qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 28));

  // Magnifying glass circle
  const QPointF glassCenter = uv(r, 0.44, 0.42);
  const qreal glassR = r.width() * 0.24;
  p->drawEllipse(glassCenter, glassR, glassR);

  // Handle
  QPen handlePen(accent, stroke * 1.5, Qt::SolidLine, Qt::RoundCap);
  p->setPen(handlePen);
  p->drawLine(uv(r, 0.60, 0.58), uv(r, 0.78, 0.78));

  // Search lines inside lens
  p->setPen(QPen(accent, stroke * 0.7, Qt::SolidLine, Qt::RoundCap));
  p->drawLine(uv(r, 0.32, 0.38), uv(r, 0.52, 0.38));
  p->drawLine(uv(r, 0.32, 0.46), uv(r, 0.48, 0.46));
}

void drawLoggingIcon(QPainter *p, const QRectF &r, const QColor &accent,
                     qreal stroke) {
  p->setPen(QPen(accent, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(accent, 28));

  // Document/log file
  QPainterPath doc;
  doc.moveTo(uv(r, 0.24, 0.14));
  doc.lineTo(uv(r, 0.62, 0.14));
  doc.lineTo(uv(r, 0.76, 0.28));
  doc.lineTo(uv(r, 0.76, 0.86));
  doc.lineTo(uv(r, 0.24, 0.86));
  doc.closeSubpath();
  p->drawPath(doc);

  // Folded corner
  p->setBrush(withAlpha(accent, 60));
  QPainterPath fold;
  fold.moveTo(uv(r, 0.62, 0.14));
  fold.lineTo(uv(r, 0.62, 0.28));
  fold.lineTo(uv(r, 0.76, 0.28));
  fold.closeSubpath();
  p->drawPath(fold);

  // Log text lines
  p->setBrush(Qt::NoBrush);
  p->drawLine(uv(r, 0.32, 0.40), uv(r, 0.68, 0.40));
  p->drawLine(uv(r, 0.32, 0.52), uv(r, 0.68, 0.52));
  p->drawLine(uv(r, 0.32, 0.64), uv(r, 0.56, 0.64));
  p->drawLine(uv(r, 0.32, 0.76), uv(r, 0.62, 0.76));
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
  initPaintCache();
}

QRectF ArchitectureElementItem::boundingRect() const {
  return QRectF(0, 0, ELEM_W, ELEM_H);
}

void ArchitectureElementItem::initPaintCache() {
  const QColor accent =
      accentColor_.isValid() ? accentColor_ : QColor("#3b82f6");

  const QColor topBase("#242a35");
  const QColor bottomBase("#171c24");

  colors_.bgTop = mixColor(topBase, accent, 0.18);
  colors_.bgBottom = mixColor(bottomBase, accent, 0.08);
  colors_.cardBorder = withAlpha(accent, 150);
  colors_.bandTop = withAlpha(accent.lighter(135), 110);
  colors_.badgeCenter = withAlpha(accent.lighter(145), 90);
  colors_.badgeMid = withAlpha(accent.darker(140), 120);
  colors_.badgeEdge = withAlpha(accent.darker(180), 140);
  colors_.badgeBorder = withAlpha(accent.lighter(150), 180);
  colors_.iconStroke = accent.lighter(225);
  colors_.selectBorder = withAlpha(accent.lighter(150), 235);

  cardRect_ = boundingRect().adjusted(1.0, 1.0, -1.0, -1.0);
  bandRect_ = QRectF(cardRect_.x() + 1.0, cardRect_.y() + 1.0,
                     cardRect_.width() - 2.0, 11.0);
  badgeRect_ = QRectF((ELEM_W - ICON_SIZE) / 2.0, 12.0, ICON_SIZE, ICON_SIZE);
  iconRect_ = badgeRect_.adjusted(10.0, 10.0, -10.0, -10.0);
  labelRect_ = QRectF(8.0, ELEM_H - 30.0, ELEM_W - 16.0, 20.0);

  cardPath_ = QPainterPath();
  cardPath_.addRoundedRect(cardRect_, CORNER, CORNER);

  iconStrokeWidth_ = qMax(1.25, iconRect_.width() * 0.09);

  renderToPixmap();
}

void ArchitectureElementItem::renderToPixmap(qreal scale) {
  cachedPixmapScale_ = scale;
  const int w = qMax(1, static_cast<int>(std::ceil(ELEM_W * scale)));
  const int h = qMax(1, static_cast<int>(std::ceil(ELEM_H * scale)));
  QPixmap pm(w, h);
  pm.setDevicePixelRatio(scale);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setRenderHint(QPainter::TextAntialiasing, true);

  // Shadow
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0, 0, 0, 55));
  p.drawRoundedRect(cardRect_.translated(0.0, 2.0), CORNER, CORNER);

  // Card fill
  QLinearGradient bg(cardRect_.topLeft(), cardRect_.bottomLeft());
  bg.setColorAt(0.0, colors_.bgTop);
  bg.setColorAt(1.0, colors_.bgBottom);
  p.setBrush(bg);
  p.setPen(QPen(colors_.cardBorder, 1.35));
  p.drawPath(cardPath_);

  // Accent band
  p.save();
  p.setClipPath(cardPath_);
  QLinearGradient band(bandRect_.topLeft(), bandRect_.bottomLeft());
  band.setColorAt(0.0, colors_.bandTop);
  band.setColorAt(1.0, QColor(0, 0, 0, 0));
  p.setPen(Qt::NoPen);
  p.setBrush(band);
  p.drawRect(bandRect_);
  p.restore();

  // Badge
  QRadialGradient badgeGrad(badgeRect_.center(), ICON_SIZE / 2.0);
  badgeGrad.setColorAt(0.0, colors_.badgeCenter);
  badgeGrad.setColorAt(0.9, colors_.badgeMid);
  badgeGrad.setColorAt(1.0, colors_.badgeEdge);
  p.setBrush(badgeGrad);
  p.setPen(QPen(colors_.badgeBorder, 1.15));
  p.drawEllipse(badgeRect_);

  // Icon
  paintIcon(&p, iconRect_);

  // Label
  static const QFont labelFont = []() {
    QFont f("Segoe UI", 9, QFont::DemiBold);
    f.setLetterSpacing(QFont::PercentageSpacing, 102);
    return f;
  }();
  p.setPen(QColor("#ebeff7"));
  p.setFont(labelFont);
  p.drawText(labelRect_, Qt::AlignCenter, label_);

  p.end();
  cachedPixmap_ = pm;
}

void ArchitectureElementItem::paint(QPainter *painter,
                                    const QStyleOptionGraphicsItem * /*option*/,
                                    QWidget * /*widget*/) {
  // Re-render pixmap when the effective scale changes (e.g. after resize)
  // so vector content stays crisp at any size.
  const QTransform &wt = painter->worldTransform();
  const qreal sx = std::hypot(wt.m11(), wt.m21());
  const qreal sy = std::hypot(wt.m12(), wt.m22());
  const qreal effectiveScale = qBound(0.25, qMax(sx, sy), 8.0);
  if (std::abs(effectiveScale - cachedPixmapScale_) >
      cachedPixmapScale_ * 0.15) {
    const_cast<ArchitectureElementItem *>(this)->renderToPixmap(effectiveScale);
  }

  painter->drawPixmap(0, 0, cachedPixmap_);

  if (isSelected()) {
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen selectPen(colors_.selectBorder, 1.8, Qt::DashLine);
    selectPen.setDashPattern({3.0, 2.0});
    painter->setPen(selectPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(cardRect_.adjusted(1.5, 1.5, -1.5, -1.5),
                             CORNER - 1.5, CORNER - 1.5);
  }
}

void ArchitectureElementItem::paintIcon(QPainter *painter,
                                        const QRectF &rect) const {
  const QColor &stroke = colors_.iconStroke;
  const qreal width = iconStrokeWidth_;

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
  case IconKind::User:
    drawUserIcon(painter, rect, stroke, width);
    break;
  case IconKind::UserGroup:
    drawUserGroupIcon(painter, rect, stroke, width);
    break;
  case IconKind::Cloud:
    drawCloudIcon(painter, rect, stroke, width);
    break;
  case IconKind::CDN:
    drawCDNIcon(painter, rect, stroke, width);
    break;
  case IconKind::DNS:
    drawDNSIcon(painter, rect, stroke, width);
    break;
  case IconKind::Firewall:
    drawFirewallIcon(painter, rect, stroke, width);
    break;
  case IconKind::Container:
    drawContainerIcon(painter, rect, stroke, width);
    break;
  case IconKind::Serverless:
    drawServerlessIcon(painter, rect, stroke, width);
    break;
  case IconKind::VirtualMachine:
    drawVirtualMachineIcon(painter, rect, stroke, width);
    break;
  case IconKind::Microservice:
    drawMicroserviceIcon(painter, rect, stroke, width);
    break;
  case IconKind::API:
    drawAPIIcon(painter, rect, stroke, width);
    break;
  case IconKind::Notification:
    drawNotificationIcon(painter, rect, stroke, width);
    break;
  case IconKind::Search:
    drawSearchIcon(painter, rect, stroke, width);
    break;
  case IconKind::Logging:
    drawLoggingIcon(painter, rect, stroke, width);
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

UserElement::UserElement(QGraphicsItem *parent)
    : ArchitectureElementItem("User", IconKind::User, QColor("#6366f1"),
                              parent) {}

UserGroupElement::UserGroupElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Users", IconKind::UserGroup, QColor("#818cf8"),
                              parent) {}

CloudElement::CloudElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Cloud", IconKind::Cloud, QColor("#0ea5e9"),
                              parent) {}

CDNElement::CDNElement(QGraphicsItem *parent)
    : ArchitectureElementItem("CDN", IconKind::CDN, QColor("#38bdf8"),
                              parent) {}

DNSElement::DNSElement(QGraphicsItem *parent)
    : ArchitectureElementItem("DNS", IconKind::DNS, QColor("#a78bfa"),
                              parent) {}

FirewallElement::FirewallElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Firewall", IconKind::Firewall,
                              QColor("#f43f5e"), parent) {}

ContainerElement::ContainerElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Container", IconKind::Container,
                              QColor("#2dd4bf"), parent) {}

ServerlessElement::ServerlessElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Serverless", IconKind::Serverless,
                              QColor("#fb923c"), parent) {}

VirtualMachineElement::VirtualMachineElement(QGraphicsItem *parent)
    : ArchitectureElementItem("VM", IconKind::VirtualMachine,
                              QColor("#a3e635"), parent) {}

MicroserviceElement::MicroserviceElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Microservice", IconKind::Microservice,
                              QColor("#34d399"), parent) {}

APIElement::APIElement(QGraphicsItem *parent)
    : ArchitectureElementItem("API", IconKind::API, QColor("#c084fc"),
                              parent) {}

NotificationElement::NotificationElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Notification", IconKind::Notification,
                              QColor("#fb7185"), parent) {}

SearchElement::SearchElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Search", IconKind::Search, QColor("#fbbf24"),
                              parent) {}

LoggingElement::LoggingElement(QGraphicsItem *parent)
    : ArchitectureElementItem("Logging", IconKind::Logging, QColor("#94a3b8"),
                              parent) {}
