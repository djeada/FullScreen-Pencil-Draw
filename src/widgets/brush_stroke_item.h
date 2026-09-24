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

  // Accessors used by project serialization and the Pixel Eraser.
  const BrushTip &tip() const { return tip_; }
  qreal brushSize() const { return brushSize_; }
  QColor color() const { return color_; }
  qreal strokeOpacity() const { return opacity_; }
  const QVector<QPointF> &points() const { return points_; }
  /// Rendered stroke pixels; imageRect() is their item-local placement.
  const QImage &image() const { return buffer_; }
  QRectF imageRect() const { return bounds_; }

  /**
   * @brief Restore a saved stroke exactly (pixels and parameters) without
   *        re-stamping, so reopening a file reproduces what was saved.
   */
  void restore(const QVector<QPointF> &points, const QImage &image,
               const QRectF &imageRect);
  /// Replace the rendered pixels (same placement), e.g. after pixel erasing.
  void setImage(const QImage &image);

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  qreal stampRadius() const;
  void initBuffer(const QPointF &p);
  void expandBuffer(const QPointF &p);
  void allocateBuffer(const QRectF &desired);
  void stampPoint(const QPointF &p);

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
