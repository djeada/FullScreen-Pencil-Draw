/**
 * @file transform_action.cpp
 * @brief Implementation of the transform action for undo/redo.
 */
#include "transform_action.h"
#include "item_store.h"
#include <QGraphicsItem>

TransformAction::TransformAction(QGraphicsItem *item,
                                 const QTransform &oldTransform,
                                 const QTransform &newTransform,
                                 const QPointF &oldPos, const QPointF &newPos)
    : item_(item), itemId_(), itemStore_(nullptr),
      oldTransform_(oldTransform), newTransform_(newTransform),
      oldPos_(oldPos), newPos_(newPos) {}

TransformAction::TransformAction(const ItemId &id, ItemStore *store,
                                 const QTransform &oldTransform,
                                 const QTransform &newTransform,
                                 const QPointF &oldPos, const QPointF &newPos)
    : item_(nullptr), itemId_(id), itemStore_(store),
      oldTransform_(oldTransform), newTransform_(newTransform),
      oldPos_(oldPos), newPos_(newPos) {
  if (store && id.isValid()) {
    item_ = store->item(id);
  }
}

TransformAction::~TransformAction() = default;

QGraphicsItem *TransformAction::resolveItem() const {
  if (itemStore_ && itemId_.isValid()) {
    return itemStore_->item(itemId_);
  }
  return item_;
}

void TransformAction::undo() {
  QGraphicsItem *item = resolveItem();
  if (item) {
    item->setTransform(oldTransform_);
    item->setPos(oldPos_);
  }
}

void TransformAction::redo() {
  QGraphicsItem *item = resolveItem();
  if (item) {
    item->setTransform(newTransform_);
    item->setPos(newPos_);
  }
}
