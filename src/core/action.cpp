/**
 * @file action.cpp
 * @brief Implementation of the undo/redo action system.
 */
#include "action.h"
#include "item_store.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <utility>

Action::~Action() = default;

// DrawAction implementation
DrawAction::DrawAction(QGraphicsItem *item, QGraphicsScene *scene,
                       ItemCallback onAdd, ItemCallback onRemove)
    : item_(item), itemId_(), itemStore_(nullptr), scene_(scene),
      itemOwnedByAction_(false), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {}

DrawAction::DrawAction(const ItemId &id, ItemStore *store, QGraphicsScene *scene,
                       ItemCallback onAdd, ItemCallback onRemove)
    : item_(nullptr), itemId_(id), itemStore_(store), scene_(scene),
      itemOwnedByAction_(false), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {
  // Cache the item pointer for backwards compatibility
  if (store && id.isValid()) {
    item_ = store->item(id);
  }
}

DrawAction::~DrawAction() {
  // If ItemStore owns lifecycle, don't delete directly
  if (itemStore_ && itemId_.isValid()) {
    return;
  }
  // Clean up the item if we own it and it still exists
  if (itemOwnedByAction_ && item_) {
    delete item_;
    item_ = nullptr;
  }
}

QGraphicsItem *DrawAction::resolveItem() const {
  if (itemStore_ && itemId_.isValid()) {
    return itemStore_->item(itemId_);
  }
  return item_;
}

void DrawAction::undo() {
  if (itemStore_ && itemId_.isValid()) {
    QGraphicsItem *item = itemStore_->item(itemId_);
    if (item && onRemove_) {
      onRemove_(item);
    }
    itemStore_->scheduleDelete(itemId_, true);
    return;
  }
  QGraphicsItem *item = resolveItem();
  if (item && scene_) {
    scene_->removeItem(item);
    if (onRemove_) {
      onRemove_(item);
    }
    itemOwnedByAction_ = true;  // We now own the item
  }
}

void DrawAction::redo() {
  if (itemStore_ && itemId_.isValid()) {
    bool restored = itemStore_->restoreItem(itemId_);
    QGraphicsItem *item = itemStore_->item(itemId_);
    if (!item && item_) {
      // Fallback: re-register cached item
      itemStore_->registerItem(item_);
      item = item_;
    }
    if (item && onAdd_) {
      onAdd_(item);
    }
    (void)restored;
    return;
  }
  QGraphicsItem *item = resolveItem();
  if (item && scene_) {
    scene_->addItem(item);
    if (onAdd_) {
      onAdd_(item);
    }
    itemOwnedByAction_ = false;  // Scene now owns the item
  }
}

// DeleteAction implementation
DeleteAction::DeleteAction(QGraphicsItem *item, QGraphicsScene *scene,
                           ItemCallback onAdd, ItemCallback onRemove)
    : item_(item), itemId_(), itemStore_(nullptr), scene_(scene),
      itemOwnedByAction_(true), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {}

DeleteAction::DeleteAction(const ItemId &id, ItemStore *store, QGraphicsScene *scene,
                           ItemCallback onAdd, ItemCallback onRemove)
    : item_(nullptr), itemId_(id), itemStore_(store), scene_(scene),
      itemOwnedByAction_(true), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {
  // Cache the item pointer for backwards compatibility
  if (store && id.isValid()) {
    item_ = store->item(id);
  }
}

DeleteAction::~DeleteAction() {
  // If ItemStore owns lifecycle, don't delete directly
  if (itemStore_ && itemId_.isValid()) {
    return;
  }
  // Clean up the item if we own it and it still exists
  if (itemOwnedByAction_ && item_) {
    delete item_;
    item_ = nullptr;
  }
}

QGraphicsItem *DeleteAction::resolveItem() const {
  if (itemStore_ && itemId_.isValid()) {
    return itemStore_->item(itemId_);
  }
  return item_;
}

void DeleteAction::undo() {
  if (itemStore_ && itemId_.isValid()) {
    if (itemStore_->restoreItem(itemId_) || itemStore_->item(itemId_)) {
      QGraphicsItem *item = itemStore_->item(itemId_);
      if (item && onAdd_) {
        onAdd_(item);
      }
    }
    return;
  }
  QGraphicsItem *item = resolveItem();
  if (item && scene_) {
    scene_->addItem(item);
    if (onAdd_) {
      onAdd_(item);
    }
    itemOwnedByAction_ = false;  // Scene now owns the item
  }
}

void DeleteAction::redo() {
  if (itemStore_ && itemId_.isValid()) {
    QGraphicsItem *item = itemStore_->item(itemId_);
    if (item && onRemove_) {
      onRemove_(item);
    }
    itemStore_->scheduleDelete(itemId_, true);
    return;
  }
  QGraphicsItem *item = resolveItem();
  if (item && scene_) {
    scene_->removeItem(item);
    if (onRemove_) {
      onRemove_(item);
    }
    itemOwnedByAction_ = true;  // We now own the item
  }
}

