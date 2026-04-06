#ifndef UNDO_REDO_MANAGER_H
#define UNDO_REDO_MANAGER_H

#include <cstddef>
#include <memory>
#include <vector>

class Action;

class UndoRedoManager {
public:
  static constexpr std::size_t kMaxUndoSteps = 30;

  void push(std::unique_ptr<Action> action);
  void undo();
  void redo();
  void clear();

  bool canUndo() const;
  bool canRedo() const;

private:
  void enforceLimit();

  std::vector<std::unique_ptr<Action>> undoStack_;
  std::vector<std::unique_ptr<Action>> redoStack_;
};

#endif // UNDO_REDO_MANAGER_H
