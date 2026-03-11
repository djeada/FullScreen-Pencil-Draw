#ifndef UNDO_REDO_MANAGER_H
#define UNDO_REDO_MANAGER_H

#include <memory>
#include <vector>

class Action;

class UndoRedoManager {
public:
  void push(std::unique_ptr<Action> action);
  void undo();
  void redo();
  void clear();

  bool canUndo() const;
  bool canRedo() const;

private:
  std::vector<std::unique_ptr<Action>> undoStack_;
  std::vector<std::unique_ptr<Action>> redoStack_;
};

#endif // UNDO_REDO_MANAGER_H
