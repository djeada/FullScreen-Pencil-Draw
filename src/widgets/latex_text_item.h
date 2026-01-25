/**
 * @file latex_text_item.h
 * @brief Custom graphics item for LaTeX-enabled text with inline editing.
 *
 * This class provides an editable text item that can render LaTeX
 * expressions enclosed by $...$ delimiters. Features inline text editing
 * with a visible text rectangle and real-time LaTeX preview.
 */
#ifndef LATEX_TEXT_ITEM_H
#define LATEX_TEXT_ITEM_H

#include <QFont>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTextEdit>

class QFocusEvent;

/**
 * @brief Inline text editor for LaTeX input with real-time preview hint.
 */
class LatexTextEdit : public QTextEdit {
  Q_OBJECT

public:
  explicit LatexTextEdit(QWidget *parent = nullptr);

signals:
  void editingFinished();
  void editingCancelled();

protected:
  void focusOutEvent(QFocusEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
};

/**
 * @brief A graphics item that supports inline text editing with LaTeX rendering.
 *
 * When the text contains expressions enclosed by $...$, they are rendered
 * as mathematical formulas using Unicode symbols. The item supports:
 * - Inline text editing with visible text rectangle
 * - LaTeX rendering when focus is lost (clicking outside)
 * - Double-clicking to re-edit existing text
 * - Real-time LaTeX syntax hints
 * - Selection and movement like other graphics items
 */
class LatexTextItem : public QGraphicsObject {
  Q_OBJECT

public:
  /**
   * @brief Construct a new LatexTextItem.
   * @param parent Parent graphics item (optional)
   */
  explicit LatexTextItem(QGraphicsItem *parent = nullptr);
  ~LatexTextItem() override;

  /**
   * @brief Get the bounding rectangle of the item.
   * @return The bounding rectangle
   */
  QRectF boundingRect() const override;

  /**
   * @brief Paint the item.
   * @param painter The painter to use
   * @param option Style options
   * @param widget The widget being painted on
   */
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  /**
   * @brief Get the raw text content.
   * @return The text content
   */
  QString text() const { return text_; }

  /**
   * @brief Set the text content.
   * @param text The new text content
   */
  void setText(const QString &text);

  /**
   * @brief Get the text color.
   * @return The text color
   */
  QColor textColor() const { return textColor_; }

  /**
   * @brief Set the text color.
   * @param color The new text color
   */
  void setTextColor(const QColor &color);

  /**
   * @brief Get the font.
   * @return The font
   */
  QFont font() const { return font_; }

  /**
   * @brief Set the font.
   * @param font The new font
   */
  void setFont(const QFont &font);

  /**
   * @brief Start inline editing mode with text rectangle.
   */
  void startEditing();

  /**
   * @brief Finish editing and render the LaTeX content.
   */
  void finishEditing();

  /**
   * @brief Check if currently in editing mode.
   * @return true if editing
   */
  bool isEditing() const { return isEditing_; }

  /**
   * @brief Check if the text contains LaTeX expressions.
   * @return true if LaTeX is present
   */
  bool hasLatex() const;

signals:
  /**
   * @brief Emitted when editing is finished.
   */
  void editingFinished();

  /**
   * @brief Emitted when the text content changes.
   */
  void textChanged();

protected:
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private slots:
  void onEditingFinished();
  void onEditingCancelled();

private:
  /**
   * @brief Render the text content (including LaTeX if present).
   */
  void renderContent();

  /**
   * @brief Parse LaTeX expressions and convert to rendered format.
   * @param text The text to parse
   * @return The rendered pixmap
   */
  QPixmap renderLatex(const QString &text);

  /**
   * @brief Convert a LaTeX expression to Unicode/HTML representation.
   * @param latex The LaTeX expression (without $ delimiters)
   * @return HTML-formatted text
   */
  QString latexToHtml(const QString &latex);

  /**
   * @brief Convert basic LaTeX commands to Unicode.
   * @param latex The LaTeX command
   * @return Unicode string
   */
  QString latexCommandToUnicode(const QString &latex);

  QString text_;
  QColor textColor_;
  QFont font_;
  QPixmap renderedContent_;
  QRectF contentRect_;
  bool isEditing_;

  // Inline editing widgets
  QGraphicsProxyWidget *proxyWidget_;
  LatexTextEdit *textEdit_;

  // Layout constants for refined visual appearance
  static constexpr int MIN_WIDTH = 120;
  static constexpr int MIN_HEIGHT = 28;
  static constexpr int PADDING = 10;
  static constexpr int EDIT_MIN_WIDTH = 280;
  static constexpr int EDIT_MIN_HEIGHT = 70;
};

#endif // LATEX_TEXT_ITEM_H
