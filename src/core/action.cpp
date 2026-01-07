/**
 * @file action.cpp
 * @brief Implementation of the undo/redo action system.
 */
#include "action.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>

Action::~Action() = default;

// DrawAction implementation
DrawAction::DrawAction(QGraphicsItem *item, QGraphicsScene *scene)
    : item_(item), scene_(scene) {}

DrawAction::~DrawAction() {
  // Item ownership is managed by the scene when added
  // If item was removed and never re-added, we need to clean it up
  if (item_ && !item_->scene()) {
    delete item_;
  }
}

void DrawAction::undo() {
  if (item_ && scene_) {
    scene_->removeItem(item_);
  }
}

void DrawAction::redo() {
  if (item_ && scene_) {
    scene_->addItem(item_);
  }
}

// DeleteAction implementation
DeleteAction::DeleteAction(QGraphicsItem *item, QGraphicsScene *scene)
    : item_(item), scene_(scene) {}

DeleteAction::~DeleteAction() {
  // Item ownership is managed by the scene when added
  // If item was removed and never re-added, we need to clean it up
  if (item_ && !item_->scene()) {
    delete item_;
  }
}

void DeleteAction::undo() {
  if (item_ && scene_) {
    scene_->addItem(item_);
  }
}

void DeleteAction::redo() {
  if (item_ && scene_) {
    scene_->removeItem(item_);
  }
}

// MoveAction implementation
MoveAction::MoveAction(QGraphicsItem *item, const QPointF &oldPos,
                       const QPointF &newPos)
    : item_(item), oldPos_(oldPos), newPos_(newPos) {}

MoveAction::~MoveAction() = default;

void MoveAction::undo() {
  if (item_) {
    item_->setPos(oldPos_);
  }
}

void MoveAction::redo() {
  if (item_) {
    item_->setPos(newPos_);
  }
}

// CompositeAction implementation
CompositeAction::CompositeAction() = default;

CompositeAction::~CompositeAction() = default;

void CompositeAction::addAction(std::unique_ptr<Action> action) {
  actions_.push_back(std::move(action));
}

void CompositeAction::undo() {
  // Undo in reverse order
  for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
    (*it)->undo();
  }
}

void CompositeAction::redo() {
  // Redo in forward order
  for (auto &action : actions_) {
    action->redo();
  }
}

// FillAction implementation
FillAction::FillAction(QGraphicsItem *item, const QBrush &oldBrush, const QBrush &newBrush)
    : item_(item), oldBrush_(oldBrush), newBrush_(newBrush) {}

FillAction::~FillAction() = default;

void FillAction::undo() {
  if (!item_) return;
  
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item_)) {
    rect->setBrush(oldBrush_);
  } else if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item_)) {
    ellipse->setBrush(oldBrush_);
  } else if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item_)) {
    polygon->setBrush(oldBrush_);
  }
}

void FillAction::redo() {
  if (!item_) return;
  
  if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item_)) {
    rect->setBrush(newBrush_);
  } else if (auto *ellipse = dynamic_cast<QGraphicsEllipseItem *>(item_)) {
    ellipse->setBrush(newBrush_);
  } else if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item_)) {
    polygon->setBrush(newBrush_);
  }
}
