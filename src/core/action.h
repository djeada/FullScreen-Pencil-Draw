/**
 * @file action.h
 * @brief Undo/Redo action system for canvas operations.
 *
 * This file defines the polymorphic action classes used to implement
 * undo/redo functionality. Each action represents a reversible operation
 * on the canvas.
 */
#ifndef ACTION_H
#define ACTION_H

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <QPointer>
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief Abstract base class for all undoable actions.
 *
 * Actions encapsulate operations that can be undone and redone.
 * Subclasses implement specific operations like drawing, deleting,
 * and moving items.
 */
class Action {
public:
  Action() = default;
  virtual ~Action();

  /**
   * @brief Undo the action, reverting to the previous state.
   */
  virtual void undo() = 0;

  /**
   * @brief Redo the action, reapplying the operation.
   */
  virtual void redo() = 0;

  /**
   * @brief Get a description of this action for display purposes.
   * @return A human-readable description of the action.
   */
  virtual QString description() const { return "Action"; }
};

/**
 * @brief Action for adding items to the scene.
 *
 * This action tracks items that were drawn/added to the scene.
 * Undo removes the item, redo adds it back.
 * Uses QPointer to safely track item lifetime.
 */
class DrawAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  DrawAction(QGraphicsItem *item, QGraphicsScene *scene,
             ItemCallback onAdd = {}, ItemCallback onRemove = {});
  ~DrawAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Draw"; }

private:
  QGraphicsItem *item_;
  QPointer<QGraphicsScene> scene_;
  bool itemOwnedByAction_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for removing items from the scene.
 *
 * This action tracks items that were deleted from the scene.
 * Undo adds the item back, redo removes it again.
 * Uses QPointer to safely track item lifetime.
 */
class DeleteAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  DeleteAction(QGraphicsItem *item, QGraphicsScene *scene,
               ItemCallback onAdd = {}, ItemCallback onRemove = {});
  ~DeleteAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Delete"; }

private:
  QGraphicsItem *item_;
  QPointer<QGraphicsScene> scene_;
  bool itemOwnedByAction_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for moving items on the scene.
 *
 * This action tracks position changes of items.
 * Undo moves the item back to its original position,
 * redo moves it to the new position.
 * Uses QPointer to safely track item lifetime.
 */
class MoveAction : public Action {
public:
  MoveAction(QGraphicsItem *item, const QPointF &oldPos, const QPointF &newPos);
  ~MoveAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Move"; }

private:
  QGraphicsItem *item_;
  QPointF oldPos_;
  QPointF newPos_;
};

/**
 * @brief Composite action that groups multiple actions together.
 *
 * This allows multiple operations to be undone/redone as a single unit.
 * Useful for operations like paste that add multiple items.
 */
class CompositeAction : public Action {
public:
  CompositeAction();
  ~CompositeAction() override;

  /**
   * @brief Add an action to this composite.
   * @param action The action to add (ownership transferred)
   */
  void addAction(std::unique_ptr<Action> action);

  /**
   * @brief Check if this composite contains any actions.
   * @return true if the composite is empty
   */
  bool isEmpty() const { return actions_.empty(); }

  void undo() override;
  void redo() override;
  QString description() const override { return "Composite Action"; }

private:
  std::vector<std::unique_ptr<Action>> actions_;
};

/**
 * @brief Action for filling a shape with a color.
 *
 * This action tracks fill operations on fillable shapes.
 * Undo restores the original brush, redo applies the fill again.
 * Uses QPointer to safely track item lifetime.
 */
class FillAction : public Action {
public:
  FillAction(QGraphicsItem *item, const QBrush &oldBrush, const QBrush &newBrush);
  ~FillAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Fill"; }

private:
  QGraphicsItem *item_;
  QBrush oldBrush_;
  QBrush newBrush_;
};

class QGraphicsItemGroup;

/**
 * @brief Action for grouping multiple items together.
 *
 * This action tracks group operations.
 * Undo ungroups the items, redo groups them again.
 */
class GroupAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  GroupAction(QGraphicsItemGroup *group, const QList<QGraphicsItem *> &items,
              QGraphicsScene *scene, ItemCallback onAdd = {},
              ItemCallback onRemove = {});
  ~GroupAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Group"; }

private:
  QGraphicsItemGroup *group_;
  QList<QGraphicsItem *> items_;
  QList<QPointF> originalPositions_;
  QPointer<QGraphicsScene> scene_;
  bool groupOwnedByAction_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for ungrouping a group into individual items.
 *
 * This action tracks ungroup operations.
 * Undo regroups the items, redo ungroups them again.
 */
class UngroupAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  UngroupAction(QGraphicsItemGroup *group, const QList<QGraphicsItem *> &items,
                QGraphicsScene *scene, ItemCallback onAdd = {},
                ItemCallback onRemove = {});
  ~UngroupAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Ungroup"; }

private:
  QGraphicsItemGroup *group_;
  QList<QGraphicsItem *> items_;
  QPointF groupPosition_;
  QPointer<QGraphicsScene> scene_;
  bool groupOwnedByAction_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

#endif // ACTION_H
