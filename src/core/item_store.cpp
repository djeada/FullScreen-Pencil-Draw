/**
 * @file item_store.cpp
 * @brief Implementation of the ItemStore class.
 */
#include "item_store.h"
#include <QCoreApplication>
#include <QList>
#include <QSet>
#include <algorithm>

namespace {
void collectDescendants(QGraphicsItem *item, QList<QGraphicsItem *> &out) {
  if (!item) {
    return;
  }
  const auto children = item->childItems();
  for (QGraphicsItem *child : children) {
    if (!child) {
      continue;
    }
    out.append(child);
    collectDescendants(child, out);
  }
}

bool hasAncestorInSet(QGraphicsItem *item, const QSet<QGraphicsItem *> &set) {
  for (QGraphicsItem *parent = item ? item->parentItem() : nullptr; parent;
       parent = parent->parentItem()) {
    if (set.contains(parent)) {
      return true;
    }
  }
  return false;
}
} // namespace

ItemStore::ItemStore(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), scene_(scene) {}

ItemStore::~ItemStore() {
  // Clean up any remaining items in the deletion queue
  flushDeletions();

  // Clean up snapshot items (kept for potential undo)
  QSet<QGraphicsItem *> snapshotPtrs;
  for (const auto &pair : snapshotItems_) {
    snapshotPtrs.insert(pair.second);
  }
  for (const auto &pair : snapshotItems_) {
    QGraphicsItem *item = pair.second;
    if (!item) {
      continue;
    }
    // Avoid double-deleting child items when a parent is also tracked.
    if (hasAncestorInSet(item, snapshotPtrs)) {
      continue;
    }
    delete item;
  }
  snapshotItems_.clear();
}

ItemId ItemStore::registerItem(QGraphicsItem *item) {
  if (!item) {
    return ItemId();
  }

  // Check if already registered
  auto reverseIt = reverseMap_.find(item);
  if (reverseIt != reverseMap_.end()) {
    if (scene_ && !item->scene()) {
      scene_->addItem(item);
    }
    return reverseIt->second;
  }

  // Generate a new ID
  ItemId id = ItemId::generate();

  // Add to maps
  items_[id] = item;
  reverseMap_[item] = id;

  // Add to scene if not already present
  if (scene_ && !item->scene()) {
    scene_->addItem(item);
  }

  emit itemRegistered(id);
  return id;
}

QGraphicsItem *ItemStore::unregisterItem(const ItemId &id) {
  if (!id.isValid()) {
    return nullptr;
  }

  auto it = items_.find(id);
  if (it == items_.end()) {
    return nullptr;
  }

  QGraphicsItem *item = it->second;
  items_.erase(it);
  reverseMap_.erase(item);

  return item;
}

QGraphicsItem *ItemStore::item(const ItemId &id) const {
  if (!id.isValid()) {
    return nullptr;
  }

  auto it = items_.find(id);
  if (it != items_.end()) {
    return it->second;
  }

  return nullptr;
}

bool ItemStore::contains(const ItemId &id) const {
  return id.isValid() && items_.find(id) != items_.end();
}

ItemId ItemStore::idForItem(QGraphicsItem *item) const {
  if (!item) {
    return ItemId();
  }

  auto it = reverseMap_.find(item);
  if (it != reverseMap_.end()) {
    return it->second;
  }

  return ItemId();
}

void ItemStore::scheduleDelete(const ItemId &id) {
  scheduleDelete(id, false);
}

