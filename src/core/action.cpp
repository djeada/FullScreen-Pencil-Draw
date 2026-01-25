/**
 * @file action.cpp
 * @brief Implementation of the undo/redo action system.
 */
#include "action.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <utility>

Action::~Action() = default;

// DrawAction implementation
DrawAction::DrawAction(QGraphicsItem *item, QGraphicsScene *scene,
                       ItemCallback onAdd, ItemCallback onRemove)
    : item_(item), scene_(scene), itemOwnedByAction_(false),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {}

DrawAction::~DrawAction() {
  // Clean up the item if we own it and it still exists
  if (itemOwnedByAction_ && item_) {
    delete item_;
    item_ = nullptr;
  }
}

void DrawAction::undo() {
  if (item_ && scene_) {
    scene_->removeItem(item_);
    if (onRemove_) {
      onRemove_(item_);
    }
    itemOwnedByAction_ = true;  // We now own the item
  }
}

void DrawAction::redo() {
  if (item_ && scene_) {
    scene_->addItem(item_);
    if (onAdd_) {
      onAdd_(item_);
    }
    itemOwnedByAction_ = false;  // Scene now owns the item
  }
}

// DeleteAction implementation
DeleteAction::DeleteAction(QGraphicsItem *item, QGraphicsScene *scene,
                           ItemCallback onAdd, ItemCallback onRemove)
    : item_(item), scene_(scene), itemOwnedByAction_(true),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {}

DeleteAction::~DeleteAction() {
  // Clean up the item if we own it and it still exists
  if (itemOwnedByAction_ && item_) {
    delete item_;
    item_ = nullptr;
  }
}

void DeleteAction::undo() {
  if (item_ && scene_) {
    scene_->addItem(item_);
    if (onAdd_) {
      onAdd_(item_);
    }
    itemOwnedByAction_ = false;  // Scene now owns the item
  }
}

void DeleteAction::redo() {
  if (item_ && scene_) {
    scene_->removeItem(item_);
    if (onRemove_) {
      onRemove_(item_);
    }
    itemOwnedByAction_ = true;  // We now own the item
  }
}

// MoveAction implementation
MoveAction::MoveAction(QGraphicsItem *item, const QPointF &oldPos,
                       const QPointF &newPos)
    : item_(item), oldPos_(oldPos), newPos_(newPos) {}

MoveAction::~MoveAction() = default;

void MoveAction::undo() {
  if (item_) {
    item_->setPos(oldPos_);
  }
}

void MoveAction::redo() {
  if (item_) {
    item_->setPos(newPos_);
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
    : item_(item), oldBrush_(oldBrush), newBrush_(newBrush) {}

FillAction::~FillAction() = default;

void FillAction::undo() {
  if (!item_) return;
  
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item_)) {
    rect->setBrush(oldBrush_);
  } else if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item_)) {
    ellipse->setBrush(oldBrush_);
  } else if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item_)) {
    polygon->setBrush(oldBrush_);
  }
}

void FillAction::redo() {
  if (!item_) return;
  
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item_)) {
    rect->setBrush(newBrush_);
  } else if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item_)) {
    ellipse->setBrush(newBrush_);
  } else if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item_)) {
    polygon->setBrush(newBrush_);
  }
}

// GroupAction implementation
GroupAction::GroupAction(QGraphicsItemGroup *group,
                         const QList<QGraphicsItem *> &items,
                         QGraphicsScene *scene, ItemCallback onAdd,
                         ItemCallback onRemove)
    : group_(group), items_(items), scene_(scene), groupOwnedByAction_(false),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {
  // Store original positions relative to scene
  for (QGraphicsItem *item : items_) {
    originalPositions_.append(item->scenePos());
  }
}

GroupAction::~GroupAction() {
  if (groupOwnedByAction_ && group_) {
    delete group_;
    group_ = nullptr;
  }
}

void GroupAction::undo() {
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
    : group_(group), items_(items), scene_(scene), groupOwnedByAction_(true),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {
  groupPosition_ = group_->pos();
}

UngroupAction::~UngroupAction() {
  if (groupOwnedByAction_ && group_) {
    delete group_;
    group_ = nullptr;
  }
}

void UngroupAction::undo() {
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
