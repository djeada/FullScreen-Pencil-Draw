// image_size_dialog.h
#ifndef IMAGE_SIZE_DIALOG_H
#define IMAGE_SIZE_DIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

class ImageSizeDialog : public QDialog {
  Q_OBJECT

public:
  explicit ImageSizeDialog(int originalWidth, int originalHeight,
                           QWidget *parent = nullptr);

  int getWidth() const;
  int getHeight() const;

private slots:
  void onWidthChanged(int value);
  void onHeightChanged(int value);
  void onMaintainAspectChanged(int state);

private:
  QSpinBox *widthSpinBox;
  QSpinBox *heightSpinBox;
  QCheckBox *maintainAspectCheckBox;
  QLabel *originalSizeLabel;

  int originalWidth;
  int originalHeight;
  double aspectRatio;
  bool updatingValues;
};

#endif // IMAGE_SIZE_DIALOG_H
