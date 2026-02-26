/**
 * @file brush_tip.cpp
 * @brief Custom brush tip implementation.
 */
#include "brush_tip.h"
#include <QPainter>
#include <QtMath>

BrushTip::BrushTip() = default;
BrushTip::~BrushTip() = default;

QPainterPath BrushTip::tipShape(qreal size) const {
  QPainterPath path;
  switch (shape_) {
  case BrushTipShape::Chisel: {
    // Thin rectangle rotated by angle_
    qreal halfLen = size / 2.0;
    qreal halfWidth = size / 8.0;
    QRectF rect(-halfLen, -halfWidth, size, size / 4.0);
    QTransform rot;
    rot.rotate(angle_);
    path.addPolygon(rot.map(QPolygonF(rect)));
    path.closeSubpath();
    break;
  }
  case BrushTipShape::Stamp:
  case BrushTipShape::Textured:
  case BrushTipShape::Round:
  default:
    path.addEllipse(QPointF(0, 0), size / 2.0, size / 2.0);
    break;
  }
  return path;
}

QImage BrushTip::renderTip(qreal size, const QColor &color,
                           qreal opacity) const {
  int dim = qMax(1, static_cast<int>(qCeil(size)));
  QImage img(dim, dim, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::transparent);

  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  p.translate(dim / 2.0, dim / 2.0);

  QColor c = color;
  c.setAlphaF(opacity);

  switch (shape_) {
  case BrushTipShape::Chisel: {
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawPath(tipShape(size));
    break;
  }
  case BrushTipShape::Stamp: {
    if (!tipImage_.isNull()) {
      QImage scaled =
          tipImage_.scaled(dim, dim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      p.setOpacity(opacity);
      p.drawImage(-scaled.width() / 2, -scaled.height() / 2, scaled);
    } else {
      // Fallback: star-like stamp
      p.setPen(Qt::NoPen);
      p.setBrush(c);
      QPainterPath star;
      qreal outer = size / 2.0;
      qreal inner = size / 5.0;
      for (int i = 0; i < 10; ++i) {
        qreal r = (i % 2 == 0) ? outer : inner;
        qreal a = M_PI / 5.0 * i - M_PI / 2.0;
        QPointF pt(r * qCos(a), r * qSin(a));
        if (i == 0)
          star.moveTo(pt);
        else
          star.lineTo(pt);
      }
      star.closeSubpath();
      p.drawPath(star);
    }
    break;
  }
  case BrushTipShape::Textured: {
    if (!tipImage_.isNull()) {
      QBrush texBrush(tipImage_);
      p.setPen(Qt::NoPen);
      p.setBrush(texBrush);
      p.setOpacity(opacity);
      p.drawEllipse(QPointF(0, 0), size / 2.0, size / 2.0);
    } else {
      // Fallback: noisy circle
      p.setPen(Qt::NoPen);
      QColor c1 = c;
      QColor c2 = c;
      c2.setAlphaF(opacity * 0.4);
      QRadialGradient grad(0, 0, size / 2.0);
      grad.setColorAt(0, c1);
      grad.setColorAt(0.6, c2);
      grad.setColorAt(1.0, Qt::transparent);
      p.setBrush(grad);
      p.drawEllipse(QPointF(0, 0), size / 2.0, size / 2.0);
    }
    break;
  }
  case BrushTipShape::Round:
  default: {
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(QPointF(0, 0), size / 2.0, size / 2.0);
    break;
  }
  }

  p.end();
  return img;
}

QString BrushTip::shapeName(BrushTipShape shape) {
  switch (shape) {
  case BrushTipShape::Round:
    return QStringLiteral("Round");
  case BrushTipShape::Chisel:
    return QStringLiteral("Chisel");
  case BrushTipShape::Stamp:
    return QStringLiteral("Stamp");
  case BrushTipShape::Textured:
    return QStringLiteral("Textured");
  }
  return QStringLiteral("Round");
}
