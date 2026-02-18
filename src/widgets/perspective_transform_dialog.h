/**
 * @file perspective_transform_dialog.h
 * @brief Dialog for specifying perspective transform parameters.
 *
 * Provides a dialog that lets users drag corner handles to define a
 * perspective transformation, useful for mockups and diagram annotations.
 */
#ifndef PERSPECTIVE_TRANSFORM_DIALOG_H
#define PERSPECTIVE_TRANSFORM_DIALOG_H

#include <QDialog>
#include <QTransform>

class QDoubleSpinBox;
class QLabel;

/**
 * @brief Interactive preview widget showing a quad with draggable corners.
 *
 * The preview displays the original rectangle and lets the user drag each
 * corner to define the target perspective quad.
 */
class PerspectivePreviewWidget : public QWidget {
  Q_OBJECT

public:
  explicit PerspectivePreviewWidget(QWidget *parent = nullptr);

  /**
   * @brief Get the corner offsets relative to the unit square.
   *
   * Each QPointF represents the fractional offset (0..1) from the
   * original corner position.  For example, topLeft() == (0,0) means
   * no change; (0.1, 0.05) means the top-left corner has been moved
   * 10 % right and 5 % down.
   */
  QPointF topLeft() const { return corners_[0]; }
  QPointF topRight() const { return corners_[1]; }
  QPointF bottomRight() const { return corners_[2]; }
  QPointF bottomLeft() const { return corners_[3]; }

  void setCorner(int index, const QPointF &pos);
  void reset();

signals:
  void cornersChanged();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  static constexpr int MARGIN = 20;
  static constexpr int HANDLE_RADIUS = 6;

  QPointF corners_[4]; // normalised 0..1
  int dragIndex_;

  QRectF previewRect() const;
  QPointF cornerToWidget(int index) const;
  int cornerIndexAt(const QPointF &widgetPos) const;
};

/**
 * @brief Dialog for applying a perspective transform to selected items.
 *
 * Users adjust four corner points via an interactive preview or spin-box
 * fields.  The resulting QTransform can be retrieved after the dialog is
 * accepted.
 */
class PerspectiveTransformDialog : public QDialog {
  Q_OBJECT

public:
  explicit PerspectiveTransformDialog(QWidget *parent = nullptr);

  /**
   * @brief Compute the perspective QTransform for a given bounding rect.
   *
   * The transform maps the four corners of @p rect to the adjusted
   * positions specified by the user.
   */
  QTransform perspectiveTransform(const QRectF &rect) const;

private slots:
  void onCornersChanged();
  void onSpinBoxChanged();
  void onReset();

private:
  PerspectivePreviewWidget *preview_;

  // Spin-boxes for each corner offset (dx, dy) as percentage
  QDoubleSpinBox *tlX_;
  QDoubleSpinBox *tlY_;
  QDoubleSpinBox *trX_;
  QDoubleSpinBox *trY_;
  QDoubleSpinBox *blX_;
  QDoubleSpinBox *blY_;
  QDoubleSpinBox *brX_;
  QDoubleSpinBox *brY_;

  bool updatingFromPreview_;
  bool updatingFromSpinBox_;

  QDoubleSpinBox *createSpinBox();
  void syncSpinBoxesFromPreview();
  void syncPreviewFromSpinBoxes();
};

#endif // PERSPECTIVE_TRANSFORM_DIALOG_H
