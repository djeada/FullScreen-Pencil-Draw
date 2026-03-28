/**
 * @file electronics_elements.cpp
 * @brief Implementation of custom vector-drawn electronics schematic elements.
 */
#include "electronics_elements.h"
#include "wire_item.h"
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

// -----------------------------------------------------------------------
// Icon drawing helpers – each renders a recognizable electronics symbol
// inside the normalized 26×26 icon rect.
// -----------------------------------------------------------------------

void drawResistorIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                      qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  // Horizontal zigzag resistor symbol between two terminal stubs.
  const qreal y0 = r.top() + r.height() * 0.5;
  const qreal xL = r.left();
  const qreal xR = r.right();
  const qreal stubLen = r.width() * 0.15;
  const qreal zigStart = xL + stubLen;
  const qreal zigEnd = xR - stubLen;
  const qreal amp = r.height() * 0.30;
  const int teeth = 5;
  const qreal segW = (zigEnd - zigStart) / teeth;

  QPainterPath path;
  path.moveTo(xL, y0);
  path.lineTo(zigStart, y0);
  for (int i = 0; i < teeth; ++i) {
    qreal xSeg = zigStart + i * segW;
    qreal dir = (i % 2 == 0) ? -1.0 : 1.0;
    path.lineTo(xSeg + segW * 0.5, y0 + dir * amp);
    path.lineTo(xSeg + segW, y0);
  }
  path.lineTo(xR, y0);
  p->drawPath(path);
}

void drawCapacitorIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                       qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(Qt::NoBrush);

  const qreal cx = r.left() + r.width() * 0.5;
  const qreal gap = r.width() * 0.10;
  const qreal plateH = r.height() * 0.55;
  const qreal topY = r.top() + (r.height() - plateH) * 0.5;
  const qreal botY = topY + plateH;

  // Left plate.
  p->drawLine(QPointF(cx - gap, topY), QPointF(cx - gap, botY));
  // Right plate.
  p->drawLine(QPointF(cx + gap, topY), QPointF(cx + gap, botY));
  // Left terminal.
  p->drawLine(QPointF(r.left(), r.top() + r.height() * 0.5),
              QPointF(cx - gap, r.top() + r.height() * 0.5));
  // Right terminal.
  p->drawLine(QPointF(cx + gap, r.top() + r.height() * 0.5),
              QPointF(r.right(), r.top() + r.height() * 0.5));
}

void drawInductorIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                      qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(Qt::NoBrush);

  const qreal y0 = r.top() + r.height() * 0.55;
  const qreal xL = r.left();
  const qreal xR = r.right();
  const qreal stubLen = r.width() * 0.12;
  const qreal coilStart = xL + stubLen;
  const qreal coilEnd = xR - stubLen;
  const int humps = 4;
  const qreal humpW = (coilEnd - coilStart) / humps;
  const qreal humpH = r.height() * 0.38;

  // Terminal stubs.
  p->drawLine(QPointF(xL, y0), QPointF(coilStart, y0));
  p->drawLine(QPointF(coilEnd, y0), QPointF(xR, y0));

  // Humps (arcs).
  QPainterPath path;
  path.moveTo(coilStart, y0);
  for (int i = 0; i < humps; ++i) {
    qreal sx = coilStart + i * humpW;
    path.cubicTo(sx + humpW * 0.15, y0 - humpH, sx + humpW * 0.85,
                 y0 - humpH, sx + humpW, y0);
  }
  p->drawPath(path);
}

void drawFuseIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                  qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  const qreal midY = r.top() + r.height() * 0.5;
  const QRectF body = uvRect(r, 0.20, 0.22, 0.60, 0.56);
  p->drawRect(body);

  // Terminal stubs.
  p->drawLine(QPointF(r.left(), midY), QPointF(body.left(), midY));
  p->drawLine(QPointF(body.right(), midY), QPointF(r.right(), midY));

  // S-curve inside the body.
  QPainterPath s;
  s.moveTo(body.left() + body.width() * 0.25, body.top() + body.height() * 0.2);
  s.cubicTo(body.left() + body.width() * 0.65, body.top() + body.height() * 0.15,
            body.left() + body.width() * 0.35, body.top() + body.height() * 0.85,
            body.left() + body.width() * 0.75, body.top() + body.height() * 0.8);
  p->drawPath(s);
}

void drawCrystalIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                     qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(Qt::NoBrush);

  const qreal midY = r.top() + r.height() * 0.5;
  const qreal plateH = r.height() * 0.50;
  const qreal plateTop = r.top() + (r.height() - plateH) * 0.5;
  const qreal plateBot = plateTop + plateH;
  const qreal lx = r.left() + r.width() * 0.30;
  const qreal rx = r.left() + r.width() * 0.70;

  // Outer plates.
  p->drawLine(QPointF(lx, plateTop), QPointF(lx, plateBot));
  p->drawLine(QPointF(rx, plateTop), QPointF(rx, plateBot));

  // Inner rectangle between plates.
  const QRectF inner = uvRect(r, 0.37, 0.28, 0.26, 0.44);
  p->drawRect(inner);

  // Terminal stubs.
  p->drawLine(QPointF(r.left(), midY), QPointF(lx, midY));
  p->drawLine(QPointF(rx, midY), QPointF(r.right(), midY));
}

void drawTransformerIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                         qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(Qt::NoBrush);

  const qreal cx = r.left() + r.width() * 0.5;
  const qreal coilTop = r.top() + r.height() * 0.10;
  const qreal coilBot = r.top() + r.height() * 0.90;
  const int humps = 3;
  const qreal humpH = (coilBot - coilTop) / humps;
  const qreal humpW = r.width() * 0.22;

  // Left coil (humps going left).
  QPainterPath left;
  left.moveTo(cx - r.width() * 0.08, coilTop);
  for (int i = 0; i < humps; ++i) {
    qreal sy = coilTop + i * humpH;
    left.cubicTo(cx - r.width() * 0.08 - humpW, sy + humpH * 0.15,
                 cx - r.width() * 0.08 - humpW, sy + humpH * 0.85,
                 cx - r.width() * 0.08, sy + humpH);
  }
  p->drawPath(left);

  // Right coil (humps going right).
  QPainterPath right;
  right.moveTo(cx + r.width() * 0.08, coilTop);
  for (int i = 0; i < humps; ++i) {
    qreal sy = coilTop + i * humpH;
    right.cubicTo(cx + r.width() * 0.08 + humpW, sy + humpH * 0.15,
                  cx + r.width() * 0.08 + humpW, sy + humpH * 0.85,
                  cx + r.width() * 0.08, sy + humpH);
  }
  p->drawPath(right);

  // Core lines.
  p->drawLine(QPointF(cx - r.width() * 0.02, coilTop),
              QPointF(cx - r.width() * 0.02, coilBot));
  p->drawLine(QPointF(cx + r.width() * 0.02, coilTop),
              QPointF(cx + r.width() * 0.02, coilBot));
}

void drawDiodeIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                   qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 38));

  const qreal midY = r.top() + r.height() * 0.5;
  const qreal triLeft = r.left() + r.width() * 0.25;
  const qreal triRight = r.left() + r.width() * 0.70;
  const qreal triH = r.height() * 0.45;

  // Triangle pointing right.
  QPolygonF tri;
  tri << QPointF(triLeft, midY - triH)
      << QPointF(triRight, midY)
      << QPointF(triLeft, midY + triH);
  p->drawPolygon(tri);

  // Cathode bar.
  p->drawLine(QPointF(triRight, midY - triH),
              QPointF(triRight, midY + triH));

  // Terminal stubs.
  p->drawLine(QPointF(r.left(), midY), QPointF(triLeft, midY));
  p->drawLine(QPointF(triRight, midY), QPointF(r.right(), midY));
}

void drawLEDIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                 qreal width) {
  // Draw diode body first.
  drawDiodeIcon(p, r, stroke, width);

  // Two small arrows pointing away (emission).
  p->setPen(QPen(stroke, width * 0.75, Qt::SolidLine, Qt::RoundCap,
                 Qt::RoundJoin));
  const qreal ax = r.left() + r.width() * 0.60;
  const qreal ay1 = r.top() + r.height() * 0.10;
  const qreal ay2 = r.top() + r.height() * 0.22;
  const qreal arrowLen = r.width() * 0.18;

  // Arrow 1.
  p->drawLine(QPointF(ax, ay1), QPointF(ax + arrowLen, ay1 - arrowLen * 0.5));
  p->drawLine(QPointF(ax + arrowLen, ay1 - arrowLen * 0.5),
              QPointF(ax + arrowLen * 0.55, ay1 - arrowLen * 0.35));
  // Arrow 2.
  p->drawLine(QPointF(ax + r.width() * 0.08, ay2),
              QPointF(ax + r.width() * 0.08 + arrowLen,
                      ay2 - arrowLen * 0.5));
  p->drawLine(QPointF(ax + r.width() * 0.08 + arrowLen,
                       ay2 - arrowLen * 0.5),
              QPointF(ax + r.width() * 0.08 + arrowLen * 0.55,
                      ay2 - arrowLen * 0.35));
}

void drawTransistorIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                        qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  // Circle body.
  const QRectF body = uvRect(r, 0.15, 0.10, 0.70, 0.80);
  p->drawEllipse(body);

  // Base line (vertical bar inside left part of circle).
  const qreal bx = r.left() + r.width() * 0.40;
  p->drawLine(QPointF(bx, r.top() + r.height() * 0.28),
              QPointF(bx, r.top() + r.height() * 0.72));

  // Collector (line from bar up-right).
  p->drawLine(QPointF(bx, r.top() + r.height() * 0.36),
              QPointF(r.left() + r.width() * 0.72, r.top() + r.height() * 0.15));

  // Emitter (line from bar down-right with arrow).
  const QPointF emBase(bx, r.top() + r.height() * 0.64);
  const QPointF emTip(r.left() + r.width() * 0.72,
                      r.top() + r.height() * 0.85);
  p->drawLine(emBase, emTip);

  // Small arrowhead on emitter.
  p->setBrush(stroke);
  const qreal hs = r.width() * 0.10;
  const qreal dx = emTip.x() - emBase.x();
  const qreal dy = emTip.y() - emBase.y();
  const qreal len = std::hypot(dx, dy);
  const qreal ux = dx / len;
  const qreal uy = dy / len;
  const QPointF n(-uy, ux);
  QPolygonF arrow;
  arrow << emTip
        << QPointF(emTip.x() - ux * hs + n.x() * hs * 0.5,
                   emTip.y() - uy * hs + n.y() * hs * 0.5)
        << QPointF(emTip.x() - ux * hs - n.x() * hs * 0.5,
                   emTip.y() - uy * hs - n.y() * hs * 0.5);
  p->drawPolygon(arrow);
  p->setBrush(Qt::NoBrush);

  // Base terminal stub.
  p->drawLine(QPointF(r.left(), r.top() + r.height() * 0.5),
              QPointF(bx, r.top() + r.height() * 0.5));
}

void drawMOSFETIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                    qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  const qreal gateX = r.left() + r.width() * 0.30;
  const qreal bodyX = r.left() + r.width() * 0.40;
  const qreal midY = r.top() + r.height() * 0.5;

  // Gate terminal (horizontal from left to gate line).
  p->drawLine(QPointF(r.left(), midY), QPointF(gateX, midY));
  // Gate vertical line.
  p->drawLine(QPointF(gateX, r.top() + r.height() * 0.20),
              QPointF(gateX, r.top() + r.height() * 0.80));

  // Body channel (three short segments, insulated from gate).
  const qreal segH = r.height() * 0.16;
  for (int i = 0; i < 3; ++i) {
    qreal sy = r.top() + r.height() * 0.22 + i * (segH + r.height() * 0.05);
    p->drawLine(QPointF(bodyX, sy), QPointF(bodyX, sy + segH));
  }

  // Drain (top terminal).
  const qreal drainY = r.top() + r.height() * 0.22 + segH * 0.5;
  p->drawLine(QPointF(bodyX, drainY),
              QPointF(r.right(), drainY));
  p->drawLine(QPointF(r.right(), r.top()), QPointF(r.right(), drainY));

  // Source (bottom terminal).
  const qreal srcY = r.top() + r.height() * 0.22 + 2 * (segH + r.height() * 0.05) + segH * 0.5;
  p->drawLine(QPointF(bodyX, srcY), QPointF(r.right(), srcY));
  p->drawLine(QPointF(r.right(), srcY), QPointF(r.right(), r.bottom()));

  // Arrow on source.
  p->setBrush(stroke);
  const qreal ax = bodyX + (r.right() - bodyX) * 0.3;
  const qreal ahs = r.width() * 0.08;
  QPolygonF arrow;
  arrow << QPointF(bodyX + r.width() * 0.04, srcY)
        << QPointF(ax, srcY - ahs)
        << QPointF(ax, srcY + ahs);
  p->drawPolygon(arrow);
  p->setBrush(Qt::NoBrush);
}

void drawOpAmpIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                   qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 28));

  // Triangle body.
  const qreal triLeft = r.left() + r.width() * 0.15;
  const qreal triRight = r.left() + r.width() * 0.82;
  const qreal midY = r.top() + r.height() * 0.5;
  const qreal triH = r.height() * 0.45;

  QPolygonF tri;
  tri << QPointF(triLeft, midY - triH)
      << QPointF(triRight, midY)
      << QPointF(triLeft, midY + triH);
  p->drawPolygon(tri);

  // "−" at inverting input.
  const qreal symX = triLeft + r.width() * 0.06;
  const qreal symLen = r.width() * 0.10;
  const qreal minusY = midY - triH * 0.47;
  p->drawLine(QPointF(symX, minusY), QPointF(symX + symLen, minusY));

  // "+" at non-inverting input.
  const qreal plusY = midY + triH * 0.47;
  p->drawLine(QPointF(symX, plusY), QPointF(symX + symLen, plusY));
  p->drawLine(QPointF(symX + symLen * 0.5, plusY - symLen * 0.5),
              QPointF(symX + symLen * 0.5, plusY + symLen * 0.5));

  // Input stubs.
  p->drawLine(QPointF(r.left(), minusY), QPointF(triLeft, minusY));
  p->drawLine(QPointF(r.left(), plusY), QPointF(triLeft, plusY));
  // Output stub.
  p->drawLine(QPointF(triRight, midY), QPointF(r.right(), midY));
}

void drawVoltageRegulatorIcon(QPainter *p, const QRectF &r,
                              const QColor &stroke, qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 22));

  // Body rectangle.
  const QRectF body = uvRect(r, 0.15, 0.12, 0.70, 0.56);
  p->drawRect(body);

  // "In" label with arrow stub on the left.
  p->drawLine(QPointF(r.left(), body.center().y()),
              QPointF(body.left(), body.center().y()));

  // "Out" arrow stub on the right.
  p->drawLine(QPointF(body.right(), body.center().y()),
              QPointF(r.right(), body.center().y()));

  // GND stub going down from center bottom.
  p->drawLine(QPointF(body.center().x(), body.bottom()),
              QPointF(body.center().x(), r.bottom()));

  // Small labels.
  QFont f("Segoe UI", qMax(1, qRound(r.height() * 0.13)), QFont::Bold);
  p->setFont(f);
  p->drawText(uvRect(r, 0.18, 0.18, 0.25, 0.22), Qt::AlignCenter, "In");
  p->drawText(uvRect(r, 0.55, 0.18, 0.28, 0.22), Qt::AlignCenter, "Out");
  p->drawText(uvRect(r, 0.32, 0.42, 0.36, 0.22), Qt::AlignCenter, "GND");
}

void drawBatteryIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                     qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(Qt::NoBrush);

  const qreal cx = r.left() + r.width() * 0.5;
  const int cells = 3;
  const qreal totalH = r.height() * 0.70;
  const qreal cellH = totalH / cells;
  const qreal startY = r.top() + (r.height() - totalH) * 0.5;

  // Top terminal.
  p->drawLine(QPointF(cx, r.top()), QPointF(cx, startY));

  for (int i = 0; i < cells; ++i) {
    qreal y = startY + i * cellH;
    // Long plate (thin cell positive).
    qreal longW = r.width() * 0.55;
    p->drawLine(QPointF(cx - longW * 0.5, y), QPointF(cx + longW * 0.5, y));
    // Short plate (thick cell negative).
    qreal shortW = r.width() * 0.30;
    qreal shortY = y + cellH * 0.45;
    QPen thickPen(stroke, width * 1.6, Qt::SolidLine, Qt::RoundCap);
    p->setPen(thickPen);
    p->drawLine(QPointF(cx - shortW * 0.5, shortY),
                QPointF(cx + shortW * 0.5, shortY));
    p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  }

  // Bottom terminal.
  p->drawLine(QPointF(cx, startY + totalH), QPointF(cx, r.bottom()));
}

void drawGroundIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                    qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(Qt::NoBrush);

  const qreal cx = r.left() + r.width() * 0.5;
  const qreal topY = r.top() + r.height() * 0.10;
  const qreal gndTop = r.top() + r.height() * 0.40;

  // Vertical stub down.
  p->drawLine(QPointF(cx, topY), QPointF(cx, gndTop));

  // Three horizontal lines decreasing in width.
  const qreal lineSpacing = r.height() * 0.15;
  for (int i = 0; i < 3; ++i) {
    qreal y = gndTop + i * lineSpacing;
    qreal halfW = r.width() * (0.45 - i * 0.13);
    p->drawLine(QPointF(cx - halfW, y), QPointF(cx + halfW, y));
  }
}

void drawSwitchIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                    qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  const qreal midY = r.top() + r.height() * 0.55;
  const qreal leftX = r.left() + r.width() * 0.15;
  const qreal rightX = r.left() + r.width() * 0.85;
  const qreal dotR = r.width() * 0.06;

  // Terminal stubs.
  p->drawLine(QPointF(r.left(), midY), QPointF(leftX, midY));
  p->drawLine(QPointF(rightX, midY), QPointF(r.right(), midY));

  // Terminal dots.
  p->setBrush(stroke);
  p->drawEllipse(QPointF(leftX, midY), dotR, dotR);
  p->drawEllipse(QPointF(rightX, midY), dotR, dotR);
  p->setBrush(Qt::NoBrush);

  // Hinged lever (open switch – angled line from left dot upward to near right).
  p->drawLine(QPointF(leftX, midY),
              QPointF(rightX - r.width() * 0.05,
                      midY - r.height() * 0.35));
}

void drawRelayIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                   qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  // Coil box (bottom half).
  const QRectF coilBox = uvRect(r, 0.15, 0.52, 0.70, 0.40);
  p->drawRect(coilBox);

  // Coil windings inside (simple arcs).
  const int humps = 3;
  const qreal humpW = coilBox.width() / humps;
  QPainterPath coil;
  coil.moveTo(coilBox.left(), coilBox.center().y());
  for (int i = 0; i < humps; ++i) {
    qreal sx = coilBox.left() + i * humpW;
    coil.cubicTo(sx + humpW * 0.15, coilBox.top() + coilBox.height() * 0.1,
                 sx + humpW * 0.85, coilBox.top() + coilBox.height() * 0.1,
                 sx + humpW, coilBox.center().y());
  }
  p->drawPath(coil);

  // Switch contacts (top half).
  const qreal contactY = r.top() + r.height() * 0.35;
  const qreal leftP = r.left() + r.width() * 0.25;
  const qreal rightP = r.left() + r.width() * 0.75;
  const qreal dotR = r.width() * 0.05;

  p->setBrush(stroke);
  p->drawEllipse(QPointF(leftP, contactY), dotR, dotR);
  p->drawEllipse(QPointF(rightP, contactY), dotR, dotR);
  p->setBrush(Qt::NoBrush);

  // Lever between contacts.
  p->drawLine(QPointF(leftP, contactY),
              QPointF(rightP - r.width() * 0.04,
                      contactY - r.height() * 0.18));

  // Dashed line connecting coil to switch.
  QPen dashed(stroke, width * 0.6, Qt::DashLine);
  p->setPen(dashed);
  p->drawLine(QPointF(r.left() + r.width() * 0.5, coilBox.top()),
              QPointF(r.left() + r.width() * 0.5, contactY));
}

void drawMotorIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                   qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(withAlpha(stroke, 28));

  // Circle.
  const QRectF body = uvRect(r, 0.15, 0.10, 0.70, 0.70);
  p->drawEllipse(body);

  // "M" label.
  QFont f("Segoe UI", qMax(1, qRound(r.height() * 0.24)), QFont::Bold);
  p->setFont(f);
  p->drawText(body, Qt::AlignCenter, "M");

  // Terminal stubs.
  p->drawLine(QPointF(r.left(), body.center().y()),
              QPointF(body.left(), body.center().y()));
  p->drawLine(QPointF(body.right(), body.center().y()),
              QPointF(r.right(), body.center().y()));
}

void drawPowerSupplyIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                         qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 22));

  // Outer body.
  const QRectF body = uvRect(r, 0.08, 0.15, 0.84, 0.60);
  p->drawRoundedRect(body, width * 1.5, width * 1.5);

  // Plug prongs on the left.
  const qreal px = body.left();
  const qreal py1 = body.top() + body.height() * 0.30;
  const qreal py2 = body.top() + body.height() * 0.70;
  const qreal pLen = r.width() * 0.10;
  p->drawLine(QPointF(px, py1), QPointF(px - pLen, py1));
  p->drawLine(QPointF(px, py2), QPointF(px - pLen, py2));

  // "AC~" label.
  QFont f("Segoe UI", qMax(1, qRound(r.height() * 0.16)), QFont::Bold);
  p->setFont(f);
  p->drawText(body, Qt::AlignCenter, "AC~");
}

void drawMicrocontrollerIcon(QPainter *p, const QRectF &r,
                             const QColor &stroke, qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 22));

  // DIP package body.
  const QRectF body = uvRect(r, 0.20, 0.08, 0.60, 0.84);
  p->drawRect(body);

  // Notch at top center.
  const qreal notchR = r.width() * 0.06;
  p->setBrush(QColor(0, 0, 0, 60));
  p->drawEllipse(QPointF(body.center().x(), body.top()), notchR, notchR);
  p->setBrush(Qt::NoBrush);

  // Pins on left side.
  const int pins = 4;
  const qreal pinLen = r.width() * 0.12;
  const qreal pinSpacing = body.height() / (pins + 1);
  for (int i = 1; i <= pins; ++i) {
    qreal py = body.top() + i * pinSpacing;
    p->drawLine(QPointF(body.left(), py),
                QPointF(body.left() - pinLen, py));
  }

  // Pins on right side.
  for (int i = 1; i <= pins; ++i) {
    qreal py = body.top() + i * pinSpacing;
    p->drawLine(QPointF(body.right(), py),
                QPointF(body.right() + pinLen, py));
  }
}

void drawICChipIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                    qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 22));

  // Square chip body.
  const QRectF body = uvRect(r, 0.18, 0.18, 0.64, 0.64);
  p->drawRect(body);

  // Corner notch (top-left).
  const qreal notchSize = body.width() * 0.18;
  QPainterPath notch;
  notch.moveTo(body.left(), body.top() + notchSize);
  notch.lineTo(body.left() + notchSize, body.top());
  p->setBrush(Qt::NoBrush);
  p->drawPath(notch);

  // Pins on all four sides (2 per side).
  const qreal pinLen = r.width() * 0.10;
  for (int i = 0; i < 2; ++i) {
    qreal frac = 0.35 + i * 0.30;
    qreal px = body.left() + frac * body.width();
    qreal py = body.top() + frac * body.height();

    // Top pins.
    p->drawLine(QPointF(px, body.top()), QPointF(px, body.top() - pinLen));
    // Bottom pins.
    p->drawLine(QPointF(px, body.bottom()),
                QPointF(px, body.bottom() + pinLen));
    // Left pins.
    p->drawLine(QPointF(body.left(), py),
                QPointF(body.left() - pinLen, py));
    // Right pins.
    p->drawLine(QPointF(body.right(), py),
                QPointF(body.right() + pinLen, py));
  }
}

void drawSensorIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                    qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap));
  p->setBrush(withAlpha(stroke, 28));

  // Sensor body (circle).
  const QRectF body = uvRect(r, 0.08, 0.20, 0.45, 0.55);
  p->drawEllipse(body);
  p->setBrush(Qt::NoBrush);

  // Detection/wave arcs emanating to the right.
  for (int i = 1; i <= 3; ++i) {
    qreal arcR = r.width() * (0.15 + i * 0.10);
    QRectF arcRect(body.center().x() - arcR, body.center().y() - arcR,
                   arcR * 2.0, arcR * 2.0);
    p->drawArc(arcRect, -45 * 16, 90 * 16);
  }
}

void drawAntennaIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                     qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(Qt::NoBrush);

  const qreal cx = r.left() + r.width() * 0.5;

  // Vertical mast.
  p->drawLine(QPointF(cx, r.bottom()), QPointF(cx, r.top() + r.height() * 0.30));

  // V-shaped antenna arms at top.
  const qreal tipY = r.top() + r.height() * 0.30;
  p->drawLine(QPointF(cx, tipY),
              QPointF(r.left() + r.width() * 0.15, r.top() + r.height() * 0.05));
  p->drawLine(QPointF(cx, tipY),
              QPointF(r.left() + r.width() * 0.85, r.top() + r.height() * 0.05));

  // Radiating arcs.
  for (int i = 1; i <= 2; ++i) {
    qreal arcR = r.width() * (0.10 + i * 0.12);
    QRectF arcRect(cx - arcR, tipY - arcR * 1.2, arcR * 2.0, arcR * 2.0);
    p->drawArc(arcRect, 30 * 16, 120 * 16);
  }
}

void drawSpeakerIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                     qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 32));

  // Speaker body: small rectangle on left + trapezoid expanding to right.
  const qreal boxL = r.left() + r.width() * 0.10;
  const qreal boxR = r.left() + r.width() * 0.30;
  const qreal coneR = r.left() + r.width() * 0.65;
  const qreal midY = r.top() + r.height() * 0.50;
  const qreal boxH = r.height() * 0.25;
  const qreal coneH = r.height() * 0.42;

  // Driver box.
  p->drawRect(QRectF(boxL, midY - boxH, boxR - boxL, boxH * 2.0));

  // Cone (trapezoid).
  QPolygonF cone;
  cone << QPointF(boxR, midY - boxH)
       << QPointF(coneR, midY - coneH)
       << QPointF(coneR, midY + coneH)
       << QPointF(boxR, midY + boxH);
  p->drawPolygon(cone);
  p->setBrush(Qt::NoBrush);

  // Sound wave arcs.
  for (int i = 1; i <= 2; ++i) {
    qreal arcR = r.width() * (0.08 + i * 0.10);
    QRectF arcRect(coneR + r.width() * 0.02 - arcR, midY - arcR,
                   arcR * 2.0, arcR * 2.0);
    p->drawArc(arcRect, -40 * 16, 80 * 16);
  }
}

void drawConnectorIcon(QPainter *p, const QRectF &r, const QColor &stroke,
                       qreal width) {
  p->setPen(QPen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p->setBrush(withAlpha(stroke, 22));

  // Plug body (left half).
  const QRectF plug = uvRect(r, 0.05, 0.18, 0.40, 0.64);
  p->drawRoundedRect(plug, width, width);

  // Socket body (right half).
  const QRectF sock = uvRect(r, 0.55, 0.18, 0.40, 0.64);
  p->drawRoundedRect(sock, width, width);
  p->setBrush(Qt::NoBrush);

  // Plug pins (protruding right from plug).
  const int pins = 3;
  const qreal pinLen = r.width() * 0.10;
  const qreal spacing = plug.height() / (pins + 1);
  for (int i = 1; i <= pins; ++i) {
    qreal py = plug.top() + i * spacing;
    p->drawLine(QPointF(plug.right(), py),
                QPointF(plug.right() + pinLen, py));
  }

  // Socket holes (small circles inside socket aligned with pins).
  const qreal holeR = r.width() * 0.03;
  for (int i = 1; i <= pins; ++i) {
    qreal py = sock.top() + i * spacing;
    p->drawEllipse(QPointF(sock.left() + sock.width() * 0.3, py), holeR,
                   holeR);
  }
}

} // namespace

// ===========================================================================
// ElectronicsElementItem  (base)
// ===========================================================================

ElectronicsElementItem::ElectronicsElementItem(const QString &label,
                                               IconKind iconKind,
                                               const QColor &accentColor,
                                               QGraphicsItem *parent)
    : QGraphicsItem(parent), label_(label), iconKind_(iconKind),
      accentColor_(accentColor) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
  initPins();
}

ElectronicsElementItem::~ElectronicsElementItem() {
  for (WireItem *w : connectedWires_)
    w->detachElement(this);
}

QRectF ElectronicsElementItem::boundingRect() const {
  const qreal m = PIN_RADIUS + 1.0;
  return QRectF(-m, -m, ELEM_W + 2 * m, ELEM_H + 2 * m);
}

void ElectronicsElementItem::paint(QPainter *painter,
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

  // Draw connection-pin indicators.
  for (const ElectronicsPin &pin : pins_) {
    painter->setPen(QPen(withAlpha(accent.lighter(170), 200), 1.2));
    painter->setBrush(QColor(255, 255, 255, 180));
    painter->drawEllipse(pin.offset, PIN_RADIUS, PIN_RADIUS);
  }
}

