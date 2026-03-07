/**
 * @file transform_action.cpp
 * @brief Implementation of the transform action for undo/redo.
 *
 * Items are tracked by ItemId only - never by raw pointer.
 */
#include "transform_action.h"
#include "../widgets/latex_text_item.h"
#include "item_store.h"
#include <QGraphicsItem>

TransformAction::TransformAction(const ItemId &id, ItemStore *store,
                                 const QTransform &oldTransform,
                                 const QTransform &newTransform,
                                 const QPointF &oldPos, const QPointF &newPos)
    : itemId_(id), itemStore_(store), oldTransform_(oldTransform),
      newTransform_(newTransform), oldPos_(oldPos), newPos_(newPos) {}

TransformAction::~TransformAction() = default;

void TransformAction::undo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (item) {
    item->setTransform(oldTransform_);
    item->setPos(oldPos_);
  }
}

void TransformAction::redo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (item) {
    item->setTransform(newTransform_);
    item->setPos(newPos_);
  }
}

// TextResizeAction implementation
TextResizeAction::TextResizeAction(const ItemId &id, ItemStore *store,
                                   const QFont &oldFont, const QFont &newFont,
                                   const QPointF &oldPos, const QPointF &newPos)
    : itemId_(id), itemStore_(store), oldFont_(oldFont), newFont_(newFont),
      oldPos_(oldPos), newPos_(newPos) {}

TextResizeAction::~TextResizeAction() = default;

void TextResizeAction::undo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (auto *textItem = dynamic_cast<LatexTextItem *>(item)) {
    textItem->setFont(oldFont_);
    textItem->setPos(oldPos_);
  }
}

void TextResizeAction::redo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (auto *textItem = dynamic_cast<LatexTextItem *>(item)) {
    textItem->setFont(newFont_);
    textItem->setPos(newPos_);
  }
}