void ItemStore::scheduleDelete(const ItemId &id, bool keepSnapshot) {
  if (!id.isValid()) {
    return;
  }

  auto it = items_.find(id);
  if (it == items_.end()) {
    return;
  }

  QGraphicsItem *item = it->second;

  // Remove from scene FIRST, before any signal emissions that might trigger paint
  if (item->scene()) {
    item->scene()->removeItem(item);
  }

  emit itemAboutToBeDeleted(id);

  // Also notify and remove any registered descendants to avoid stale pointers.
  QList<QGraphicsItem *> descendants;
  collectDescendants(item, descendants);
  if (!descendants.isEmpty()) {
    for (QGraphicsItem *child : descendants) {
      if (!child) {
        continue;
      }
      // Remove child from scene too
      if (child->scene()) {
        child->scene()->removeItem(child);
      }
      auto childIt = reverseMap_.find(child);
      if (childIt == reverseMap_.end()) {
        continue;
      }
      ItemId childId = childIt->second;
      emit itemAboutToBeDeleted(childId);
      items_.erase(childId);
      reverseMap_.erase(child);
      if (keepSnapshot) {
        snapshotItems_[childId] = child;
      }
    }
  }

  // Remove from active tracking
  items_.erase(it);
  reverseMap_.erase(item);

  if (keepSnapshot) {
    // Keep for potential undo
    snapshotItems_[id] = item;
  } else {
    // Schedule for permanent deletion
    deletionQueue_.emplace_back(id, item);
  }
}

void ItemStore::flushDeletions() {
  // Actually delete items in the queue
  QSet<QGraphicsItem *> queuedPtrs;
  for (const auto &pair : deletionQueue_) {
    queuedPtrs.insert(pair.second);
  }
  for (const auto &pair : deletionQueue_) {
    QGraphicsItem *item = pair.second;
    if (!item) {
      continue;
    }
    // Avoid double-deleting child items when a parent is also queued.
    if (hasAncestorInSet(item, queuedPtrs)) {
      continue;
    }
    // Always remove from ANY scene, not just our tracked scene
    if (item->scene()) {
      item->scene()->removeItem(item);
    }
    delete item;
  }
  deletionQueue_.clear();
}

bool ItemStore::restoreItem(const ItemId &id) {
  if (!id.isValid()) {
    return false;
  }

  // Find in snapshot storage
  auto it = snapshotItems_.find(id);
  if (it == snapshotItems_.end()) {
    return false;
  }

  QGraphicsItem *item = it->second;
  snapshotItems_.erase(it);

  // Re-register the item
  items_[id] = item;
  reverseMap_[item] = id;

  // Add back to scene
  if (scene_ && !item->scene()) {
    scene_->addItem(item);
  }

  // Re-register any descendants that were stored as snapshots
  QList<QGraphicsItem *> descendants;
  collectDescendants(item, descendants);
  if (!descendants.isEmpty()) {
    for (QGraphicsItem *child : descendants) {
      if (!child) {
        continue;
      }
      ItemId childId;
      auto snapIt = snapshotItems_.begin();
      while (snapIt != snapshotItems_.end()) {
        if (snapIt->second == child) {
          childId = snapIt->first;
          snapIt = snapshotItems_.erase(snapIt);
          break;
        }
        ++snapIt;
      }
      if (childId.isValid()) {
        items_[childId] = child;
        reverseMap_[child] = childId;
        emit itemRestored(childId);
      }
    }
  }

  emit itemRestored(id);
  return true;
}

bool ItemStore::isPendingDeletion(const ItemId &id) const {
  for (const auto &pair : deletionQueue_) {
    if (pair.first == id) {
      return true;
    }
  }
  return false;
}

int ItemStore::itemCount() const {
  return static_cast<int>(items_.size());
}

std::vector<ItemId> ItemStore::allItemIds() const {
  std::vector<ItemId> result;
  result.reserve(items_.size());
  for (const auto &pair : items_) {
    result.push_back(pair.first);
  }
  return result;
}

void ItemStore::clear() {
  // Schedule all items for deletion
  std::vector<ItemId> ids;
  ids.reserve(items_.size());
  for (const auto &pair : items_) {
    ids.push_back(pair.first);
  }

  for (const auto &id : ids) {
    scheduleDelete(id);
  }

  // Also clear snapshot items
  for (auto &pair : snapshotItems_) {
    deletionQueue_.emplace_back(pair.first, pair.second);
  }
  snapshotItems_.clear();
}
