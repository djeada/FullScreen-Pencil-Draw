/**
 * @file action.cpp
 * @brief Implementation of the undo/redo action system.
 *
 * All actions use ItemId-based storage only. Item pointers are NEVER cached.
 */
#include "action.h"
#include "item_store.h"
#include "layer.h"
#include "../widgets/latex_text_item.h"
#include "../widgets/mermaid_text_item.h"
#include <QAbstractGraphicsShapeItem>
#include <QGraphicsColorizeEffect>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <utility>

Action::~Action() = default;

// DrawAction implementation
DrawAction::DrawAction(const ItemId &id, ItemStore *store, ItemCallback onAdd,
                       ItemCallback onRemove)
    : itemId_(id), itemStore_(store), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {}

DrawAction::~DrawAction() = default;

void DrawAction::undo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (item && onRemove_) {
    onRemove_(item);
  }
  itemStore_->scheduleDelete(itemId_, true);
}

void DrawAction::redo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  itemStore_->restoreItem(itemId_);
  QGraphicsItem *item = itemStore_->item(itemId_);
  if (item && onAdd_) {
    onAdd_(item);
  }
}

// DeleteAction implementation
DeleteAction::DeleteAction(const ItemId &id, ItemStore *store,
                           ItemCallback onAdd, ItemCallback onRemove)
    : itemId_(id), itemStore_(store), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {}

DeleteAction::~DeleteAction() = default;

void DeleteAction::undo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  if (itemStore_->restoreItem(itemId_) || itemStore_->item(itemId_)) {
    QGraphicsItem *item = itemStore_->item(itemId_);
    if (item && onAdd_) {
      onAdd_(item);
    }
  }
}

void DeleteAction::redo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (item && onRemove_) {
    onRemove_(item);
  }
  itemStore_->scheduleDelete(itemId_, true);
}

// MoveAction implementation
MoveAction::MoveAction(const ItemId &id, ItemStore *store,
                       const QPointF &oldPos, const QPointF &newPos)
    : itemId_(id), itemStore_(store), oldPos_(oldPos), newPos_(newPos) {}

MoveAction::~MoveAction() = default;

void MoveAction::undo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (item) {
    item->setPos(oldPos_);
  }
}

void MoveAction::redo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
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
FillAction::FillAction(const ItemId &id, ItemStore *store,
                       const QBrush &oldBrush, const QBrush &newBrush)
    : itemId_(id), itemStore_(store), property_(Property::Brush),
      oldBrush_(oldBrush), newBrush_(newBrush) {}

FillAction::FillAction(const ItemId &id, ItemStore *store, const QPen &oldPen,
                       const QPen &newPen)
    : itemId_(id), itemStore_(store), property_(Property::Pen),
      oldPen_(oldPen), newPen_(newPen) {}

FillAction::FillAction(const ItemId &id, ItemStore *store,
                       const QColor &oldColor, const QColor &newColor)
    : itemId_(id), itemStore_(store), property_(Property::TextColor),
      oldColor_(oldColor), newColor_(newColor) {}

FillAction::FillAction(const ItemId &id, ItemStore *store,
                       const QString &oldTheme, const QString &newTheme)
    : itemId_(id), itemStore_(store), property_(Property::MermaidTheme),
      oldTheme_(oldTheme), newTheme_(newTheme) {}

FillAction::FillAction(const ItemId &id, ItemStore *store,
                       const PixmapTintState &oldTintState,
                       const PixmapTintState &newTintState)
    : itemId_(id), itemStore_(store), property_(Property::PixmapTint),
      oldTintState_(oldTintState), newTintState_(newTintState) {}

FillAction::~FillAction() = default;

void FillAction::applyBrush(const QBrush &brush) {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (!item)
    return;

  if (auto *shape = dynamic_cast<QAbstractGraphicsShapeItem *>(item)) {
    shape->setBrush(brush);
  }
}

void FillAction::applyPen(const QPen &pen) {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (!item)
    return;

  if (auto *shape = dynamic_cast<QAbstractGraphicsShapeItem *>(item)) {
    shape->setPen(pen);
  } else if (auto *line = dynamic_cast<QGraphicsLineItem *>(item)) {
    line->setPen(pen);
  }
}

