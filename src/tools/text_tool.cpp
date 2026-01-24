/**
 * @file text_tool.cpp
 * @brief Text annotation tool implementation with inline LaTeX editing.
 */
#include "text_tool.h"
#include "../core/scene_renderer.h"
#include "../widgets/latex_text_item.h"
#include <QFont>
#include <QPointer>

TextTool::TextTool(SceneRenderer *renderer) : Tool(renderer), currentEditingItem_(nullptr) {}

TextTool::~TextTool() = default;

void TextTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->button() == Qt::LeftButton) {
    // Check if we clicked on an existing LatexTextItem
    QGraphicsItem *item = renderer_->scene()->itemAt(scenePos, QTransform());
    if (auto *latexItem = dynamic_cast<LatexTextItem *>(item)) {
      // If item is not editing, start editing it
      if (!latexItem->isEditing()) {
        latexItem->startEditing();
        currentEditingItem_ = latexItem;
      }
      return;
    }

    // If there's a currently editing item, finish editing first
    if (currentEditingItem_ && currentEditingItem_->isEditing()) {
      // The editing will be finished by the focus out event
      currentEditingItem_ = nullptr;
    }

    // Create a new LatexTextItem with inline editing
    createTextItem(scenePos);
  }
}

void TextTool::mouseMoveEvent(QMouseEvent * /*event*/,
                               const QPointF & /*scenePos*/) {
  // Nothing to do on move
}

void TextTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                  const QPointF & /*scenePos*/) {
  // Nothing to do on release
}

void TextTool::createTextItem(const QPointF &position) {
  auto *textItem = new LatexTextItem();
  textItem->setFont(QFont("Arial", qMax(12, renderer_->currentPen().width() * 3)));
  textItem->setTextColor(renderer_->currentPen().color());
  textItem->setPos(position);

  renderer_->scene()->addItem(textItem);

  // Connect to handle when editing is finished
  // Use QPointer to safely track the textItem in case it gets deleted before signal fires
  QObject::connect(textItem, &LatexTextItem::editingFinished, [this, textItem = QPointer<LatexTextItem>(textItem)]() {
    // Check if textItem is still valid (not deleted)
    if (!textItem) {
      return;
    }
    // If the text is empty after editing, remove the item
    if (textItem->text().trimmed().isEmpty()) {
      renderer_->scene()->removeItem(textItem);
      textItem->deleteLater();
    } else {
      // Add to undo stack only when there's actual content
      renderer_->addDrawAction(textItem);
    }
    if (currentEditingItem_ == textItem) {
      currentEditingItem_ = nullptr;
    }
  });

  // Start inline editing immediately
  textItem->startEditing();
  currentEditingItem_ = textItem;
}
