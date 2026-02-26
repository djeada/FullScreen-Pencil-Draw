/**
 * @file busy_spinner_overlay.h
 * @brief A semi-transparent overlay with an animated spinner indicator.
 *
 * Shows a spinning arc animation and an optional label over a parent widget
 * to indicate that a long-running operation is in progress.
 */
#ifndef BUSY_SPINNER_OVERLAY_H
#define BUSY_SPINNER_OVERLAY_H

#include <QLabel>
#include <QTimer>
#include <QWidget>

class BusySpinnerOverlay : public QWidget {
  Q_OBJECT

public:
  explicit BusySpinnerOverlay(QWidget *parent = nullptr);

  void setText(const QString &text);

  /// Show the overlay and start the animation.
  void start(const QString &text = QString());

  /// Stop the animation and hide the overlay.
  void stop();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QTimer animationTimer_;
  int angle_ = 0;
  QString text_;
};

#endif // BUSY_SPINNER_OVERLAY_H
