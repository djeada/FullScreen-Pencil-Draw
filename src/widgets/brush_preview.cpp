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

  // Draw modern flat background with subtle gradient
  QLinearGradient bgGradient(0, 0, 0, height());
  bgGradient.setColorAt(0, QColor(26, 26, 30));
  bgGradient.setColorAt(1, QColor(22, 22, 26));
  painter.fillRect(rect(), bgGradient);

  // Draw subtle rounded border with glow
  painter.setPen(QPen(QColor(55, 55, 62), 1));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);

  // Calculate display size (scale down if brush is larger than preview area)
  int maxDisplaySize = PREVIEW_SIZE - 10;
  int displaySize = qMin(brushSize_, maxDisplaySize);
  displaySize = qMax(displaySize, MIN_DISPLAY_SIZE);

  // Calculate scale factor for visual feedback
  qreal scaleFactor = 1.0;
  if (brushSize_ > maxDisplaySize) {
    scaleFactor = static_cast<qreal>(maxDisplaySize) / brushSize_;
  }

  // Draw subtle crosshairs
  painter.setPen(QPen(QColor(60, 60, 68), 1, Qt::DotLine));
  int centerX = width() / 2;
  int centerY = height() / 2;
  painter.drawLine(centerX, 6, centerX, height() - 6);
  painter.drawLine(6, centerY, width() - 6, centerY);

  // Draw multiple glow layers for enhanced effect
  QColor fillColor = brushColor_;
  fillColor.setAlpha(180);

  // Outer glow (larger, more diffuse)
  QColor outerGlowColor = brushColor_;
  outerGlowColor.setAlpha(25);
  painter.setPen(Qt::NoPen);
  painter.setBrush(outerGlowColor);
  int outerGlowSize = displaySize + 12;
  int outerGlowX = centerX - outerGlowSize / 2;
  int outerGlowY = centerY - outerGlowSize / 2;
  painter.drawEllipse(outerGlowX, outerGlowY, outerGlowSize, outerGlowSize);

  // Inner glow
  QColor glowColor = brushColor_;
  glowColor.setAlpha(50);
  painter.setBrush(glowColor);
  int glowSize = displaySize + 6;
  int glowX = centerX - glowSize / 2;
  int glowY = centerY - glowSize / 2;
  painter.drawEllipse(glowX, glowY, glowSize, glowSize);

  // Main brush circle with gradient
  QRadialGradient circleGradient(centerX, centerY, displaySize / 2);
  circleGradient.setColorAt(0, brushColor_);
  circleGradient.setColorAt(0.7, fillColor);
  circleGradient.setColorAt(1, QColor(brushColor_.red(), brushColor_.green(), brushColor_.blue(), 100));
  
  painter.setPen(QPen(brushColor_.lighter(120), 2));
  painter.setBrush(circleGradient);
  int x = centerX - displaySize / 2;
  int y = centerY - displaySize / 2;
  painter.drawEllipse(x, y, displaySize, displaySize);

  // Draw actual size text if scaled
  if (brushSize_ > maxDisplaySize) {
    painter.setPen(QColor(160, 160, 168));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setWeight(QFont::Medium);
    painter.setFont(font);
    QString sizeText = QString("%1px").arg(brushSize_);
    painter.drawText(rect().adjusted(0, 0, 0, -4), Qt::AlignBottom | Qt::AlignHCenter, sizeText);
  }
}
