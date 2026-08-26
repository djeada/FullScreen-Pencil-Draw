#include "undo_redo_manager.h"
#include "action.h"

void UndoRedoManager::push(std::unique_ptr<Action> action) {
  if (!action) {
    return;
  }
  undoStack_.push_back(std::move(action));
  enforceLimit();
  // Pushing a new action invalidates every redoable action; release any
  // snapshots they were keeping alive before destroying them.
  for (const auto &discarded : redoStack_) {
    notifyDiscarded(discarded.get());
  }
  redoStack_.clear();
}

void UndoRedoManager::undo() {
  if (undoStack_.empty()) {
    return;
  }

  std::unique_ptr<Action> action = std::move(undoStack_.back());
  undoStack_.pop_back();
  action->undo();
  redoStack_.push_back(std::move(action));
}

void UndoRedoManager::redo() {
  if (redoStack_.empty()) {
    return;
  }

  std::unique_ptr<Action> action = std::move(redoStack_.back());
  redoStack_.pop_back();
  action->redo();
  undoStack_.push_back(std::move(action));
  enforceLimit();
}

void UndoRedoManager::clear() {
  for (const auto &action : undoStack_) {
    notifyDiscarded(action.get());
  }
  for (const auto &action : redoStack_) {
    notifyDiscarded(action.get());
  }
  undoStack_.clear();
  redoStack_.clear();
}

bool UndoRedoManager::canUndo() const { return !undoStack_.empty(); }

bool UndoRedoManager::canRedo() const { return !redoStack_.empty(); }

void UndoRedoManager::enforceLimit() {
  while (undoStack_.size() > kMaxUndoSteps) {
    notifyDiscarded(undoStack_.front().get());
    undoStack_.erase(undoStack_.begin());
  }
}

void UndoRedoManager::notifyDiscarded(const Action *action) {
  if (!action || discardListeners_.empty()) {
    return;
  }
  QVector<ItemId> ids;
  action->collectReferencedItems(ids);
  for (const ItemId &id : ids) {
    for (const auto &listener : discardListeners_) {
      if (listener) {
        listener(id);
      }
    }
  }
}