void FillAction::applyTextColor(const QColor &color) {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (!item)
    return;

  if (auto *text = dynamic_cast<QGraphicsTextItem *>(item)) {
    text->setDefaultTextColor(color);
  } else if (auto *latex = dynamic_cast<LatexTextItem *>(item)) {
    latex->setTextColor(color);
  }
}

void FillAction::applyMermaidTheme(const QString &theme) {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (!item)
    return;

  if (auto *mermaid = dynamic_cast<MermaidTextItem *>(item)) {
    mermaid->setTheme(theme);
  }
}

void FillAction::applyPixmapTint(const PixmapTintState &state) {
  if (!itemStore_ || !itemId_.isValid())
    return;

  QGraphicsItem *item = itemStore_->item(itemId_);
  if (!item)
    return;

  auto *pixmap = dynamic_cast<QGraphicsPixmapItem *>(item);
  if (!pixmap)
    return;

  if (!state.enabled) {
    pixmap->setGraphicsEffect(nullptr);
    return;
  }

  auto *effect =
      dynamic_cast<QGraphicsColorizeEffect *>(pixmap->graphicsEffect());
  if (!effect) {
    effect = new QGraphicsColorizeEffect();
    pixmap->setGraphicsEffect(effect);
  }
  effect->setColor(state.color);
  effect->setStrength(state.strength);
}

void FillAction::undo() {
  switch (property_) {
  case Property::Brush:
    applyBrush(oldBrush_);
    break;
  case Property::Pen:
    applyPen(oldPen_);
    break;
  case Property::TextColor:
    applyTextColor(oldColor_);
    break;
  case Property::MermaidTheme:
    applyMermaidTheme(oldTheme_);
    break;
  case Property::PixmapTint:
    applyPixmapTint(oldTintState_);
    break;
  }
}

void FillAction::redo() {
  switch (property_) {
  case Property::Brush:
    applyBrush(newBrush_);
    break;
  case Property::Pen:
    applyPen(newPen_);
    break;
  case Property::TextColor:
    applyTextColor(newColor_);
    break;
  case Property::MermaidTheme:
    applyMermaidTheme(newTheme_);
    break;
  case Property::PixmapTint:
    applyPixmapTint(newTintState_);
    break;
  }
}

RasterPixmapAction::RasterPixmapAction(const ItemId &id, ItemStore *store,
                                       const QImage &oldImage,
                                       const QImage &newImage)
    : itemId_(id), itemStore_(store), oldImage_(oldImage), newImage_(newImage) {
}

RasterPixmapAction::~RasterPixmapAction() = default;

void RasterPixmapAction::undo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  auto *pixmapItem =
      dynamic_cast<QGraphicsPixmapItem *>(itemStore_->item(itemId_));
  if (!pixmapItem || oldImage_.isNull())
    return;

  pixmapItem->setPixmap(QPixmap::fromImage(oldImage_));
}

void RasterPixmapAction::redo() {
  if (!itemStore_ || !itemId_.isValid())
    return;

  auto *pixmapItem =
      dynamic_cast<QGraphicsPixmapItem *>(itemStore_->item(itemId_));
  if (!pixmapItem || newImage_.isNull())
    return;

  pixmapItem->setPixmap(QPixmap::fromImage(newImage_));
}

// GroupAction implementation
GroupAction::GroupAction(const ItemId &groupId, const QList<ItemId> &itemIds,
                         ItemStore *store,
                         const QList<QPointF> &originalPositions,
                         ItemCallback onAdd, ItemCallback onRemove)
    : groupId_(groupId), itemIds_(itemIds), itemStore_(store),
      originalPositions_(originalPositions), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {}

GroupAction::~GroupAction() = default;

