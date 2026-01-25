/**
 * @file layer.cpp
 * @brief Implementation of the Layer system.
 */
#include "layer.h"
#include "item_store.h"
#include "scene_controller.h"
#include <algorithm>

// Layer implementation
Layer::Layer(const QString &name, Type type)
    : id_(QUuid::createUuid()), name_(name), type_(type), visible_(true),
      locked_(false), opacity_(1.0), itemStore_(nullptr) {}

Layer::~Layer() = default;

Layer::Layer(Layer &&other) noexcept
    : id_(other.id_), name_(std::move(other.name_)), type_(other.type_),
      visible_(other.visible_), locked_(other.locked_), opacity_(other.opacity_),
      itemIds_(std::move(other.itemIds_)), itemStore_(other.itemStore_) {}

Layer &Layer::operator=(Layer &&other) noexcept {
  if (this != &other) {
    id_ = other.id_;
    name_ = std::move(other.name_);
    type_ = other.type_;
    visible_ = other.visible_;
    locked_ = other.locked_;
    opacity_ = other.opacity_;
    itemIds_ = std::move(other.itemIds_);
    itemStore_ = other.itemStore_;
  }
  return *this;
}

void Layer::setVisible(bool visible) {
  if (visible_ != visible) {
    visible_ = visible;
    updateItemsVisibility();
  }
}

void Layer::setOpacity(qreal opacity) {
  opacity_ = qBound(0.0, opacity, 1.0);
  updateItemsOpacity();
}

void Layer::addItem(QGraphicsItem *item) {
  if (!item) {
    return;
  }
  
  // Must have ItemStore to add items safely
  if (!itemStore_) {
    return;
  }
  
  ItemId id = itemStore_->idForItem(item);
  if (id.isValid() && !itemIds_.contains(id)) {
    itemIds_.append(id);
    item->setVisible(visible_);
    item->setOpacity(opacity_);
  }
}

void Layer::addItem(const ItemId &id, ItemStore *store) {
  if (!id.isValid()) {
    return;
  }
  
  if (!itemIds_.contains(id)) {
    itemIds_.append(id);
  }
  
  // Apply layer properties to item
  QGraphicsItem *item = store ? store->item(id) : nullptr;
  if (item) {
    item->setVisible(visible_);
    item->setOpacity(opacity_);
  }
}

bool Layer::removeItem(QGraphicsItem *item) {
  if (!item || !itemStore_) {
    return false;
  }
  
  ItemId id = itemStore_->idForItem(item);
  if (id.isValid()) {
    return itemIds_.removeOne(id);
  }
  return false;
}

bool Layer::removeItem(const ItemId &id) {
  return itemIds_.removeOne(id);
}

QList<QGraphicsItem *> Layer::items() const {
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

bool Layer::containsItem(QGraphicsItem *item) const {
  if (!item || !itemStore_) {
    return false;
  }
  
  ItemId id = itemStore_->idForItem(item);
  return id.isValid() && itemIds_.contains(id);
}

bool Layer::containsItem(const ItemId &id) const {
  return itemIds_.contains(id);
}

void Layer::clear() {
  itemIds_.clear();
}

void Layer::updateItemsVisibility() {
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
    item->setVisible(visible_);
  }
}

void Layer::updateItemsOpacity() {
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
    item->setOpacity(opacity_);
  }
}

// LayerManager implementation
LayerManager::LayerManager(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), scene_(scene), itemStore_(nullptr), sceneController_(nullptr),
      activeLayerIndex_(-1) {
  // Create default layer
  createLayer("Background", Layer::Type::Vector);
}

LayerManager::~LayerManager() = default;

Layer *LayerManager::createLayer(const QString &name, Layer::Type type) {
  auto layer = std::make_unique<Layer>(name, type);
  if (itemStore_) {
    layer->setItemStore(itemStore_);
  }
  Layer *ptr = layer.get();
  layers_.push_back(std::move(layer));
  
  // Set as active if first layer
  if (activeLayerIndex_ < 0) {
    activeLayerIndex_ = 0;
  }
  
  updateLayerZOrder();
  emit layerAdded(ptr);
  return ptr;
}