// MoveAction implementation
MoveAction::MoveAction(QGraphicsItem *item, const QPointF &oldPos,
                       const QPointF &newPos)
    : item_(item), itemId_(), itemStore_(nullptr), oldPos_(oldPos),
      newPos_(newPos) {}

MoveAction::MoveAction(const ItemId &id, ItemStore *store, const QPointF &oldPos,
                       const QPointF &newPos)
    : item_(nullptr), itemId_(id), itemStore_(store), oldPos_(oldPos),
      newPos_(newPos) {
  if (store && id.isValid()) {
    item_ = store->item(id);
  }
}

MoveAction::~MoveAction() = default;

QGraphicsItem *MoveAction::resolveItem() const {
  if (itemStore_ && itemId_.isValid()) {
    return itemStore_->item(itemId_);
  }
  return item_;
}

void MoveAction::undo() {
  QGraphicsItem *item = resolveItem();
  if (item) {
    item->setPos(oldPos_);
  }
}

void MoveAction::redo() {
  QGraphicsItem *item = resolveItem();
  if (item) {
    item->setPos(newPos_);
  }
}

// CompositeAction implementation
CompositeAction::CompositeAction() = default;

CompositeAction::~CompositeAction() = default;

void CompositeAction::addAction(std::unique_ptr<Action> action) {
  actions_.push_back(std::move(action));
}

void CompositeAction::undo() {
  // Undo in reverse order
  for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
    (*it)->undo();
  }
}

void CompositeAction::redo() {
  // Redo in forward order
  for (auto &action : actions_) {
    action->redo();
  }
}

// FillAction implementation
FillAction::FillAction(QGraphicsItem *item, const QBrush &oldBrush, const QBrush &newBrush)
    : item_(item), itemId_(), itemStore_(nullptr), oldBrush_(oldBrush),
      newBrush_(newBrush) {}

FillAction::FillAction(const ItemId &id, ItemStore *store, const QBrush &oldBrush,
                       const QBrush &newBrush)
    : item_(nullptr), itemId_(id), itemStore_(store), oldBrush_(oldBrush),
      newBrush_(newBrush) {
  if (store && id.isValid()) {
    item_ = store->item(id);
  }
}

FillAction::~FillAction() = default;

QGraphicsItem *FillAction::resolveItem() const {
  if (itemStore_ && itemId_.isValid()) {
    return itemStore_->item(itemId_);
  }
  return item_;
}

void FillAction::undo() {
  QGraphicsItem *item = resolveItem();
  if (!item) return;
  
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item)) {
    rect->setBrush(oldBrush_);
  } else if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item)) {
    ellipse->setBrush(oldBrush_);
  } else if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item)) {
    polygon->setBrush(oldBrush_);
  }
}

void FillAction::redo() {
  QGraphicsItem *item = resolveItem();
  if (!item) return;
  
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item)) {
    rect->setBrush(newBrush_);
  } else if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item)) {
    ellipse->setBrush(newBrush_);
  } else if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item)) {
    polygon->setBrush(newBrush_);
  }
}

// GroupAction implementation
GroupAction::GroupAction(QGraphicsItemGroup *group,
                         const QList<QGraphicsItem *> &items,
                         QGraphicsScene *scene, ItemCallback onAdd,
                         ItemCallback onRemove)
    : group_(group), groupId_(), items_(items), itemIds_(), itemStore_(nullptr),
      scene_(scene), groupOwnedByAction_(false),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {
  // Store original positions relative to scene
  for (QGraphicsItem *item : items_) {
    originalPositions_.append(item->scenePos());
  }
}

GroupAction::GroupAction(const ItemId &groupId, const QList<ItemId> &itemIds,
                         ItemStore *store, QGraphicsScene *scene,
                         ItemCallback onAdd, ItemCallback onRemove)
    : group_(nullptr), groupId_(groupId), items_(), itemIds_(itemIds),
      itemStore_(store), scene_(scene), groupOwnedByAction_(false),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {
  // Resolve items for backwards compatibility
  if (store) {
    if (groupId.isValid()) {
      group_ = dynamic_cast<QGraphicsItemGroup *>(store->item(groupId));
    }
    for (const ItemId &id : itemIds) {
      if (QGraphicsItem *item = store->item(id)) {
        items_.append(item);
        originalPositions_.append(item->scenePos());
      }
    }
  }
}

GroupAction::~GroupAction() {
  if (itemStore_ && groupId_.isValid()) {
    return;
  }
  if (groupOwnedByAction_ && group_) {
    delete group_;
    group_ = nullptr;
  }
}

void GroupAction::undo() {
  if (itemStore_ && groupId_.isValid()) {
    group_ = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
    items_.clear();
    for (const ItemId &id : itemIds_) {
      if (QGraphicsItem *item = itemStore_->item(id)) {
        items_.append(item);
      }
    }
  }
  if (!group_ || !scene_) return;

  // Remove group from scene
  scene_->removeItem(group_);
  if (onRemove_) {
    onRemove_(group_);
  }

  // Re-add individual items to scene at their original positions
  for (int i = 0; i < items_.size(); ++i) {
    QGraphicsItem *item = items_[i];
    if (item) {
      // Remove from group first (this doesn't add to scene)
      group_->removeFromGroup(item);
      // Add to scene
      scene_->addItem(item);
      // Restore original scene position
      if (i < originalPositions_.size()) {
        item->setPos(originalPositions_[i]);
      }
      item->setFlags(QGraphicsItem::ItemIsSelectable |
                     QGraphicsItem::ItemIsMovable);
      if (onAdd_) {
        onAdd_(item);
      }
    }
  }

  groupOwnedByAction_ = true;
}

