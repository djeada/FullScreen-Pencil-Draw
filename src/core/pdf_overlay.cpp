/**
 * @file pdf_overlay.cpp
 * @brief Implementation of PDF overlay management.
 */
#include "pdf_overlay.h"
#include "item_store.h"

#ifdef HAVE_QT_PDF

// --- PdfPageOverlay Implementation ---

PdfPageOverlay::PdfPageOverlay() : itemStore_(nullptr), visible_(true) {}

PdfPageOverlay::~PdfPageOverlay() { clear(); }

PdfPageOverlay::PdfPageOverlay(PdfPageOverlay &&other) noexcept
    : itemIds_(std::move(other.itemIds_)), itemStore_(other.itemStore_),
      visible_(other.visible_) {}

PdfPageOverlay &PdfPageOverlay::operator=(PdfPageOverlay &&other) noexcept {
  if (this != &other) {
    itemIds_ = std::move(other.itemIds_);
    itemStore_ = other.itemStore_;
    visible_ = other.visible_;
  }
  return *this;
}

void PdfPageOverlay::setItemStore(ItemStore *store) { itemStore_ = store; }

void PdfPageOverlay::addItem(QGraphicsItem *item) {
  if (!item || !itemStore_) {
    return;
  }

  ItemId id = itemStore_->idForItem(item);
  if (id.isValid() && !itemIds_.contains(id)) {
    itemIds_.append(id);
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

  // Apply visibility to item
  QGraphicsItem *item = store ? store->item(id) : nullptr;
  if (item) {
    item->setVisible(visible_);
  }
}

bool PdfPageOverlay::removeItem(QGraphicsItem *item) {
  if (!item || !itemStore_) {
    return false;
  }

  ItemId id = itemStore_->idForItem(item);
  if (id.isValid()) {
    return itemIds_.removeOne(id);
  }
  return false;
}

bool PdfPageOverlay::removeItem(const ItemId &id) {
  return itemIds_.removeOne(id);
}

QList<QGraphicsItem *> PdfPageOverlay::items() const {
  QList<QGraphicsItem *> result;
  if (!itemStore_) {
    return result;
  }

  for (const ItemId &id : itemIds_) {
    if (QGraphicsItem *item = itemStore_->item(id)) {
      result.append(item);
    }
  }
  return result;
}

bool PdfPageOverlay::containsItem(QGraphicsItem *item) const {
  if (!item || !itemStore_) {
    return false;
  }

  ItemId id = itemStore_->idForItem(item);
  return id.isValid() && itemIds_.contains(id);
}

bool PdfPageOverlay::containsItem(const ItemId &id) const {
  return itemIds_.contains(id);
}

void PdfPageOverlay::clear() { itemIds_.clear(); }

void PdfPageOverlay::setVisible(bool visible) {
  visible_ = visible;
  if (!itemStore_) {
    return;
  }

  for (int i = itemIds_.size() - 1; i >= 0; --i) {
    const ItemId &id = itemIds_[i];
    QGraphicsItem *item = itemStore_->item(id);
    if (!item) {
      // Item was deleted, remove stale ID
      itemIds_.removeAt(i);
      continue;
    }
    item->setVisible(visible);
  }
}

// --- PdfOverlayManager Implementation ---

PdfOverlayManager::PdfOverlayManager(QObject *parent)
    : QObject(parent), currentPage_(-1), itemStore_(nullptr) {}

PdfOverlayManager::~PdfOverlayManager() { clear(); }

void PdfOverlayManager::setItemStore(ItemStore *store) {
  itemStore_ = store;
  for (auto &overlay : overlays_) {
    if (overlay) {
      overlay->setItemStore(store);
    }
  }

  // Connect to itemAboutToBeDeleted to remove stale ItemIds from overlays
  if (store) {
    connect(store, &ItemStore::itemAboutToBeDeleted, this,
            [this](const ItemId &id) {
              for (auto &overlay : overlays_) {
                if (overlay) {
                  overlay->removeItem(id);
                }
              }
            });
  }
}

void PdfOverlayManager::initialize(int pageCount) {
  clear();
  overlays_.reserve(pageCount);
  undoStacks_.resize(pageCount);
  redoStacks_.resize(pageCount);
  for (int i = 0; i < pageCount; ++i) {
    auto overlay = std::make_unique<PdfPageOverlay>();
    if (itemStore_) {
      overlay->setItemStore(itemStore_);
    }
    overlays_.push_back(std::move(overlay));
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
    if (itemStore_) {
      ItemId id = itemStore_->idForItem(item);
      if (id.isValid()) {
        ov->addItem(id, itemStore_);
      }
    }
    emit overlayModified(pageIndex);
  }
}

bool PdfOverlayManager::removeItemFromPage(int pageIndex, QGraphicsItem *item) {
  if (auto *ov = overlay(pageIndex)) {
    bool removed = false;
    if (itemStore_) {
      ItemId id = itemStore_->idForItem(item);
      if (id.isValid()) {
        removed = ov->removeItem(id);
      }
    }
    if (removed) {
      emit overlayModified(pageIndex);
      return true;
    }
  }
  return false;
}

int PdfOverlayManager::findPageForItem(QGraphicsItem *item) const {
  if (!itemStore_) {
    return -1;
  }

  ItemId id = itemStore_->idForItem(item);
  if (id.isValid()) {
    for (int i = 0; i < static_cast<int>(overlays_.size()); ++i) {
      if (overlays_[i] && overlays_[i]->containsItem(id)) {
        return i;
      }
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
