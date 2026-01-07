/**
 * @file brush_preview.h
 * @brief Brush size preview widget.
 */
#ifndef BRUSH_PREVIEW_H
#define BRUSH_PREVIEW_H

#include <QColor>
#include <QWidget>

/**
 * @brief A widget that displays a visual preview of the current brush size.
 *
 * Shows a circle representing the brush size with the current color.
 * This provides intuitive feedback about the brush dimensions.
 */
class BrushPreview : public QWidget {
  Q_OBJECT

public:
  explicit BrushPreview(QWidget *parent = nullptr);
  ~BrushPreview() override;

  /**
   * @brief Set the brush size to display
   * @param size The brush diameter in pixels
   */
  void setBrushSize(int size);

  /**
   * @brief Set the brush color
   * @param color The color to use for the preview
   */
  void setBrushColor(const QColor &color);

  /**
   * @brief Get the current brush size
   * @return The brush size
   */
  int brushSize() const { return brushSize_; }

  /**
   * @brief Get the current brush color
   * @return The brush color
   */
  QColor brushColor() const { return brushColor_; }

protected:
  void paintEvent(QPaintEvent *event) override;
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

private:
  int brushSize_;
  QColor brushColor_;
  static constexpr int PREVIEW_SIZE = 60;
  static constexpr int MIN_DISPLAY_SIZE = 4;
};

#endif // BRUSH_PREVIEW_H
