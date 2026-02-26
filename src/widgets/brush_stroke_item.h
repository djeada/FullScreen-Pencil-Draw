/**
 * @file brush_stroke_item.h
 * @brief A QGraphicsItem that renders a stroke using a custom BrushTip.
 */
#ifndef BRUSH_STROKE_ITEM_H
#define BRUSH_STROKE_ITEM_H

#include "../core/brush_tip.h"
#include <QColor>
#include <QGraphicsItem>
#include <QImage>
#include <QPainterPath>
#include <QVector>

/**
 * @brief A scene item that paints a stroke by stamping a BrushTip along a
 * path.
 *
 * For each recorded point the brush tip image is composited onto an internal
 * raster buffer. The item is displayed as a pixmap inside its bounding rect.
 */
class BrushStrokeItem : public QGraphicsItem {
public:
  BrushStrokeItem(const BrushTip &tip, qreal size, const QColor &color,
                  qreal opacity = 1.0, QGraphicsItem *parent = nullptr);
  ~BrushStrokeItem() override;

  /**
   * @brief Append a new point to the stroke.
   */
  void addPoint(const QPointF &scenePoint);

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  void rebuildImage();

  BrushTip tip_;
  qreal brushSize_;
  QColor color_;
  qreal opacity_;
  QVector<QPointF> points_;
  QImage tipImage_;
  QImage buffer_;
  QRectF bounds_;
  static constexpr qreal MARGIN = 2.0;
};

#endif // BRUSH_STROKE_ITEM_H
