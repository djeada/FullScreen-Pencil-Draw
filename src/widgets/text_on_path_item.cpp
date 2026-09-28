/**
 * @file text_on_path_item.cpp
 * @brief Implementation of TextOnPathItem.
 */
#include "text_on_path_item.h"
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QInputDialog>
#include <QPainter>
#include <QPointer>
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
  const QFontMetricsF fm(font_);
  for (const GlyphPlacement &g : glyphPlacements()) {
    painter->save();
    painter->translate(g.position);
    painter->rotate(-g.angle);
    painter->drawText(QPointF(-g.width / 2.0, fm.ascent() / 2.0),
                      QString(g.character));
    painter->restore();
  }
}

QList<TextOnPathItem::GlyphPlacement> TextOnPathItem::glyphPlacements() const {
  QList<GlyphPlacement> glyphs;
  if (text_.isEmpty() || path_.isEmpty())
    return glyphs;
  const QFontMetricsF fm(font_);
  const qreal totalLen = path_.length();
  qreal pos = 0.0;
  for (const QChar ch : text_) {
    const qreal charWidth = fm.horizontalAdvance(ch);
    const qreal mid = pos + charWidth / 2.0;
    if (mid > totalLen)
      break; // the rest of the text does not fit on the path
    // pointAtPercent() takes a curve *parameter*, not a length fraction; on
    // curved paths the two differ a lot and letters piled up or spread
    // out. Convert the arc length explicitly.
    const qreal pct = path_.percentAtLength(mid);
    glyphs.append(
        {ch, path_.pointAtPercent(pct), path_.angleAtPercent(pct), charWidth});
    pos += charWidth;
  }
  return glyphs;
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
  // The dialog runs a nested event loop, during which this item can be
  // deleted (undo, layer removal, document close) – guard `this` and parent
  // the dialog to the view so it centres and stays modal to the window.
  QPointer<TextOnPathItem> guard(this);
  QWidget *dialogParent = nullptr;
  if (scene() && !scene()->views().isEmpty()) {
    dialogParent = scene()->views().constFirst();
  }

  bool ok = false;
  QString newText =
      QInputDialog::getText(dialogParent, "Edit Text on Path",
                            "Text:", QLineEdit::Normal, text_, &ok);
  if (!guard) {
    return;
  }
  if (ok && !newText.isEmpty() && newText != text_) {
    const QString oldText = text_;
    setText(newText);
    emit textEdited(oldText, newText);
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
