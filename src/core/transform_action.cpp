/**
 * @file transform_action.cpp
 * @brief Implementation of the transform action for undo/redo.
 */
#include "transform_action.h"
#include <QGraphicsItem>

TransformAction::TransformAction(QGraphicsItem *item,
                                 const QTransform &oldTransform,
                                 const QTransform &newTransform,
                                 const QPointF &oldPos, const QPointF &newPos)
    : item_(item), oldTransform_(oldTransform), newTransform_(newTransform),
      oldPos_(oldPos), newPos_(newPos) {}

TransformAction::~TransformAction() = default;

void TransformAction::undo() {
  if (item_) {
    item_->setTransform(oldTransform_);
    item_->setPos(oldPos_);
  }
}

void TransformAction::redo() {
  if (item_) {
    item_->setTransform(newTransform_);
    item_->setPos(newPos_);
  }
}
