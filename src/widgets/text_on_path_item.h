/**
 * @file text_on_path_item.h
 * @brief Graphics item that renders text along a QPainterPath.
 */
#ifndef TEXT_ON_PATH_ITEM_H
#define TEXT_ON_PATH_ITEM_H

#include <QFont>
#include <QGraphicsObject>
#include <QPainterPath>

/**
 * @brief A graphics item that draws text along an arbitrary path.
 *
 * TextOnPathItem takes a QPainterPath and a text string, then renders
 * each character positioned and rotated to follow the path. The item
 * supports editing via a simple dialog triggered by double-click.
 */
class TextOnPathItem : public QGraphicsObject {
  Q_OBJECT

public:
  explicit TextOnPathItem(QGraphicsItem *parent = nullptr);
  ~TextOnPathItem() override;

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  /** @brief Get the path along which text is drawn. */
  QPainterPath path() const { return path_; }

  /** @brief Set the path along which text is drawn. */
  void setPath(const QPainterPath &path);

  /** @brief Get the display text. */
  QString text() const { return text_; }

  /** @brief Set the display text. */
  void setText(const QString &text);

  /** @brief Get the text color. */
  QColor textColor() const { return textColor_; }

  /** @brief Set the text color. */
  void setTextColor(const QColor &color);

  /** @brief Get the font. */
  QFont font() const { return font_; }

  /** @brief Set the font. */
  void setFont(const QFont &font);

signals:
  void textChanged();

protected:
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
  void rebuildLayout();

  QPainterPath path_;
  QString text_;
  QColor textColor_;
  QFont font_;
  QRectF cachedBounds_;
};

#endif // TEXT_ON_PATH_ITEM_H
