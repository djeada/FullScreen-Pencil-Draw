/**
 * @file transform_action.h
 * @brief Undo/Redo action for transform operations (scale/rotate).
 */
#ifndef TRANSFORM_ACTION_H
#define TRANSFORM_ACTION_H

#include "action.h"
#include <QTransform>

/**
 * @brief Action for transforming items on the scene.
 *
 * This action tracks transformation changes (scale, rotation, etc.) of items.
 * Undo restores the item to its original transform and position,
 * redo applies the new transform and position.
 * Uses QPointer to safely track item lifetime.
 */
class TransformAction : public Action {
public:
  TransformAction(QGraphicsItem *item, const QTransform &oldTransform,
                  const QTransform &newTransform, const QPointF &oldPos,
                  const QPointF &newPos);
  ~TransformAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Transform"; }

private:
  QGraphicsItem *item_;
  QTransform oldTransform_;
  QTransform newTransform_;
  QPointF oldPos_;
  QPointF newPos_;
};

#endif // TRANSFORM_ACTION_H
