/**
 * @file mermaid_text_item.h
 * @brief Custom graphics item for Mermaid diagrams with inline editing.
 *
 * This class provides an editable text item that can render Mermaid
 * diagram code. Features inline text editing with a visible text
 * rectangle and real-time Mermaid preview.
 */
#ifndef MERMAID_TEXT_ITEM_H
#define MERMAID_TEXT_ITEM_H

#include <QFont>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QTextEdit>

class QFocusEvent;

/**
 * @brief Inline text editor for Mermaid input.
 */
class MermaidTextEdit : public QTextEdit {
  Q_OBJECT

public:
  explicit MermaidTextEdit(QWidget *parent = nullptr);

signals:
  void editingFinished();
  void editingCancelled();

protected:
  void focusOutEvent(QFocusEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
};

/**
 * @brief A graphics item that supports inline text editing with Mermaid
 * rendering.
 *
 * The item supports:
 * - Inline text editing with visible text rectangle
 * - Mermaid diagram rendering when focus is lost (clicking outside)
 * - Double-clicking to re-edit existing code
 * - Selection and movement like other graphics items
 */
class MermaidTextItem : public QGraphicsObject {
  Q_OBJECT

public:
  /**
   * @brief Construct a new MermaidTextItem.
   * @param parent Parent graphics item (optional)
   */
  explicit MermaidTextItem(QGraphicsItem *parent = nullptr);
  ~MermaidTextItem() override;

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
   * @brief Get the raw Mermaid code content.
   * @return The Mermaid code
   */
  QString mermaidCode() const { return mermaidCode_; }

  /**
   * @brief Set the Mermaid code content.
   * @param code The new Mermaid code
   */
  void setMermaidCode(const QString &code);

  /**
   * @brief Get the theme.
   * @return The theme name
   */
  QString theme() const { return theme_; }

  /**
   * @brief Set the theme.
   * @param theme The theme name (default, dark, forest, neutral)
   */
  void setTheme(const QString &theme);

  /**
   * @brief Start inline editing mode with text rectangle.
   */
  void startEditing();

  /**
   * @brief Finish editing and render the Mermaid content.
   */
  void finishEditing();

  /**
   * @brief Check if currently in editing mode.
   * @return true if editing
   */
  bool isEditing() const { return isEditing_; }

signals:
  /**
   * @brief Emitted when editing is finished.
   */
  void editingFinished();

  /**
   * @brief Emitted when the Mermaid code changes.
   */
  void codeChanged();

protected:
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
  QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private slots:
  void onEditingFinished();
  void onEditingCancelled();
#ifdef HAVE_QT_WEBENGINE
  void onMermaidRenderComplete(quintptr requestId, const QPixmap &pixmap,
                                bool success);
#endif

private:
  /**
   * @brief Render the Mermaid content.
   */
  void renderContent();

  /**
   * @brief Create a placeholder pixmap when rendering is not available.
   * @return Placeholder pixmap
   */
  QPixmap createPlaceholder() const;

  QString mermaidCode_;
  QString theme_;
  QPixmap renderedContent_;
  QRectF contentRect_;
  bool isEditing_;

  // Inline editing widgets
  QGraphicsProxyWidget *proxyWidget_;
  MermaidTextEdit *textEdit_;

#ifdef HAVE_QT_WEBENGINE
  // Mermaid async rendering
  quintptr pendingRenderId_;
  bool mermaidConnected_;
#endif

  // Layout constants
  static constexpr int MIN_WIDTH = 200;
  static constexpr int MIN_HEIGHT = 100;
  static constexpr int PADDING = 16;
  static constexpr int EDIT_MIN_WIDTH = 400;
  static constexpr int EDIT_MIN_HEIGHT = 200;
};

#endif // MERMAID_TEXT_ITEM_H
