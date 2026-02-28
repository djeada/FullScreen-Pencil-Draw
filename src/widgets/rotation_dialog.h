/**
 * @file rotation_dialog.h
 * @brief Dialog for specifying rotation angles.
 *
 * Provides a dialog that lets users enter a rotation angle in degrees,
 * with preset buttons for common angles (90, 180, 270).
 */
#ifndef ROTATION_DIALOG_H
#define ROTATION_DIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>

class RotationDialog : public QDialog {
  Q_OBJECT

public:
  explicit RotationDialog(QWidget *parent = nullptr);

  /**
   * @brief Get the rotation angle in degrees (positive = counter-clockwise)
   */
  double angle() const;

private:
  QDoubleSpinBox *angleSpinBox_;
};

#endif // ROTATION_DIALOG_H
