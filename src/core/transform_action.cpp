/**
 * @file transform_action.cpp
 * @brief Implementation of the transform action for undo/redo.
 *
 * Items are tracked by ItemId only - never by raw pointer.
 */
#include "transform_action.h"
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
