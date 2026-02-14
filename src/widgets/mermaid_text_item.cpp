/**
 * @file mermaid_text_item.cpp
 * @brief Implementation of Mermaid diagram graphics item with inline editing.
 */
#include "mermaid_text_item.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDateTime>
#include <QFocusEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextDocument>
#include <atomic>

#ifdef HAVE_QT_WEBENGINE
#include "../core/mermaid_renderer.h"
#endif

// ============================================================================
// MermaidTextEdit Implementation
// ============================================================================

MermaidTextEdit::MermaidTextEdit(QWidget *parent) : QTextEdit(parent) {
  setAcceptRichText(false);
  setLineWrapMode(QTextEdit::NoWrap);
  setFont(QFont("Monospace", 10));
  setPlaceholderText(
      "Enter Mermaid diagram code...\nExample:\ngraph TD\n    A[Start] --> "
      "B{Decision}\n    B -->|Yes| C[OK]\n    B -->|No| D[End]");
}

void MermaidTextEdit::focusOutEvent(QFocusEvent *event) {
  QTextEdit::focusOutEvent(event);
  // Only emit if focus went somewhere other than this widget's children
  if (event->reason() != Qt::PopupFocusReason) {
    emit editingFinished();
  }
}

void MermaidTextEdit::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    emit editingCancelled();
    return;
  }
  // Ctrl+Enter to finish editing
  if (event->key() == Qt::Key_Return &&
      (event->modifiers() & Qt::ControlModifier)) {
    emit editingFinished();
    return;
  }
  QTextEdit::keyPressEvent(event);
}

// ============================================================================
// MermaidTextItem Implementation
// ============================================================================

MermaidTextItem::MermaidTextItem(QGraphicsItem *parent)
    : QGraphicsObject(parent), theme_("default"), isEditing_(false),
      proxyWidget_(nullptr), textEdit_(nullptr)
#ifdef HAVE_QT_WEBENGINE
      ,
      pendingRenderId_(0), mermaidConnected_(false)
#endif
{
  setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);

  // Initialize with default size
  contentRect_ = QRectF(0, 0, MIN_WIDTH, MIN_HEIGHT);
}

MermaidTextItem::~MermaidTextItem() {
  if (textEdit_) {
    textEdit_->deleteLater();
    textEdit_ = nullptr;
  }
}

QRectF MermaidTextItem::boundingRect() const {
  if (isEditing_ && proxyWidget_) {
    return proxyWidget_->boundingRect();
  }
  return contentRect_;
}

