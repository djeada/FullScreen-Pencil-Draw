#include "undo_redo_manager.h"
#include "action.h"

void UndoRedoManager::push(std::unique_ptr<Action> action) {
  if (!action) {
    return;
  }
  undoStack_.push_back(std::move(action));
  enforceLimit();
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
  undoStack_.clear();
  redoStack_.clear();
}

bool UndoRedoManager::canUndo() const { return !undoStack_.empty(); }

bool UndoRedoManager::canRedo() const { return !redoStack_.empty(); }

void UndoRedoManager::enforceLimit() {
  while (undoStack_.size() > kMaxUndoSteps) {
    undoStack_.erase(undoStack_.begin());
  }
}
