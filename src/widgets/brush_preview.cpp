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

  // Draw modern flat background
  painter.fillRect(rect(), QColor(30, 30, 34));

  // Draw subtle rounded border
  painter.setPen(QPen(QColor(58, 58, 64), 1));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6, 6);

  // Calculate display size (scale down if brush is larger than preview area)
  int maxDisplaySize = PREVIEW_SIZE - 8;
  int displaySize = qMin(brushSize_, maxDisplaySize);
  displaySize = qMax(displaySize, MIN_DISPLAY_SIZE);

  // Calculate scale factor for visual feedback
  qreal scaleFactor = 1.0;
  if (brushSize_ > maxDisplaySize) {
    scaleFactor = static_cast<qreal>(maxDisplaySize) / brushSize_;
  }

  // Draw subtle crosshairs
  painter.setPen(QPen(QColor(70, 70, 76), 1));
  int centerX = width() / 2;
  int centerY = height() / 2;
  painter.drawLine(centerX, 4, centerX, height() - 4);
  painter.drawLine(4, centerY, width() - 4, centerY);

  // Draw brush circle with glow effect
  QColor fillColor = brushColor_;
  fillColor.setAlpha(160);

  // Outer glow
  QColor glowColor = brushColor_;
  glowColor.setAlpha(40);
  painter.setPen(Qt::NoPen);
  painter.setBrush(glowColor);
  int glowSize = displaySize + 6;
  int glowX = centerX - glowSize / 2;
  int glowY = centerY - glowSize / 2;
  painter.drawEllipse(glowX, glowY, glowSize, glowSize);

  // Main brush circle
  painter.setPen(QPen(brushColor_, 2));
  painter.setBrush(fillColor);
  int x = centerX - displaySize / 2;
  int y = centerY - displaySize / 2;
  painter.drawEllipse(x, y, displaySize, displaySize);

  // Draw actual size text if scaled
  if (brushSize_ > maxDisplaySize) {
    painter.setPen(QColor(160, 160, 165));
    QFont font = painter.font();
    font.setPointSize(8);
    font.setWeight(QFont::Medium);
    painter.setFont(font);
    QString sizeText = QString("%1px").arg(brushSize_);
    painter.drawText(rect(), Qt::AlignBottom | Qt::AlignHCenter, sizeText);
  }
}
