/**
 * @file brush_preview.cpp
 * @brief Brush size preview widget implementation.
 */
#include "brush_preview.h"
#include <QPainter>
#include <QPainterPath>

BrushPreview::BrushPreview(QWidget *parent)
    : QWidget(parent), brushSize_(3), brushColor_(Qt::white) {
  setFixedSize(PREVIEW_SIZE, PREVIEW_SIZE);
  setToolTip("Brush size preview");
}

BrushPreview::~BrushPreview() = default;

void BrushPreview::setBrushSize(int size) {
  if (brushSize_ != size) {
    brushSize_ = size;
    update();
  }
}

void BrushPreview::setBrushColor(const QColor &color) {
  if (brushColor_ != color) {
    brushColor_ = color;
    update();
  }
}

QSize BrushPreview::sizeHint() const {
  return QSize(PREVIEW_SIZE, PREVIEW_SIZE);
}

QSize BrushPreview::minimumSizeHint() const {
  return QSize(PREVIEW_SIZE, PREVIEW_SIZE);
}

void BrushPreview::paintEvent(QPaintEvent * /*event*/) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Draw background
  painter.fillRect(rect(), QColor(30, 30, 30));

  // Draw border
  painter.setPen(QPen(QColor(60, 60, 60), 1));
  painter.drawRect(rect().adjusted(0, 0, -1, -1));

  // Calculate display size (scale down if brush is larger than preview area)
  int maxDisplaySize = PREVIEW_SIZE - 8;
  int displaySize = qMin(brushSize_, maxDisplaySize);
  displaySize = qMax(displaySize, MIN_DISPLAY_SIZE);

  // Calculate scale factor for visual feedback
  qreal scaleFactor = 1.0;
  if (brushSize_ > maxDisplaySize) {
    scaleFactor = static_cast<qreal>(maxDisplaySize) / brushSize_;
  }

  // Draw crosshairs
  painter.setPen(QPen(QColor(80, 80, 80), 1));
  int centerX = width() / 2;
  int centerY = height() / 2;
  painter.drawLine(centerX, 0, centerX, height());
  painter.drawLine(0, centerY, width(), centerY);

  // Draw brush circle
  QColor fillColor = brushColor_;
  fillColor.setAlpha(180);

  painter.setPen(QPen(brushColor_, 1));
  painter.setBrush(fillColor);

  int x = centerX - displaySize / 2;
  int y = centerY - displaySize / 2;
  painter.drawEllipse(x, y, displaySize, displaySize);

  // Draw actual size text if scaled
  if (brushSize_ > maxDisplaySize) {
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    QString sizeText = QString("%1px").arg(brushSize_);
    painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, sizeText);
  }
}
