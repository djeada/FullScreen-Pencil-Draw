// text_tool.cpp
#include "text_tool.h"
#include "../widgets/canvas.h"
#include <QFont>
#include <QGraphicsTextItem>
#include <QInputDialog>

TextTool::TextTool(Canvas *canvas) : Tool(canvas) {}

TextTool::~TextTool() = default;

void TextTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->button() == Qt::LeftButton) {
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
  bool ok;
  QString text = QInputDialog::getText(
      canvas_->viewport(), "Add Text", "Enter text:", QLineEdit::Normal, "",
      &ok);

  if (ok && !text.isEmpty()) {
    auto *textItem = new QGraphicsTextItem(text);
    textItem->setFont(QFont("Arial", canvas_->currentPen().width() * 4));
    textItem->setDefaultTextColor(canvas_->currentPen().color());
    textItem->setPos(position);
    textItem->setFlags(QGraphicsItem::ItemIsSelectable |
                       QGraphicsItem::ItemIsMovable);

    canvas_->scene()->addItem(textItem);
    canvas_->addDrawAction(textItem);
  }
}
