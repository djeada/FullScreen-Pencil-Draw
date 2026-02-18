// resize_canvas_dialog.h
#ifndef RESIZE_CANVAS_DIALOG_H
#define RESIZE_CANVAS_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

class ResizeCanvasDialog : public QDialog {
  Q_OBJECT

public:
  enum Anchor {
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    Center,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
  };

  explicit ResizeCanvasDialog(int currentWidth, int currentHeight,
                              QWidget *parent = nullptr);

  int getWidth() const;
  int getHeight() const;
  Anchor getAnchor() const;

private slots:
  void onAnchorClicked();

private:
  void updateAnchorButtons();

  QSpinBox *widthSpinBox_;
  QSpinBox *heightSpinBox_;
  QLabel *currentSizeLabel_;
  QPushButton *anchorButtons_[9];
  Anchor selectedAnchor_;
};

#endif // RESIZE_CANVAS_DIALOG_H
