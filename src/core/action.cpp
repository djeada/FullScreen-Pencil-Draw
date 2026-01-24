/**
 * @file action.cpp
 * @brief Implementation of the undo/redo action system.
 */
#include "action.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <utility>

Action::~Action() = default;

// DrawAction implementation
DrawAction::DrawAction(QGraphicsItem *item, QGraphicsScene *scene,
                       ItemCallback onAdd, ItemCallback onRemove)
    : item_(item), scene_(scene), itemOwnedByAction_(false),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {}

DrawAction::~DrawAction() {
  // Clean up the item if we own it and it still exists
  if (itemOwnedByAction_ && item_) {
    delete item_;
    item_ = nullptr;
  }
}

void DrawAction::undo() {
  if (item_ && scene_) {
    scene_->removeItem(item_);
    if (onRemove_) {
      onRemove_(item_);
    }
    itemOwnedByAction_ = true;  // We now own the item
  }
}

void DrawAction::redo() {
  if (item_ && scene_) {
    scene_->addItem(item_);
    if (onAdd_) {
      onAdd_(item_);
    }
    itemOwnedByAction_ = false;  // Scene now owns the item
  }
}

// DeleteAction implementation
DeleteAction::DeleteAction(QGraphicsItem *item, QGraphicsScene *scene,
                           ItemCallback onAdd, ItemCallback onRemove)
    : item_(item), scene_(scene), itemOwnedByAction_(true),
      onAdd_(std::move(onAdd)), onRemove_(std::move(onRemove)) {}

DeleteAction::~DeleteAction() {
  // Clean up the item if we own it and it still exists
  if (itemOwnedByAction_ && item_) {
    delete item_;
    item_ = nullptr;
  }
}

void DeleteAction::undo() {
  if (item_ && scene_) {
    scene_->addItem(item_);
    if (onAdd_) {
      onAdd_(item_);
    }
    itemOwnedByAction_ = false;  // Scene now owns the item
  }
}

void DeleteAction::redo() {
  if (item_ && scene_) {
    scene_->removeItem(item_);
    if (onRemove_) {
      onRemove_(item_);
    }
    itemOwnedByAction_ = true;  // We now own the item
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
