/**
 * @file scale_dialog.h
 * @brief Dialog for specifying scale factors.
 *
 * Provides a dialog that lets users enter scale percentages for X and Y axes,
 * with an option to maintain aspect ratio (uniform scaling).
 */
#ifndef SCALE_DIALOG_H
#define SCALE_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QDoubleSpinBox>

class ScaleDialog : public QDialog {
  Q_OBJECT

public:
  explicit ScaleDialog(QWidget *parent = nullptr);

  double scaleX() const;
  double scaleY() const;

private slots:
  void onScaleXChanged(double value);
  void onScaleYChanged(double value);

private:
  QDoubleSpinBox *scaleXSpinBox_;
  QDoubleSpinBox *scaleYSpinBox_;
  QCheckBox *uniformCheckBox_;
  bool updatingValues_;
};

#endif // SCALE_DIALOG_H
