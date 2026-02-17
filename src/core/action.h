/**
 * @file action.h
 * @brief Undo/Redo action system for canvas operations.
 *
 * All actions use ItemId-based storage for safe undo/redo.
 * Item pointers are NEVER cached - always resolved via ItemStore.
 */
#ifndef ACTION_H
#define ACTION_H

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <QPointer>
#include <QUuid>
#include <functional>
#include <memory>
#include <vector>

#include "item_id.h"

class ItemStore;

/**
 * @brief Abstract base class for all undoable actions.
 */
class Action {
public:
  Action() = default;
  virtual ~Action();

  virtual void undo() = 0;
  virtual void redo() = 0;
  virtual QString description() const { return "Action"; }
};

/**
 * @brief Action for adding items to the scene.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class DrawAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  DrawAction(const ItemId &id, ItemStore *store, ItemCallback onAdd = {},
             ItemCallback onRemove = {});
  ~DrawAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Draw"; }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for removing items from the scene.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class DeleteAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  DeleteAction(const ItemId &id, ItemStore *store, ItemCallback onAdd = {},
               ItemCallback onRemove = {});
  ~DeleteAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Delete"; }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for moving items on the scene.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class MoveAction : public Action {
public:
  MoveAction(const ItemId &id, ItemStore *store, const QPointF &oldPos,
             const QPointF &newPos);
  ~MoveAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Move"; }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  QPointF oldPos_;
  QPointF newPos_;
};

/**
 * @brief Composite action that groups multiple actions together.
 */
class CompositeAction : public Action {
public:
  CompositeAction();
  ~CompositeAction() override;

  void addAction(std::unique_ptr<Action> action);
  bool isEmpty() const { return actions_.empty(); }

  void undo() override;
  void redo() override;
  QString description() const override { return "Composite Action"; }

private:
  std::vector<std::unique_ptr<Action>> actions_;
};

/**
 * @brief Action for filling a shape with a color.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class FillAction : public Action {
public:
  FillAction(const ItemId &id, ItemStore *store, const QBrush &oldBrush,
             const QBrush &newBrush);
  ~FillAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Fill"; }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  QBrush oldBrush_;
  QBrush newBrush_;
};

class QGraphicsItemGroup;

class LayerManager;

/**
 * @brief Action for reordering an item's z-position within its layer.
 * Tracks the layer by UUID and item by ItemId.
 */
class ReorderAction : public Action {
public:
  ReorderAction(const ItemId &itemId, const QUuid &layerId, int oldIndex,
                int newIndex, LayerManager *layerManager);
  ~ReorderAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Reorder"; }

private:
  ItemId itemId_;
  QUuid layerId_;
  int oldIndex_;
  int newIndex_;
  LayerManager *layerManager_;
};

/**
 * @brief Action for grouping multiple items together.
 * All items tracked by ItemId only - never by raw pointer.
 */
class GroupAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  GroupAction(const ItemId &groupId, const QList<ItemId> &itemIds,
              ItemStore *store, const QList<QPointF> &originalPositions,
              ItemCallback onAdd = {}, ItemCallback onRemove = {});
  ~GroupAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Group"; }

private:
  ItemId groupId_;
  QList<ItemId> itemIds_;
  ItemStore *itemStore_;
  QList<QPointF> originalPositions_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for ungrouping a group into individual items.
 * All items tracked by ItemId only - never by raw pointer.
 */
class UngroupAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  UngroupAction(const ItemId &groupId, const QList<ItemId> &itemIds,
                ItemStore *store, const QPointF &groupPosition,
                ItemCallback onAdd = {}, ItemCallback onRemove = {});
  ~UngroupAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Ungroup"; }

private:
  ItemId groupId_;
  QList<ItemId> itemIds_;
  ItemStore *itemStore_;
  QPointF groupPosition_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

#endif // ACTION_H
