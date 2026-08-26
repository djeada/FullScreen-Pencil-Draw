#ifndef UNDO_REDO_MANAGER_H
#define UNDO_REDO_MANAGER_H

#include "item_id.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

class Action;

class UndoRedoManager {
public:
  static constexpr std::size_t kMaxUndoSteps = 30;

  /**
   * @brief Invoked for every ItemId referenced by an action that is being
   * destroyed without the possibility of being undone/redone anymore
   * (history eviction, redo-stack invalidation, or clear()).
   *
   * Listeners (typically ItemStore owners) use this to release undo
   * snapshot items that would otherwise be parked forever.
   */
  using DiscardListener = std::function<void(const ItemId &)>;

  void push(std::unique_ptr<Action> action);
  void undo();
  void redo();
  void clear();

  bool canUndo() const;
  bool canRedo() const;

  void addDiscardListener(DiscardListener listener) {
    discardListeners_.push_back(std::move(listener));
  }

private:
  void enforceLimit();
  void notifyDiscarded(const Action *action);

  std::vector<std::unique_ptr<Action>> undoStack_;
  std::vector<std::unique_ptr<Action>> redoStack_;
  std::vector<DiscardListener> discardListeners_;
};

#endif // UNDO_REDO_MANAGER_H
