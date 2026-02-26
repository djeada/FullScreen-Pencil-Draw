/**
 * @file busy_spinner_overlay.cpp
 * @brief Implementation of the BusySpinnerOverlay widget.
 */
#include "busy_spinner_overlay.h"

#include <QPainter>

BusySpinnerOverlay::BusySpinnerOverlay(QWidget *parent) : QWidget(parent) {
  setAttribute(Qt::WA_TransparentForMouseEvents, false);
  setVisible(false);

  connect(&animationTimer_, &QTimer::timeout, this, [this]() {
    angle_ = (angle_ + 30) % 360;
    update();
  });
}

void BusySpinnerOverlay::setText(const QString &text) { text_ = text; }

void BusySpinnerOverlay::start(const QString &text) {
  if (!text.isNull())
    text_ = text;
  if (parentWidget())
    setGeometry(parentWidget()->rect());
  angle_ = 0;
  animationTimer_.start(50);
  raise();
  show();
  // Process one paint event so the overlay is visible immediately
  repaint();
}

void BusySpinnerOverlay::stop() {
  animationTimer_.stop();
  hide();
}

void BusySpinnerOverlay::paintEvent(QPaintEvent * /*event*/) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  // Semi-transparent background
  p.fillRect(rect(), QColor(0, 0, 0, 120));

  // Spinner geometry
  const int spinnerSize = 48;
  const int penWidth = 5;
  QRect spinnerRect(0, 0, spinnerSize, spinnerSize);
  spinnerRect.moveCenter(rect().center());

  // Draw arc
  QPen arcPen(QColor(59, 130, 246), penWidth, Qt::SolidLine, Qt::RoundCap);
  p.setPen(arcPen);
  p.drawArc(spinnerRect, angle_ * 16, 270 * 16);

  // Draw label below spinner
  if (!text_.isEmpty()) {
    QFont f = font();
    f.setPointSize(11);
    p.setFont(f);
    p.setPen(Qt::white);
    QRect textRect = rect();
    textRect.setTop(spinnerRect.bottom() + 16);
    p.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, text_);
  }
}