void LayerManager::setItemStore(ItemStore *store) {
  itemStore_ = store;
  for (auto &layer : layers_) {
    if (layer) {
      layer->setItemStore(store);
    }
  }

  // Connect to itemAboutToBeDeleted to remove stale ItemIds from layers
  if (store) {
    connect(store, &ItemStore::itemAboutToBeDeleted, this,
            [this](const ItemId &id) {
              for (auto &layer : layers_) {
                if (layer) {
                  layer->removeItem(id);
                }
              }
            });
  }
}

bool LayerManager::deleteLayer(int index) {
  if (index < 0 || index >= static_cast<int>(layers_.size())) {
    return false;
  }
  
  // Don't delete the last layer
  if (layers_.size() <= 1) {
    return false;
  }
  
  Layer *layer = layers_[index].get();
  emit layerRemoved(layer);
  
  // Remove items from scene and delete them via controller/store
  const QList<ItemId> ids = layer->itemIds();
  if (sceneController_) {
    for (const ItemId &id : ids) {
      sceneController_->removeItem(id, false);
    }
  } else if (itemStore_) {
    for (const ItemId &id : ids) {
      itemStore_->scheduleDelete(id);
    }
    itemStore_->flushDeletions();
  } else {
    for (auto *item : layer->items()) {
      if (item && item->scene() == scene_) {
        scene_->removeItem(item);
        delete item;
      }
    }
  }
  
  layers_.erase(layers_.begin() + index);
  
  // Adjust active layer index
  if (activeLayerIndex_ >= static_cast<int>(layers_.size())) {
    activeLayerIndex_ = static_cast<int>(layers_.size()) - 1;
  }
  
  updateLayerZOrder();
  emit activeLayerChanged(activeLayer());
  return true;
}

bool LayerManager::deleteLayer(const QUuid &id) {
  for (size_t i = 0; i < layers_.size(); ++i) {
    if (layers_[i]->id() == id) {
      return deleteLayer(static_cast<int>(i));
    }
  }
  return false;
}

Layer *LayerManager::layer(int index) const {
  if (index < 0 || index >= static_cast<int>(layers_.size())) {
    return nullptr;
  }
  return layers_[index].get();
}

Layer *LayerManager::layer(const QUuid &id) const {
  for (const auto &layer : layers_) {
    if (layer->id() == id) {
      return layer.get();
    }
  }
  return nullptr;
}

Layer *LayerManager::activeLayer() const {
  return layer(activeLayerIndex_);
}

void LayerManager::setActiveLayer(int index) {
  if (index >= 0 && index < static_cast<int>(layers_.size())) {
    activeLayerIndex_ = index;
    emit activeLayerChanged(activeLayer());
  }
}

void LayerManager::setActiveLayer(const QUuid &id) {
  for (size_t i = 0; i < layers_.size(); ++i) {
    if (layers_[i]->id() == id) {
      setActiveLayer(static_cast<int>(i));
      return;
    }
  }
}

int LayerManager::activeLayerIndex() const {
  return activeLayerIndex_;
}

bool LayerManager::moveLayerUp(int index) {
  if (index <= 0 || index >= static_cast<int>(layers_.size())) {
    return false;
  }
  
  std::swap(layers_[index], layers_[index - 1]);
  
  // Adjust active layer index if needed
  if (activeLayerIndex_ == index) {
    activeLayerIndex_ = index - 1;
  } else if (activeLayerIndex_ == index - 1) {
    activeLayerIndex_ = index;
  }
  
  updateLayerZOrder();
  emit layerOrderChanged();
  return true;
}

bool LayerManager::moveLayerDown(int index) {
  if (index < 0 || index >= static_cast<int>(layers_.size()) - 1) {
    return false;
  }
  
  std::swap(layers_[index], layers_[index + 1]);
  
  // Adjust active layer index if needed
  if (activeLayerIndex_ == index) {
    activeLayerIndex_ = index + 1;
  } else if (activeLayerIndex_ == index + 1) {
    activeLayerIndex_ = index;
  }
  
  updateLayerZOrder();
  emit layerOrderChanged();
  return true;
}

