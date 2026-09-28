#ifndef UNDO_REDO_MANAGER_H
#define UNDO_REDO_MANAGER_H

#include "item_id.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

class Action;

/**
 * @brief How much undo history is kept.
 *
 * History is trimmed oldest-first as soon as either limit is exceeded;
 * the most recent action always stays undoable. 0 disables a limit.
 * Memory is what actions report through Action::memoryCost() (captured
 * pixels, parked snapshots of deleted objects), so a few huge raster edits
 * cannot pin gigabytes while many small vector edits keep deep history.
 */
struct HistoryPolicy {
  std::size_t maxSteps = 100;
  std::size_t memoryBudgetBytes = std::size_t(256) * 1024 * 1024;

  bool operator==(const HistoryPolicy &o) const {
    return maxSteps == o.maxSteps && memoryBudgetBytes == o.memoryBudgetBytes;
  }
};

class UndoRedoManager {
public:
  /// Defaults of HistoryPolicy (100 steps, 256 MiB).
  static constexpr std::size_t kDefaultMaxSteps = 100;
  static constexpr std::size_t kDefaultMemoryBudgetBytes =
      std::size_t(256) * 1024 * 1024;

  /// Apply a new policy; trims existing history if it is now over.
  void setPolicy(const HistoryPolicy &policy);
  const HistoryPolicy &policy() const { return policy_; }

  std::size_t undoCount() const { return undoStack_.size(); }
  std::size_t redoCount() const { return redoStack_.size(); }
  /// Sum of Action::memoryCost() over the undo and redo stacks.
  std::size_t memoryUsage() const;

  /**
   * @brief Invoked for every ItemId referenced by an action that is being
   * destroyed without the possibility of being undone/redone anymore
   * (history eviction, redo-stack invalidation, or clear()).
   *
   * Listeners (typically ItemStore owners) use this to release undo
   * snapshot items that would otherwise be parked forever.
   */
  using DiscardListener = std::function<void(const ItemId &)>;

  /**
   * @param owner Optional tag for the surface that created the action (the
   *        canvas and the PDF viewer share one history); see clearOwnedBy().
   */
  void push(std::unique_ptr<Action> action, const void *owner = nullptr);
  void undo();
  void redo();
  void clear();
  /// Drop only the actions pushed with @p owner (e.g. when a PDF closes).
  void clearOwnedBy(const void *owner);

  bool canUndo() const;
  bool canRedo() const;

  /// Owner tag (see push()) of the action undo() / redo() would replay.
  const void *undoOwner() const;
  const void *redoOwner() const;

  void addDiscardListener(DiscardListener listener) {
    discardListeners_.push_back(std::move(listener));
  }

private:
  void enforceLimit();
  void evictOverLimit(std::vector<std::unique_ptr<Action>> &discarded);
  void notifyDiscarded(const std::vector<std::unique_ptr<Action>> &discarded);

  HistoryPolicy policy_;
  std::vector<std::unique_ptr<Action>> undoStack_;
  std::vector<std::unique_ptr<Action>> redoStack_;
  std::vector<DiscardListener> discardListeners_;
  std::unordered_map<const Action *, const void *> owners_;
  // Actions pushed while an action is being replayed (e.g. a callback
  // commits an in-progress gesture) are queued until the replay finished:
  // pushing mid-replay would reshuffle the stacks around the in-flight
  // action.
  bool replaying_ = false;
  std::vector<std::pair<std::unique_ptr<Action>, const void *>> deferred_;
  void flushDeferred();
};

#endif // UNDO_REDO_MANAGER_H