void ElectronicsElementItem::paintIcon(QPainter *painter,
                                       const QRectF &rect) const {
  const QColor stroke = accentColor_.lighter(225);
  const qreal width = qMax(1.25, rect.width() * 0.09);

  switch (iconKind_) {
  case IconKind::Resistor:
    drawResistorIcon(painter, rect, stroke, width);
    break;
  case IconKind::Capacitor:
    drawCapacitorIcon(painter, rect, stroke, width);
    break;
  case IconKind::Inductor:
    drawInductorIcon(painter, rect, stroke, width);
    break;
  case IconKind::Fuse:
    drawFuseIcon(painter, rect, stroke, width);
    break;
  case IconKind::Crystal:
    drawCrystalIcon(painter, rect, stroke, width);
    break;
  case IconKind::Transformer:
    drawTransformerIcon(painter, rect, stroke, width);
    break;
  case IconKind::Diode:
    drawDiodeIcon(painter, rect, stroke, width);
    break;
  case IconKind::LED:
    drawLEDIcon(painter, rect, stroke, width);
    break;
  case IconKind::Transistor:
    drawTransistorIcon(painter, rect, stroke, width);
    break;
  case IconKind::MOSFET:
    drawMOSFETIcon(painter, rect, stroke, width);
    break;
  case IconKind::OpAmp:
    drawOpAmpIcon(painter, rect, stroke, width);
    break;
  case IconKind::VoltageRegulator:
    drawVoltageRegulatorIcon(painter, rect, stroke, width);
    break;
  case IconKind::Battery:
    drawBatteryIcon(painter, rect, stroke, width);
    break;
  case IconKind::Ground:
    drawGroundIcon(painter, rect, stroke, width);
    break;
  case IconKind::Switch:
    drawSwitchIcon(painter, rect, stroke, width);
    break;
  case IconKind::Relay:
    drawRelayIcon(painter, rect, stroke, width);
    break;
  case IconKind::Motor:
    drawMotorIcon(painter, rect, stroke, width);
    break;
  case IconKind::PowerSupply:
    drawPowerSupplyIcon(painter, rect, stroke, width);
    break;
  case IconKind::Microcontroller:
    drawMicrocontrollerIcon(painter, rect, stroke, width);
    break;
  case IconKind::ICChip:
    drawICChipIcon(painter, rect, stroke, width);
    break;
  case IconKind::Sensor:
    drawSensorIcon(painter, rect, stroke, width);
    break;
  case IconKind::Antenna:
    drawAntennaIcon(painter, rect, stroke, width);
    break;
  case IconKind::Speaker:
    drawSpeakerIcon(painter, rect, stroke, width);
    break;
  case IconKind::Connector:
    drawConnectorIcon(painter, rect, stroke, width);
    break;
  }
}

// ===========================================================================
// Pin / wire helpers
// ===========================================================================

QPointF ElectronicsElementItem::pinScenePos(int index) const {
  if (index < 0 || index >= pins_.size())
    return mapToScene(QPointF(ELEM_W / 2, ELEM_H / 2));
  return mapToScene(pins_[index].offset);
}

int ElectronicsElementItem::nearestPin(const QPointF &scenePos,
                                       qreal maxDist) const {
  int best = -1;
  qreal bestD = maxDist;
  for (int i = 0; i < pins_.size(); ++i) {
    const qreal d = QLineF(pinScenePos(i), scenePos).length();
    if (d < bestD) {
      bestD = d;
      best = i;
    }
  }
  return best;
}

void ElectronicsElementItem::addWire(WireItem *wire) {
  if (wire && !connectedWires_.contains(wire))
    connectedWires_.append(wire);
}

void ElectronicsElementItem::removeWire(WireItem *wire) {
  connectedWires_.removeAll(wire);
}

QVariant ElectronicsElementItem::itemChange(GraphicsItemChange change,
                                            const QVariant &value) {
  if (change == ItemPositionHasChanged) {
    for (WireItem *w : connectedWires_)
      w->updatePath();
  }
  return QGraphicsItem::itemChange(change, value);
}

