/**
 * @file text_on_path_item.cpp
 * @brief Implementation of TextOnPathItem.
 */
#include "text_on_path_item.h"
#include <QFontMetricsF>
#include <QInputDialog>
#include <QPainter>
#include <QtMath>

TextOnPathItem::TextOnPathItem(QGraphicsItem *parent)
    : QGraphicsObject(parent), textColor_(Qt::black), font_("Arial", 14) {
  setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
}

TextOnPathItem::~TextOnPathItem() = default;

QRectF TextOnPathItem::boundingRect() const { return cachedBounds_; }

void TextOnPathItem::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem * /*option*/,
                           QWidget * /*widget*/) {
  if (text_.isEmpty() || path_.isEmpty())
    return;

  painter->setFont(font_);
  painter->setPen(textColor_);

  QFontMetricsF fm(font_);
  qreal totalLen = path_.length();
  qreal pos = 0.0;

  for (int i = 0; i < text_.size(); ++i) {
    QChar ch = text_.at(i);
    qreal charWidth = fm.horizontalAdvance(ch);

    qreal mid = pos + charWidth / 2.0;
    if (mid > totalLen)
      break;

    qreal pct = mid / totalLen;
    QPointF pt = path_.pointAtPercent(pct);
    qreal angle = path_.angleAtPercent(pct);

    painter->save();
    painter->translate(pt);
    painter->rotate(-angle);
    painter->drawText(QPointF(-charWidth / 2.0, fm.ascent() / 2.0),
                      QString(ch));
    painter->restore();

    pos += charWidth;
  }
}

void TextOnPathItem::setPath(const QPainterPath &path) {
  path_ = path;
  rebuildLayout();
}

void TextOnPathItem::setText(const QString &text) {
  if (text_ == text)
    return;
  text_ = text;
  rebuildLayout();
  emit textChanged();
}

void TextOnPathItem::setTextColor(const QColor &color) {
  textColor_ = color;
  update();
}

void TextOnPathItem::setFont(const QFont &font) {
  font_ = font;
  rebuildLayout();
}

void TextOnPathItem::mouseDoubleClickEvent(
    QGraphicsSceneMouseEvent * /*event*/) {
  bool ok = false;
  QString newText = QInputDialog::getText(
      nullptr, "Edit Text on Path", "Text:", QLineEdit::Normal, text_, &ok);
  if (ok && !newText.isEmpty()) {
    setText(newText);
  }
}

void TextOnPathItem::rebuildLayout() {
  prepareGeometryChange();
  if (path_.isEmpty()) {
    cachedBounds_ = QRectF();
    update();
    return;
  }

  QFontMetricsF fm(font_);
  qreal margin = fm.height();
  cachedBounds_ =
      path_.boundingRect().adjusted(-margin, -margin, margin, margin);
  update();
}
