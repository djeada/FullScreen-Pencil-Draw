/**
 * @file pdf_overlay.cpp
 * @brief Implementation of PDF overlay management.
 */
#include "pdf_overlay.h"
#include "item_store.h"

#ifdef HAVE_QT_PDF

// --- PdfPageOverlay Implementation ---

PdfPageOverlay::PdfPageOverlay() : visible_(true), itemStore_(nullptr) {}

PdfPageOverlay::~PdfPageOverlay() { clear(); }

PdfPageOverlay::PdfPageOverlay(PdfPageOverlay &&other) noexcept
    : items_(std::move(other.items_)), itemIds_(std::move(other.itemIds_)),
      visible_(other.visible_), itemStore_(other.itemStore_) {}

PdfPageOverlay &PdfPageOverlay::operator=(PdfPageOverlay &&other) noexcept {
  if (this != &other) {
    items_ = std::move(other.items_);
    itemIds_ = std::move(other.itemIds_);
    visible_ = other.visible_;
    itemStore_ = other.itemStore_;
  }
  return *this;
}

void PdfPageOverlay::addItem(QGraphicsItem *item) {
  if (item && !items_.contains(item)) {
    items_.append(item);
    
    // Also track by ItemId if we have an ItemStore
    if (itemStore_) {
      ItemId id = itemStore_->idForItem(item);
      if (id.isValid() && !itemIds_.contains(id)) {
        itemIds_.append(id);
      }
    }
    
    item->setVisible(visible_);
  }
}

void PdfPageOverlay::addItem(const ItemId &id, ItemStore *store) {
  if (!id.isValid()) {
    return;
  }
  
  if (!itemIds_.contains(id)) {
    itemIds_.append(id);
  }
  
  // Also maintain backwards-compatible raw pointer list
  QGraphicsItem *item = store ? store->item(id) : nullptr;
  if (item && !items_.contains(item)) {
    items_.append(item);
    item->setVisible(visible_);
  }
}

bool PdfPageOverlay::removeItem(QGraphicsItem *item) {
  bool removed = items_.removeOne(item);
  
  // Also remove by ItemId if we have an ItemStore
  if (itemStore_) {
    ItemId id = itemStore_->idForItem(item);
    if (id.isValid()) {
      itemIds_.removeOne(id);
    }
  }
  
  return removed;
}

bool PdfPageOverlay::removeItem(const ItemId &id) {
  bool removed = itemIds_.removeOne(id);
  
  // Also remove from backwards-compatible raw pointer list
  if (itemStore_) {
    QGraphicsItem *item = itemStore_->item(id);
    if (item) {
      items_.removeOne(item);
    }
  }
  
  return removed;
}

bool PdfPageOverlay::containsItem(QGraphicsItem *item) const {
  return items_.contains(item);
}

bool PdfPageOverlay::containsItem(const ItemId &id) const {
  return itemIds_.contains(id);
}

void PdfPageOverlay::clear() {
  items_.clear();
  itemIds_.clear();
}

void PdfPageOverlay::setVisible(bool visible) {
  visible_ = visible;
  for (QGraphicsItem *item : items_) {
    if (item) {
      item->setVisible(visible);
    }
  }
}

// --- PdfOverlayManager Implementation ---

PdfOverlayManager::PdfOverlayManager(QObject *parent)
    : QObject(parent), currentPage_(-1) {}

PdfOverlayManager::~PdfOverlayManager() { clear(); }

void PdfOverlayManager::initialize(int pageCount) {
  clear();
  overlays_.reserve(pageCount);
  undoStacks_.resize(pageCount);
  redoStacks_.resize(pageCount);
  for (int i = 0; i < pageCount; ++i) {
    overlays_.push_back(std::make_unique<PdfPageOverlay>());
  }
  currentPage_ = 0;
}

PdfPageOverlay *PdfOverlayManager::overlay(int pageIndex) {
  if (pageIndex < 0 || pageIndex >= static_cast<int>(overlays_.size())) {
    return nullptr;
  }
  return overlays_[pageIndex].get();
}

const PdfPageOverlay *PdfOverlayManager::overlay(int pageIndex) const {
  if (pageIndex < 0 || pageIndex >= static_cast<int>(overlays_.size())) {
    return nullptr;
  }
  return overlays_[pageIndex].get();
}

void PdfOverlayManager::addItemToPage(int pageIndex, QGraphicsItem *item) {
  if (auto *ov = overlay(pageIndex)) {
    ov->addItem(item);
    emit overlayModified(pageIndex);
  }
}

bool PdfOverlayManager::removeItemFromPage(int pageIndex,
                                           QGraphicsItem *item) {
  if (auto *ov = overlay(pageIndex)) {
    if (ov->removeItem(item)) {
      emit overlayModified(pageIndex);
      return true;
    }
  }
  return false;
}

int PdfOverlayManager::findPageForItem(QGraphicsItem *item) const {
  for (int i = 0; i < static_cast<int>(overlays_.size()); ++i) {
    if (overlays_[i] && overlays_[i]->containsItem(item)) {
      return i;
    }
  }
  return -1;
}

void PdfOverlayManager::clear() {
  overlays_.clear();
  undoStacks_.clear();
  redoStacks_.clear();
  currentPage_ = -1;
}

void PdfOverlayManager::showPage(int pageIndex) {
  if (pageIndex < 0 || pageIndex >= static_cast<int>(overlays_.size())) {
    return;
  }

  // Hide all overlays except the current one
  for (int i = 0; i < static_cast<int>(overlays_.size()); ++i) {
    if (overlays_[i]) {
      overlays_[i]->setVisible(i == pageIndex);
    }
  }
  currentPage_ = pageIndex;
}

std::vector<std::unique_ptr<Action>> &
PdfOverlayManager::undoStack(int pageIndex) {
  static std::vector<std::unique_ptr<Action>> empty;
  if (pageIndex < 0 || pageIndex >= static_cast<int>(undoStacks_.size())) {
    return empty;
  }
  return undoStacks_[pageIndex];
}

std::vector<std::unique_ptr<Action>> &
PdfOverlayManager::redoStack(int pageIndex) {
  static std::vector<std::unique_ptr<Action>> empty;
  if (pageIndex < 0 || pageIndex >= static_cast<int>(redoStacks_.size())) {
    return empty;
  }
  return redoStacks_[pageIndex];
}

bool PdfOverlayManager::canUndo(int pageIndex) const {
  if (pageIndex < 0 || pageIndex >= static_cast<int>(undoStacks_.size())) {
    return false;
  }
  return !undoStacks_[pageIndex].empty();
}

bool PdfOverlayManager::canRedo(int pageIndex) const {
  if (pageIndex < 0 || pageIndex >= static_cast<int>(redoStacks_.size())) {
    return false;
  }
  return !redoStacks_[pageIndex].empty();
}

#endif // HAVE_QT_PDF
