/**
 * @file text_tool.cpp
 * @brief Text annotation tool implementation.
 */
#include "text_tool.h"
#include "../widgets/canvas.h"
#include "../widgets/latex_text_item.h"
#include <QFont>
#include <QInputDialog>

TextTool::TextTool(Canvas *canvas) : Tool(canvas), currentEditingItem_(nullptr) {}

TextTool::~TextTool() = default;

void TextTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->button() == Qt::LeftButton) {
    // Check if we clicked on an existing LatexTextItem
    QGraphicsItem *item = canvas_->scene()->itemAt(scenePos, QTransform());
    if (auto *latexItem = dynamic_cast<LatexTextItem *>(item)) {
      // Start editing the existing item
      latexItem->startEditing();
      // If text is empty after editing, remove the item
      if (latexItem->text().isEmpty()) {
        canvas_->scene()->removeItem(latexItem);
        delete latexItem;
      }
      return;
    }

    // Create a new LatexTextItem
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
  // Get text from user via dialog
  bool ok;
  QString text = QInputDialog::getText(
      canvas_->viewport(), "Add Text",
      "Enter text (use $...$ for LaTeX math):",
      QLineEdit::Normal, "", &ok);

  if (ok && !text.isEmpty()) {
    auto *textItem = new LatexTextItem();
    textItem->setFont(QFont("Arial", canvas_->currentPen().width() * 4));
    textItem->setTextColor(canvas_->currentPen().color());
    textItem->setText(text);
    textItem->setPos(position);

    canvas_->scene()->addItem(textItem);
    canvas_->addDrawAction(textItem);
  }
}