void ElectronicsElementItem::initPins() {
  const qreal midY = ELEM_H / 2.0;
  const qreal midX = ELEM_W / 2.0;

  switch (iconKind_) {
  // --- Two-terminal (left / right) ----------------------------------------
  case IconKind::Resistor:
  case IconKind::Capacitor:
  case IconKind::Inductor:
  case IconKind::Fuse:
  case IconKind::Crystal:
  case IconKind::Diode:
  case IconKind::LED:
  case IconKind::Switch:
  case IconKind::Motor:
  case IconKind::Speaker:
  case IconKind::Connector:
    pins_ = {{QStringLiteral("pin1"), {0.0, midY}},
             {QStringLiteral("pin2"), {ELEM_W, midY}}};
    break;

  // --- Four-terminal (transformer, relay) ---------------------------------
  case IconKind::Transformer:
  case IconKind::Relay:
    pins_ = {{QStringLiteral("pin1"), {0.0, midY - 18.0}},
             {QStringLiteral("pin2"), {0.0, midY + 18.0}},
             {QStringLiteral("pin3"), {ELEM_W, midY - 18.0}},
             {QStringLiteral("pin4"), {ELEM_W, midY + 18.0}}};
    break;

  // --- Three-terminal (transistor, MOSFET, OpAmp, regulator) ---------------
  case IconKind::Transistor:
  case IconKind::MOSFET:
    pins_ = {{QStringLiteral("base"), {0.0, midY}},
             {QStringLiteral("collector"), {ELEM_W, midY - 18.0}},
             {QStringLiteral("emitter"), {ELEM_W, midY + 18.0}}};
    break;

  case IconKind::OpAmp:
    pins_ = {{QStringLiteral("+in"), {0.0, midY - 18.0}},
             {QStringLiteral("-in"), {0.0, midY + 18.0}},
             {QStringLiteral("out"), {ELEM_W, midY}}};
    break;

  case IconKind::VoltageRegulator:
    pins_ = {{QStringLiteral("in"), {0.0, midY}},
             {QStringLiteral("out"), {ELEM_W, midY}},
             {QStringLiteral("gnd"), {midX, ELEM_H}}};
    break;

  // --- Battery / power supply (two terminals, top / bottom style) ----------
  case IconKind::Battery:
  case IconKind::PowerSupply:
    pins_ = {{QStringLiteral("+"), {0.0, midY}},
             {QStringLiteral("-"), {ELEM_W, midY}}};
    break;

  // --- Ground (single terminal) -------------------------------------------
  case IconKind::Ground:
    pins_ = {{QStringLiteral("gnd"), {midX, 0.0}}};
    break;

  // --- Antenna (single terminal) ------------------------------------------
  case IconKind::Antenna:
    pins_ = {{QStringLiteral("feed"), {midX, ELEM_H}}};
    break;

  // --- Multi-pin ICs / MCU / Sensor (four symmetric pins) ------------------
  case IconKind::Microcontroller:
  case IconKind::ICChip:
  case IconKind::Sensor:
    pins_ = {{QStringLiteral("pin1"), {0.0, midY - 18.0}},
             {QStringLiteral("pin2"), {0.0, midY + 18.0}},
             {QStringLiteral("pin3"), {ELEM_W, midY - 18.0}},
             {QStringLiteral("pin4"), {ELEM_W, midY + 18.0}}};
    break;
  }
}

// ===========================================================================
// Concrete element constructors
// ===========================================================================

ResistorElement::ResistorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Resistor", IconKind::Resistor, QColor("#e67e22"),
                             parent) {}

CapacitorElement::CapacitorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Capacitor", IconKind::Capacitor,
                             QColor("#3498db"), parent) {}

InductorElement::InductorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Inductor", IconKind::Inductor, QColor("#9b59b6"),
                             parent) {}

FuseElement::FuseElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Fuse", IconKind::Fuse, QColor("#e74c3c"),
                             parent) {}

CrystalElement::CrystalElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Crystal", IconKind::Crystal, QColor("#1abc9c"),
                             parent) {}

TransformerElement::TransformerElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Transformer", IconKind::Transformer,
                             QColor("#f39c12"), parent) {}

DiodeElement::DiodeElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Diode", IconKind::Diode, QColor("#2ecc71"),
                             parent) {}

LEDElement::LEDElement(QGraphicsItem *parent)
    : ElectronicsElementItem("LED", IconKind::LED, QColor("#f1c40f"),
                             parent) {}

TransistorElement::TransistorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Transistor", IconKind::Transistor,
                             QColor("#e74c3c"), parent) {}

MOSFETElement::MOSFETElement(QGraphicsItem *parent)
    : ElectronicsElementItem("MOSFET", IconKind::MOSFET, QColor("#8e44ad"),
                             parent) {}

OpAmpElement::OpAmpElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Op-Amp", IconKind::OpAmp, QColor("#2980b9"),
                             parent) {}

VoltageRegulatorElement::VoltageRegulatorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Regulator", IconKind::VoltageRegulator,
                             QColor("#27ae60"), parent) {}

BatteryElement::BatteryElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Battery", IconKind::Battery, QColor("#f39c12"),
                             parent) {}

GroundElement::GroundElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Ground", IconKind::Ground, QColor("#7f8c8d"),
                             parent) {}

SwitchElement::SwitchElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Switch", IconKind::Switch, QColor("#e67e22"),
                             parent) {}

RelayElement::RelayElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Relay", IconKind::Relay, QColor("#d35400"),
                             parent) {}

MotorElement::MotorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Motor", IconKind::Motor, QColor("#16a085"),
                             parent) {}

PowerSupplyElement::PowerSupplyElement(QGraphicsItem *parent)
    : ElectronicsElementItem("PSU", IconKind::PowerSupply, QColor("#c0392b"),
                             parent) {}

MicrocontrollerElement::MicrocontrollerElement(QGraphicsItem *parent)
    : ElectronicsElementItem("MCU", IconKind::Microcontroller,
                             QColor("#2c3e50"), parent) {}

ICChipElement::ICChipElement(QGraphicsItem *parent)
    : ElectronicsElementItem("IC Chip", IconKind::ICChip, QColor("#34495e"),
                             parent) {}

SensorElement::SensorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Sensor", IconKind::Sensor, QColor("#1abc9c"),
                             parent) {}

AntennaElement::AntennaElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Antenna", IconKind::Antenna, QColor("#3498db"),
                             parent) {}

SpeakerElement::SpeakerElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Speaker", IconKind::Speaker, QColor("#9b59b6"),
                             parent) {}

ConnectorElement::ConnectorElement(QGraphicsItem *parent)
    : ElectronicsElementItem("Connector", IconKind::Connector,
                             QColor("#95a5a6"), parent) {}
