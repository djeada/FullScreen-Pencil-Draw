/**
 * @file transform_action.h
 * @brief Undo/Redo action for transform operations (scale/rotate).
 *
 * Items are tracked by ItemId only - never by raw pointer.
 */
#ifndef TRANSFORM_ACTION_H
#define TRANSFORM_ACTION_H

#include "action.h"
#include "item_id.h"
#include <QTransform>

class ItemStore;

/**
 * @brief Action for transforming items on the scene.
 *
 * This action tracks transformation changes (scale, rotation, etc.) of items.
 * Undo restores the item to its original transform and position,
 * redo applies the new transform and position.
 */
class TransformAction : public Action {
public:
  TransformAction(const ItemId &id, ItemStore *store,
                  const QTransform &oldTransform,
                  const QTransform &newTransform, const QPointF &oldPos,
                  const QPointF &newPos);

  ~TransformAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Transform"; }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  QTransform oldTransform_;
  QTransform newTransform_;
  QPointF oldPos_;
  QPointF newPos_;
};

#endif // TRANSFORM_ACTION_H
