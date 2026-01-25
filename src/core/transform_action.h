/**
 * @file transform_action.h
 * @brief Undo/Redo action for transform operations (scale/rotate).
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
 * Supports both raw pointer and ItemId-based resolution.
 */
class TransformAction : public Action {
public:
  TransformAction(QGraphicsItem *item, const QTransform &oldTransform,
                  const QTransform &newTransform, const QPointF &oldPos,
                  const QPointF &newPos);
  
  /**
   * @brief Construct with ItemId for safe reference
   */
  TransformAction(const ItemId &id, ItemStore *store,
                  const QTransform &oldTransform, const QTransform &newTransform,
                  const QPointF &oldPos, const QPointF &newPos);
  
  ~TransformAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Transform"; }

private:
  QGraphicsItem *item_;
  ItemId itemId_;           // Stable ID for safe reference
  ItemStore *itemStore_;    // For resolving ItemId
  QTransform oldTransform_;
  QTransform newTransform_;
  QPointF oldPos_;
  QPointF newPos_;
  
  QGraphicsItem *resolveItem() const;
};

#endif // TRANSFORM_ACTION_H