void GroupAction::undo() {
  if (!itemStore_ || !groupId_.isValid())
    return;

  auto *group = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
  if (!group)
    return;

  QGraphicsScene *scene = group->scene();
  if (!scene)
    return;

  // Remove group from scene
  scene->removeItem(group);
  if (onRemove_) {
    onRemove_(group);
  }

  // Re-add individual items to scene at their original positions
  for (int i = 0; i < itemIds_.size(); ++i) {
    QGraphicsItem *item = itemStore_->item(itemIds_[i]);
    if (item) {
      // Remove from group first
      group->removeFromGroup(item);
      // Add to scene
      scene->addItem(item);
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
}

void GroupAction::redo() {
  if (!itemStore_ || !groupId_.isValid())
    return;

  auto *group = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
  if (!group)
    return;

  // Get scene from first available item
  QGraphicsScene *scene = nullptr;
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item && item->scene()) {
      scene = item->scene();
      break;
    }
  }
  if (!scene)
    return;

  // Remove individual items from scene and add to group
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item && item->scene()) {
      scene->removeItem(item);
      if (onRemove_) {
        onRemove_(item);
      }
    }
  }

  // Add items to group
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item) {
      group->addToGroup(item);
    }
  }

  // Add group to scene
  scene->addItem(group);
  group->setFlags(QGraphicsItem::ItemIsSelectable |
                  QGraphicsItem::ItemIsMovable);
  if (onAdd_) {
    onAdd_(group);
  }
}

// UngroupAction implementation
UngroupAction::UngroupAction(const ItemId &groupId,
                             const QList<ItemId> &itemIds, ItemStore *store,
                             const QPointF &groupPosition, ItemCallback onAdd,
                             ItemCallback onRemove)
    : groupId_(groupId), itemIds_(itemIds), itemStore_(store),
      groupPosition_(groupPosition), onAdd_(std::move(onAdd)),
      onRemove_(std::move(onRemove)) {}

UngroupAction::~UngroupAction() = default;

void UngroupAction::undo() {
  if (!itemStore_ || !groupId_.isValid())
    return;

  auto *group = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
  if (!group)
    return;

  // Get scene from first available item
  QGraphicsScene *scene = nullptr;
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item && item->scene()) {
      scene = item->scene();
      break;
    }
  }
  if (!scene)
    return;

  // Remove individual items from scene
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item && item->scene()) {
      scene->removeItem(item);
      if (onRemove_) {
        onRemove_(item);
      }
    }
  }

  // Recreate the group
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item) {
      group->addToGroup(item);
    }
  }

  // Add group to scene
  scene->addItem(group);
  group->setPos(groupPosition_);
  group->setFlags(QGraphicsItem::ItemIsSelectable |
                  QGraphicsItem::ItemIsMovable);
  if (onAdd_) {
    onAdd_(group);
  }
}

void UngroupAction::redo() {
  if (!itemStore_ || !groupId_.isValid())
    return;

  auto *group = dynamic_cast<QGraphicsItemGroup *>(itemStore_->item(groupId_));
  if (!group)
    return;

  QGraphicsScene *scene = group->scene();
  if (!scene)
    return;

  // Store positions relative to scene before ungrouping
  QList<QPointF> scenePositions;
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item) {
      scenePositions.append(item->scenePos());
    }
  }

  // Remove group from scene
  scene->removeItem(group);
  if (onRemove_) {
    onRemove_(group);
  }

  // Remove items from group and add to scene
  int i = 0;
  for (const ItemId &id : itemIds_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item) {
      group->removeFromGroup(item);
      scene->addItem(item);
      if (i < scenePositions.size()) {
        item->setPos(scenePositions[i]);
      }
      item->setFlags(QGraphicsItem::ItemIsSelectable |
                     QGraphicsItem::ItemIsMovable);
      if (onAdd_) {
        onAdd_(item);
      }
      ++i;
    }
  }
}

// ReorderAction implementation
ReorderAction::ReorderAction(const ItemId &itemId, const QUuid &layerId,
                             int oldIndex, int newIndex,
                             LayerManager *layerManager)
    : itemId_(itemId), layerId_(layerId), oldIndex_(oldIndex),
      newIndex_(newIndex), layerManager_(layerManager) {}

ReorderAction::~ReorderAction() = default;

void ReorderAction::undo() {
  if (!layerManager_ || !itemId_.isValid())
    return;

  layerManager_->reorderItem(itemId_, oldIndex_);
}

void ReorderAction::redo() {
  if (!layerManager_ || !itemId_.isValid())
    return;

  layerManager_->reorderItem(itemId_, newIndex_);
}