void GroupAction::redo() {
  if (itemStore_ && groupId_.isValid()) {
    group_ = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
    items_.clear();
    for (const ItemId &id : itemIds_) {
      if (QGraphicsItem *item = itemStore_->item(id)) {
        items_.append(item);
      }
    }
  }
  if (!group_ || !scene_) return;

  // Remove individual items and add to group
  for (QGraphicsItem *item : items_) {
    if (item && item->scene()) {
      scene_->removeItem(item);
      if (onRemove_) {
        onRemove_(item);
      }
    }
  }

  // Add items to group
  for (QGraphicsItem *item : items_) {
    if (item) {
      group_->addToGroup(item);
    }
  }

  // Add group to scene
  scene_->addItem(group_);
  group_->setFlags(QGraphicsItem::ItemIsSelectable |
                   QGraphicsItem::ItemIsMovable);
  if (onAdd_) {
    onAdd_(group_);
  }

  groupOwnedByAction_ = false;
}

// UngroupAction implementation
UngroupAction::UngroupAction(QGraphicsItemGroup *group,
                             const QList<QGraphicsItem *> &items,
                             QGraphicsScene *scene, ItemCallback onAdd,
                             ItemCallback onRemove)
    : group_(group), groupId_(), items_(items), itemIds_(), itemStore_(nullptr),
      scene_(scene), groupOwnedByAction_(true),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {
  groupPosition_ = group_->pos();
}

UngroupAction::UngroupAction(const ItemId &groupId, const QList<ItemId> &itemIds,
                             ItemStore *store, QGraphicsScene *scene,
                             ItemCallback onAdd, ItemCallback onRemove)
    : group_(nullptr), groupId_(groupId), items_(), itemIds_(itemIds),
      itemStore_(store), scene_(scene), groupOwnedByAction_(true),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {
  // Resolve items for backwards compatibility
  if (store) {
    if (groupId.isValid()) {
      group_ = dynamic_cast<QGraphicsItemGroup *>(store->item(groupId));
    }
    for (const ItemId &id : itemIds) {
      if (QGraphicsItem *item = store->item(id)) {
        items_.append(item);
      }
    }
  }
  if (group_) {
    groupPosition_ = group_->pos();
  }
}

UngroupAction::~UngroupAction() {
  if (itemStore_ && groupId_.isValid()) {
    return;
  }
  if (groupOwnedByAction_ && group_) {
    delete group_;
    group_ = nullptr;
  }
}

void UngroupAction::undo() {
  if (itemStore_ && groupId_.isValid()) {
    group_ = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
    items_.clear();
    for (const ItemId &id : itemIds_) {
      if (QGraphicsItem *item = itemStore_->item(id)) {
        items_.append(item);
      }
    }
  }
  if (!group_ || !scene_) return;

  // Remove individual items from scene
  for (QGraphicsItem *item : items_) {
    if (item && item->scene()) {
      scene_->removeItem(item);
      if (onRemove_) {
        onRemove_(item);
      }
    }
  }

  // Recreate the group
  for (QGraphicsItem *item : items_) {
    if (item) {
      group_->addToGroup(item);
    }
  }

  // Add group to scene
  scene_->addItem(group_);
  group_->setPos(groupPosition_);
  group_->setFlags(QGraphicsItem::ItemIsSelectable |
                   QGraphicsItem::ItemIsMovable);
  if (onAdd_) {
    onAdd_(group_);
  }

  groupOwnedByAction_ = false;
}

void UngroupAction::redo() {
  if (itemStore_ && groupId_.isValid()) {
    group_ = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
    items_.clear();
    for (const ItemId &id : itemIds_) {
      if (QGraphicsItem *item = itemStore_->item(id)) {
        items_.append(item);
      }
    }
  }
  if (!group_ || !scene_) return;

  // Store positions relative to scene before ungrouping
  QList<QPointF> scenePositions;
  for (QGraphicsItem *item : items_) {
    if (item) {
      scenePositions.append(item->scenePos());
    }
  }

  // Remove group from scene
  scene_->removeItem(group_);
  if (onRemove_) {
    onRemove_(group_);
  }

  // Remove items from group and add to scene
  for (int i = 0; i < items_.size(); ++i) {
    QGraphicsItem *item = items_[i];
    if (item) {
      group_->removeFromGroup(item);
      scene_->addItem(item);
      if (i < scenePositions.size()) {
        item->setPos(scenePositions[i]);
      }
      item->setFlags(QGraphicsItem::ItemIsSelectable |
                     QGraphicsItem::ItemIsMovable);
      if (onAdd_) {
        onAdd_(item);
      }
    }
  }

  groupOwnedByAction_ = true;
}
