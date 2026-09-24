#include "undo_redo_manager.h"
#include "action.h"
#include <QSet>
#include <algorithm>

void UndoRedoManager::push(std::unique_ptr<Action> action, const void *owner) {
  if (!action) {
    return;
  }
  if (replaying_) {
    deferred_.emplace_back(std::move(action), owner);
    return;
  }
  if (owner) {
    owners_[action.get()] = owner;
  }
  // Pushing a new action invalidates every redoable action. Detach those
  // (and any history evicted by the limit) before notifying, so none of
  // them counts as a still-live reference to the items it mentions.
  std::vector<std::unique_ptr<Action>> discarded = std::move(redoStack_);
  redoStack_.clear();
  undoStack_.push_back(std::move(action));
  evictOverLimit(discarded);
  notifyDiscarded(discarded);
}

void UndoRedoManager::undo() {
  if (undoStack_.empty()) {
    return;
  }

  std::unique_ptr<Action> action = std::move(undoStack_.back());
  undoStack_.pop_back();
  replaying_ = true;
  action->undo();
  replaying_ = false;
  redoStack_.push_back(std::move(action));
  flushDeferred();
}

void UndoRedoManager::redo() {
  if (redoStack_.empty()) {
    return;
  }

  std::unique_ptr<Action> action = std::move(redoStack_.back());
  redoStack_.pop_back();
  replaying_ = true;
  action->redo();
  replaying_ = false;
  undoStack_.push_back(std::move(action));
  enforceLimit();
  flushDeferred();
}

void UndoRedoManager::flushDeferred() {
  auto deferred = std::move(deferred_);
  deferred_.clear();
  for (auto &entry : deferred) {
    push(std::move(entry.first), entry.second);
  }
}

const void *UndoRedoManager::undoOwner() const {
  if (undoStack_.empty())
    return nullptr;
  const auto it = owners_.find(undoStack_.back().get());
  return it == owners_.end() ? nullptr : it->second;
}

const void *UndoRedoManager::redoOwner() const {
  if (redoStack_.empty())
    return nullptr;
  const auto it = owners_.find(redoStack_.back().get());
  return it == owners_.end() ? nullptr : it->second;
}

void UndoRedoManager::clear() {
  std::vector<std::unique_ptr<Action>> discarded = std::move(undoStack_);
  undoStack_.clear();
  for (auto &action : redoStack_) {
    discarded.push_back(std::move(action));
  }
  redoStack_.clear();
  notifyDiscarded(discarded);
}

void UndoRedoManager::clearOwnedBy(const void *owner) {
  std::vector<std::unique_ptr<Action>> discarded;
  auto extract = [&](std::vector<std::unique_ptr<Action>> &stack) {
    for (auto it = stack.begin(); it != stack.end();) {
      const auto found = owners_.find(it->get());
      if (found != owners_.end() && found->second == owner) {
        discarded.push_back(std::move(*it));
        it = stack.erase(it);
      } else {
        ++it;
      }
    }
  };
  extract(undoStack_);
  extract(redoStack_);
  notifyDiscarded(discarded);
}

bool UndoRedoManager::canUndo() const { return !undoStack_.empty(); }

bool UndoRedoManager::canRedo() const { return !redoStack_.empty(); }

void UndoRedoManager::enforceLimit() {
  std::vector<std::unique_ptr<Action>> discarded;
  evictOverLimit(discarded);
  notifyDiscarded(discarded);
}

void UndoRedoManager::setPolicy(const HistoryPolicy &policy) {
  policy_ = policy;
  enforceLimit();
}

std::size_t UndoRedoManager::memoryUsage() const {
  std::size_t bytes = 0;
  for (const auto &action : undoStack_)
    bytes += action->memoryCost();
  for (const auto &action : redoStack_)
    bytes += action->memoryCost();
  return bytes;
}

void UndoRedoManager::evictOverLimit(
    std::vector<std::unique_ptr<Action>> &discarded) {
  // Step limit: oldest undoable actions go first.
  if (policy_.maxSteps > 0) {
    while (undoStack_.size() > policy_.maxSteps) {
      discarded.push_back(std::move(undoStack_.front()));
      undoStack_.erase(undoStack_.begin());
    }
  }
  if (policy_.memoryBudgetBytes == 0)
    return;
  std::size_t used = memoryUsage();
  // Memory budget: drop the oldest undo steps, keeping the latest one...
  while (used > policy_.memoryBudgetBytes && undoStack_.size() > 1) {
    used -= std::min(used, undoStack_.front()->memoryCost());
    discarded.push_back(std::move(undoStack_.front()));
    undoStack_.erase(undoStack_.begin());
  }
  // ...then the redo steps furthest away from the current state.
  while (used > policy_.memoryBudgetBytes && !redoStack_.empty()) {
    used -= std::min(used, redoStack_.front()->memoryCost());
    discarded.push_back(std::move(redoStack_.front()));
    redoStack_.erase(redoStack_.begin());
  }
}

void UndoRedoManager::notifyDiscarded(
    const std::vector<std::unique_ptr<Action>> &discarded) {
  for (const auto &action : discarded) {
    owners_.erase(action.get());
  }
  if (discarded.empty() || discardListeners_.empty()) {
    return;
  }
  // An item can be referenced by several actions (e.g. delete, undo, delete
  // again). Only release it once no surviving action can bring it back.
  QVector<ItemId> liveIds;
  for (const auto &action : undoStack_) {
    action->collectReferencedItems(liveIds);
  }
  for (const auto &action : redoStack_) {
    action->collectReferencedItems(liveIds);
  }
  const QSet<ItemId> live(liveIds.cbegin(), liveIds.cend());

  QVector<ItemId> ids;
  for (const auto &action : discarded) {
    if (action) {
      action->collectReferencedItems(ids);
    }
  }
  QSet<ItemId> notified;
  for (const ItemId &id : ids) {
    if (live.contains(id) || notified.contains(id)) {
      continue;
    }
    notified.insert(id);
    for (const auto &listener : discardListeners_) {
      if (listener) {
        listener(id);
      }
    }
  }
}