void MermaidTextItem::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *option,
                            QWidget * /*widget*/) {
  if (isEditing_) {
    // The proxy widget handles painting during editing
    return;
  }

  // Draw the rendered content
  if (!renderedContent_.isNull()) {
    painter->drawPixmap(contentRect_.topLeft(), renderedContent_);
  } else {
    // Draw placeholder when no content
    painter->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    painter->setBrush(QColor(240, 240, 240));
    painter->drawRect(contentRect_);

    painter->setPen(Qt::darkGray);
    QFont font("Arial", 12);
    painter->setFont(font);
    painter->drawText(contentRect_, Qt::AlignCenter,
                      "Mermaid Diagram\n(double-click to edit)");
  }

  // Draw selection highlight
  if (option->state & QStyle::State_Selected) {
    painter->setPen(QPen(Qt::blue, 2, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(contentRect_.adjusted(-2, -2, 2, 2));
  }
}

void MermaidTextItem::setMermaidCode(const QString &code) {
  if (mermaidCode_ != code) {
    mermaidCode_ = code;
    renderContent();
    emit codeChanged();
  }
}

void MermaidTextItem::setTheme(const QString &theme) {
  if (theme_ != theme) {
    theme_ = theme;
    if (!mermaidCode_.isEmpty()) {
      renderContent();
    }
  }
}

void MermaidTextItem::startEditing() {
  if (isEditing_) {
    return;
  }

  isEditing_ = true;
  prepareGeometryChange();

  // Create the text editor if needed
  if (!textEdit_) {
    textEdit_ = new MermaidTextEdit();
    connect(textEdit_, &MermaidTextEdit::editingFinished, this,
            &MermaidTextItem::onEditingFinished);
    connect(textEdit_, &MermaidTextEdit::editingCancelled, this,
            &MermaidTextItem::onEditingCancelled);
  }

  // Set up the editor
  textEdit_->setPlainText(mermaidCode_);
  textEdit_->setMinimumSize(EDIT_MIN_WIDTH, EDIT_MIN_HEIGHT);
  textEdit_->resize(
      qMax(EDIT_MIN_WIDTH, static_cast<int>(contentRect_.width())),
      qMax(EDIT_MIN_HEIGHT, static_cast<int>(contentRect_.height())));

  // Create proxy widget if needed
  if (!proxyWidget_) {
    proxyWidget_ = new QGraphicsProxyWidget(this);
  }
  proxyWidget_->setWidget(textEdit_);
  proxyWidget_->setPos(0, 0);
  proxyWidget_->show();

  // Focus the editor
  textEdit_->setFocus();
  textEdit_->selectAll();

  update();
}

void MermaidTextItem::finishEditing() {
  if (!isEditing_) {
    return;
  }

  isEditing_ = false;
  prepareGeometryChange();

  // Get the text from the editor
  if (textEdit_) {
    mermaidCode_ = textEdit_->toPlainText();
  }

  // Hide the proxy widget
  if (proxyWidget_) {
    proxyWidget_->hide();
  }

  // Render the content
  renderContent();

  emit editingFinished();
  update();
}

void MermaidTextItem::onEditingFinished() { finishEditing(); }

void MermaidTextItem::onEditingCancelled() {
  // Cancel editing without saving changes
  isEditing_ = false;
  prepareGeometryChange();

  if (proxyWidget_) {
    proxyWidget_->hide();
  }

  update();
}

void MermaidTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
  if (!isEditing_) {
    startEditing();
    event->accept();
  } else {
    QGraphicsObject::mouseDoubleClickEvent(event);
  }
}

QVariant MermaidTextItem::itemChange(GraphicsItemChange change,
                                     const QVariant &value) {
  if (change == ItemPositionChange && scene()) {
    // Could add snapping or constraint logic here
  }
  return QGraphicsObject::itemChange(change, value);
}

void MermaidTextItem::renderContent() {
  if (mermaidCode_.isEmpty()) {
    renderedContent_ = QPixmap();
    contentRect_ = QRectF(0, 0, MIN_WIDTH, MIN_HEIGHT);
    update();
    return;
  }

#ifdef HAVE_QT_WEBENGINE
  // Connect to the renderer if not already connected
  if (!mermaidConnected_) {
    connect(&MermaidRenderer::instance(), &MermaidRenderer::renderComplete,
            this, &MermaidTextItem::onMermaidRenderComplete);
    mermaidConnected_ = true;
  }

  // Generate unique request ID using static counter for thread safety
  static std::atomic<quintptr> requestCounter{0};
  pendingRenderId_ = ++requestCounter;

  // Request async rendering
  MermaidRenderer::instance().render(mermaidCode_, theme_, pendingRenderId_);
#else
  // No WebEngine - use placeholder
  renderedContent_ = createPlaceholder();
  prepareGeometryChange();
  contentRect_ =
      QRectF(0, 0, renderedContent_.width(), renderedContent_.height());
  update();
#endif
}

#ifdef HAVE_QT_WEBENGINE
void MermaidTextItem::onMermaidRenderComplete(quintptr requestId,
                                              const QPixmap &pixmap,
                                              bool success) {
  if (requestId != pendingRenderId_) {
    return; // Not our request
  }

  prepareGeometryChange();

  if (success && !pixmap.isNull()) {
    renderedContent_ = pixmap;
    contentRect_ = QRectF(0, 0, pixmap.width(), pixmap.height());
  } else {
    // Render failed - use placeholder
    renderedContent_ = createPlaceholder();
    contentRect_ =
        QRectF(0, 0, renderedContent_.width(), renderedContent_.height());
  }

  update();
}
#endif

QPixmap MermaidTextItem::createPlaceholder() const {
  int width = qMax(MIN_WIDTH, 300);
  int height = qMax(MIN_HEIGHT, 150);

  QPixmap pixmap(width, height);
  pixmap.fill(QColor(255, 250, 240));

  QPainter painter(&pixmap);
  painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
  painter.drawRect(0, 0, width - 1, height - 1);

  painter.setPen(Qt::darkGray);
  QFont font("Monospace", 9);
  painter.setFont(font);

  // Show truncated code
  QString displayText = mermaidCode_;
  if (displayText.length() > 100) {
    displayText = displayText.left(100) + "...";
  }

  painter.drawText(
      QRect(PADDING, PADDING, width - 2 * PADDING, height - 2 * PADDING),
      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
      "Mermaid Diagram:\n" + displayText);

  return pixmap;
}
