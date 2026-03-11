/**
 * @file brush_preview.cpp
 * @brief Brush size preview widget implementation.
 */
#include "brush_preview.h"
#include "../core/theme_manager.h"
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

  const bool darkTheme = ThemeManager::instance().isDarkTheme();

  // Draw a theme-aware preview well that matches the panel cards.
  QLinearGradient bgGradient(0, 0, 0, height());
  bgGradient.setColorAt(0, darkTheme ? QColor("#17212b") : QColor("#fff9f1"));
  bgGradient.setColorAt(1, darkTheme ? QColor("#10161d") : QColor("#f0e3d3"));
  painter.fillRect(rect(), bgGradient);

  painter.setPen(QPen(darkTheme ? QColor(255, 244, 230, 26)
                                : QColor(117, 59, 19, 28),
                    1));
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

  painter.setPen(QPen(darkTheme ? QColor("#314150") : QColor("#d4b89e"), 1,
                      Qt::DotLine));
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
  circleGradient.setColorAt(1, QColor(brushColor_.red(), brushColor_.green(),
                                      brushColor_.blue(), 100));

  painter.setPen(QPen(brushColor_.lighter(120), 2));
  painter.setBrush(circleGradient);
  int x = centerX - displaySize / 2;
  int y = centerY - displaySize / 2;
  painter.drawEllipse(x, y, displaySize, displaySize);

  // Draw actual size text if scaled
  if (brushSize_ > maxDisplaySize) {
    painter.setPen(darkTheme ? QColor("#d0c4b7") : QColor("#7a6858"));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setWeight(QFont::Medium);
    painter.setFont(font);
    QString sizeText = QString("%1px").arg(brushSize_);
    painter.drawText(rect().adjusted(0, 0, 0, -4),
                     Qt::AlignBottom | Qt::AlignHCenter, sizeText);
  }
}
