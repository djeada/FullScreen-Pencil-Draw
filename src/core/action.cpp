#include "action.h"

Action::~Action() {}
DrawAction::~DrawAction() {
  // Optional cleanup
}
DrawAction::DrawAction(QGraphicsItem *item) : item(item) {}

void DrawAction::undo() { item->scene()->removeItem(item); }

void DrawAction::redo() { item->scene()->addItem(item); }

DeleteAction::DeleteAction(QGraphicsItem *item) : item(item) {}

DeleteAction::~DeleteAction() {
  // Optional cleanup
}

void DeleteAction::undo() { item->scene()->addItem(item); }

void DeleteAction::redo() { item->scene()->removeItem(item); }
