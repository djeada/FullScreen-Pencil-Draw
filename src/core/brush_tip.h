/**
 * @file brush_tip.h
 * @brief Custom brush tip shapes for calligraphy, stamps, and textured
 * strokes.
 */
#ifndef BRUSH_TIP_H
#define BRUSH_TIP_H

#include <QImage>
#include <QPainterPath>
#include <QPointF>
#include <QString>

/**
 * @brief Defines the available brush tip shapes.
 */
enum class BrushTipShape {
  Round,   ///< Default circular tip
  Chisel,  ///< Angled flat tip for calligraphy
  Stamp,   ///< Stamps a custom image at each point
  Textured ///< Applies a texture pattern along the stroke
};

/**
 * @brief Describes a custom brush tip used for drawing strokes.
 *
 * A BrushTip encapsulates the shape and parameters of the mark left by the pen.
 * - Round: standard circular dot (default behaviour).
 * - Chisel: a narrow rectangle rotated by @c angle(), producing calligraphic
 *   thick/thin variation.
 * - Stamp: places a user-supplied image at regular intervals along the stroke.
 * - Textured: applies a repeating texture image as a brush pattern.
 */
class BrushTip {
public:
  BrushTip();
  ~BrushTip();

  /**
   * @brief Get the current tip shape
   */
  BrushTipShape shape() const { return shape_; }

  /**
   * @brief Set the tip shape
   */
  void setShape(BrushTipShape shape) { shape_ = shape; }

  /**
   * @brief Chisel angle in degrees (0 = horizontal)
   */
  qreal angle() const { return angle_; }

  /**
   * @brief Set the chisel angle
   */
  void setAngle(qreal angle) { angle_ = angle; }

  /**
   * @brief Spacing between stamp impressions (multiplier of brush size)
   */
  qreal stampSpacing() const { return stampSpacing_; }

  /**
   * @brief Set stamp spacing multiplier
   */
  void setStampSpacing(qreal spacing) { stampSpacing_ = spacing; }

  /**
   * @brief The image used for Stamp or Textured mode
   */
  const QImage &tipImage() const { return tipImage_; }

  /**
   * @brief Set the tip image (used by Stamp and Textured modes)
   */
  void setTipImage(const QImage &image) { tipImage_ = image; }

  /**
   * @brief Build a QPainterPath representing the tip at the origin.
   * @param size The brush diameter
   * @return A path centred on (0,0)
   */
  QPainterPath tipShape(qreal size) const;

  /**
   * @brief Render a single tip impression into a QImage.
   * @param size Brush diameter in pixels
   * @param color Fill colour
   * @param opacity 0.0 – 1.0
   * @return ARGB32-premultiplied image of the tip
   */
  QImage renderTip(qreal size, const QColor &color, qreal opacity = 1.0) const;

  /**
   * @brief Get a display name for a given shape
   */
  static QString shapeName(BrushTipShape shape);

private:
  BrushTipShape shape_ = BrushTipShape::Round;
  qreal angle_ = 45.0;
  qreal stampSpacing_ = 0.25;
  QImage tipImage_;
};

#endif // BRUSH_TIP_H
