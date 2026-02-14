/**
 * @file scene_controller.cpp
 * @brief Implementation of the SceneController class.
 */
#include "scene_controller.h"
#include "layer.h"
#include <QCoreApplication>
#include <QTimer>

SceneController::SceneController(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), scene_(scene), itemStore_(new ItemStore(scene, this)),
      layerManager_(nullptr), deletionFlushScheduled_(false) {}

SceneController::~SceneController() {
  // ItemStore is owned by this, will be deleted automatically
}

void SceneController::setLayerManager(LayerManager *layerManager) {
  layerManager_ = layerManager;
  if (layerManager_) {
    layerManager_->setItemStore(itemStore_);
    layerManager_->setSceneController(this);
  }
}

ItemId SceneController::addItem(QGraphicsItem *item) {
  return addItem(item, nullptr);
}

ItemId SceneController::addItem(QGraphicsItem *item, Layer *layer) {
  if (!item) {
    return ItemId();
  }

  // Register with ItemStore (this also adds to scene)
  ItemId id = itemStore_->registerItem(item);

  // Add to layer if specified
  if (layer) {
    layer->addItem(item);
  } else if (layerManager_) {
    layerManager_->addItemToActiveLayer(item);
  }

  emit itemAdded(id);
  return id;
}

bool SceneController::removeItem(const ItemId &id, bool keepForUndo) {
  if (!id.isValid()) {
    return false;
  }

  QGraphicsItem *itemPtr = itemStore_->item(id);
  if (!itemPtr) {
    return false;
  }

  // Remove from layer
  if (layerManager_) {
    Layer *layer = layerManager_->findLayerForItem(itemPtr);
    if (layer) {
      layer->removeItem(itemPtr);
    }
  }

  // Schedule deletion
  itemStore_->scheduleDelete(id, keepForUndo);

  // Schedule flush for later
  scheduleDeletionFlush();

  emit itemRemoved(id);
  return true;
}

bool SceneController::removeItem(QGraphicsItem *item, bool keepForUndo) {
  if (!item) {
    return false;
  }

  ItemId id = itemStore_->idForItem(item);
  return removeItem(id, keepForUndo);
}

bool SceneController::restoreItem(const ItemId &id) {
  if (!id.isValid()) {
    return false;
  }

  if (itemStore_->restoreItem(id)) {
    emit itemRestored(id);
    return true;
  }

  return false;
}

QGraphicsItem *SceneController::item(const ItemId &id) const {
  return itemStore_->item(id);
}

ItemRef SceneController::ref(const ItemId &id) const {
  return ItemRef(itemStore_, id);
}

ItemId SceneController::idForItem(QGraphicsItem *item) const {
  return itemStore_->idForItem(item);
}

bool SceneController::moveItem(const ItemId &id, const QPointF &newPos) {
  QGraphicsItem *itemPtr = itemStore_->item(id);
  if (!itemPtr) {
    return false;
  }

  itemPtr->setPos(newPos);
  emit itemModified(id);
  return true;
}

bool SceneController::transformItem(const ItemId &id,
                                    const QTransform &transform) {
  QGraphicsItem *itemPtr = itemStore_->item(id);
  if (!itemPtr) {
    return false;
  }

  itemPtr->setTransform(transform);
  emit itemModified(id);
  return true;
}

void SceneController::flushDeletions() { doFlushDeletions(); }

void SceneController::scheduleDeletionFlush() {
  if (deletionFlushScheduled_) {
    return;
  }

  deletionFlushScheduled_ = true;

  // Use QTimer::singleShot to defer to next event loop iteration
  QTimer::singleShot(0, this, [this]() {
    doFlushDeletions();
    deletionFlushScheduled_ = false;
  });
}

void SceneController::doFlushDeletions() { itemStore_->flushDeletions(); }

void SceneController::clearAll() {
  itemStore_->clear();
  flushDeletions();
}

std::vector<ItemId> SceneController::allItemIds() const {
  return itemStore_->allItemIds();
}
