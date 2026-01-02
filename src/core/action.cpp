#include "action.h"

Action::~Action() {}

DrawAction::DrawAction(QGraphicsItem *item, QGraphicsScene *scene)
    : item(item), scene(scene) {}

DrawAction::~DrawAction() {
  // Item ownership is managed by the scene when added
  // If item was removed and never re-added, we need to clean it up
  if (item && !item->scene()) {
    delete item;
  }
}

void DrawAction::undo() {
  if (item && scene) {
    scene->removeItem(item);
  }
}

void DrawAction::redo() {
  if (item && scene) {
    scene->addItem(item);
  }
}

DeleteAction::DeleteAction(QGraphicsItem *item, QGraphicsScene *scene)
    : item(item), scene(scene) {}

DeleteAction::~DeleteAction() {
  // Item ownership is managed by the scene when added
  // If item was removed and never re-added, we need to clean it up
  if (item && !item->scene()) {
    delete item;
  }
}

void DeleteAction::undo() {
  if (item && scene) {
    scene->addItem(item);
  }
}

void DeleteAction::redo() {
  if (item && scene) {
    scene->removeItem(item);
  }
}