Layer *LayerManager::findLayerForItem(QGraphicsItem *item) const {
  if (itemStore_) {
    ItemId id = itemStore_->idForItem(item);
    if (id.isValid()) {
      for (const auto &layer : layers_) {
        if (layer && layer->containsItem(id)) {
          return layer.get();
        }
      }
      return nullptr;
    }
  }
  for (const auto &layer : layers_) {
    if (layer->containsItem(item)) {
      return layer.get();
    }
  }
  return nullptr;
}

void LayerManager::addItemToActiveLayer(QGraphicsItem *item) {
  Layer *active = activeLayer();
  if (active && item) {
    if (itemStore_) {
      ItemId id = itemStore_->idForItem(item);
      if (id.isValid()) {
        active->addItem(id, itemStore_);
        return;
      }
    }
    active->addItem(item);
  }
}

bool LayerManager::mergeDown(int index) {
  if (index <= 0 || index >= static_cast<int>(layers_.size())) {
    return false;
  }
  
  Layer *source = layers_[index].get();
  Layer *target = layers_[index - 1].get();
  
  // Move all items from source to target
  for (const ItemId &id : source->itemIds()) {
    target->addItem(id, itemStore_);
  }
  source->clear();
  
  // Delete the source layer
  return deleteLayer(index);
}

Layer *LayerManager::flattenAll() {
  if (layers_.empty()) {
    return nullptr;
  }
  
  // Move all items to the first layer
  for (size_t i = 1; i < layers_.size(); ++i) {
    for (const ItemId &id : layers_[i]->itemIds()) {
      layers_[0]->addItem(id, itemStore_);
    }
    layers_[i]->clear();
  }
  
  // Remove all layers except the first
  while (layers_.size() > 1) {
    emit layerRemoved(layers_.back().get());
    layers_.pop_back();
  }
  
  activeLayerIndex_ = 0;
  layers_[0]->setName("Flattened");
  
  updateLayerZOrder();
  emit layerOrderChanged();
  emit activeLayerChanged(activeLayer());
  
  return layers_[0].get();
}

Layer *LayerManager::duplicateLayer(int index) {
  if (index < 0 || index >= static_cast<int>(layers_.size())) {
    return nullptr;
  }
  
  Layer *source = layers_[index].get();
  QString newName = source->name() + " (Copy)";
  Layer *newLayer = createLayer(newName, source->type());
  
  if (newLayer) {
    newLayer->setVisible(source->isVisible());
    newLayer->setLocked(source->isLocked());
    newLayer->setOpacity(source->opacity());
    // Note: Items are not duplicated, only the layer properties
  }
  
  return newLayer;
}

void LayerManager::clear() {
  for (auto &layer : layers_) {
    emit layerRemoved(layer.get());
    const QList<ItemId> ids = layer->itemIds();
    if (sceneController_) {
      for (const ItemId &id : ids) {
        sceneController_->removeItem(id, false);
      }
    } else if (itemStore_) {
      for (const ItemId &id : ids) {
        itemStore_->scheduleDelete(id);
      }
      itemStore_->flushDeletions();
    } else {
      for (auto *item : layer->items()) {
        if (item && item->scene() == scene_) {
          scene_->removeItem(item);
        }
      }
    }
  }
  layers_.clear();
  activeLayerIndex_ = -1;
  
  // Recreate default layer
  createLayer("Background", Layer::Type::Vector);
}

void LayerManager::updateLayerZOrder() {
  // Set Z-value based on layer order (first layer = bottom)
  qreal zBase = 0.0;
  for (size_t i = 0; i < layers_.size(); ++i) {
    qreal layerZ = zBase + static_cast<qreal>(i) * 1000.0;
    if (itemStore_) {
      for (const ItemId &id : layers_[i]->itemIds()) {
        if (QGraphicsItem *item = itemStore_->item(id)) {
          item->setZValue(layerZ);
        }
      }
    } else {
      for (auto *item : layers_[i]->items()) {
        // Check if item is still valid before accessing
        if (item && item->scene()) {
          item->setZValue(layerZ);
        }
      }
    }
  }
}
