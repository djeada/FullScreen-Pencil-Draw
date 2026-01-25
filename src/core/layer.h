/**
 * @file layer.h
 * @brief Layer system for managing graphics elements.
 *
 * This file defines the Layer class and LayerManager for organizing
 * graphics into separate, manageable layers. This enables professional
 * workflows with raster and vector graphics mixed together.
 */
#ifndef LAYER_H
#define LAYER_H

#include <QColor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QList>
#include <QObject>
#include <QString>
#include <QUuid>
#include <memory>
#include <vector>

#include "item_id.h"

class ItemStore;

/**
 * @brief Represents a single layer in the canvas.
 *
 * Layers provide a way to organize graphics into separate groups
 * that can be independently shown, hidden, locked, and reordered.
 */
class Layer {
public:
  /**
   * @brief Layer type enumeration
   */
  enum class Type {
    Vector, ///< Vector graphics layer (shapes, paths, text)
    Raster, ///< Raster graphics layer (images, pixels)
    Mixed   ///< Mixed content layer
  };

  /**
   * @brief Construct a new Layer
   * @param name Display name for the layer
   * @param type The type of layer
   */
  explicit Layer(const QString &name = "Layer",
                 Type type = Type::Vector);
  ~Layer();

  // Non-copyable
  Layer(const Layer &) = delete;
  Layer &operator=(const Layer &) = delete;

  // Movable
  Layer(Layer &&) noexcept;
  Layer &operator=(Layer &&) noexcept;

  /**
   * @brief Get the unique identifier for this layer
   * @return The layer's UUID
   */
  QUuid id() const { return id_; }

  /**
   * @brief Get the layer name
   * @return The display name
   */
  QString name() const { return name_; }

  /**
   * @brief Set the layer name
   * @param name New display name
   */
  void setName(const QString &name) { name_ = name; }

  /**
   * @brief Check if the layer is visible
   * @return true if visible
   */
  bool isVisible() const { return visible_; }

  /**
   * @brief Set layer visibility
   * @param visible Whether to show the layer
   */
  void setVisible(bool visible);

  /**
   * @brief Check if the layer is locked
   * @return true if locked (non-editable)
   */
  bool isLocked() const { return locked_; }

  /**
   * @brief Set layer lock state
   * @param locked Whether to lock the layer
   */
  void setLocked(bool locked) { locked_ = locked; }

  /**
   * @brief Get the layer opacity
   * @return Opacity value (0.0 to 1.0)
   */
  qreal opacity() const { return opacity_; }

  /**
   * @brief Set the layer opacity
   * @param opacity New opacity value (0.0 to 1.0)
   */
  void setOpacity(qreal opacity);

  /**
   * @brief Get the layer type
   * @return The layer type
   */
  Type type() const { return type_; }

  /**
   * @brief Add an item to this layer
   * @param item The graphics item to add
   */
  void addItem(QGraphicsItem *item);

  /**
   * @brief Add an item to this layer by ItemId
   * @param id The ItemId of the item to add
   * @param store The ItemStore to resolve the item from
   */
  void addItem(const ItemId &id, ItemStore *store);

  /**
   * @brief Remove an item from this layer
   * @param item The item to remove
   * @return true if the item was found and removed
   */
  bool removeItem(QGraphicsItem *item);

  /**
   * @brief Remove an item from this layer by ItemId
   * @param id The ItemId of the item to remove
   * @return true if the item was found and removed
   */
  bool removeItem(const ItemId &id);

  /**
   * @brief Get all items in this layer
   * @return List of graphics items
   * @deprecated Use itemIds() instead for safer access
   */
  const QList<QGraphicsItem *> &items() const { return items_; }

  /**
   * @brief Get all ItemIds in this layer
   * @return List of ItemIds
   */
  const QList<ItemId> &itemIds() const { return itemIds_; }

  /**
   * @brief Check if an item belongs to this layer
   * @param item The item to check
   * @return true if the item is in this layer
   */
  bool containsItem(QGraphicsItem *item) const;

  /**
   * @brief Check if an item belongs to this layer by ItemId
   * @param id The ItemId to check
   * @return true if the item is in this layer
   */
  bool containsItem(const ItemId &id) const;

  /**
   * @brief Clear all items from the layer
   * @note Items are not deleted, only removed from the layer
   */
  void clear();

  /**
   * @brief Get the number of items in this layer
   * @return Item count
   */
  int itemCount() const { return itemIds_.size(); }

  /**
   * @brief Set the ItemStore for this layer (for ID-based operations)
   * @param store The ItemStore to use
   */
  void setItemStore(ItemStore *store) { itemStore_ = store; }

private:
  QUuid id_;
  QString name_;
  Type type_;
  bool visible_;
  bool locked_;
  qreal opacity_;
  QList<QGraphicsItem *> items_;  // Deprecated: kept for backwards compatibility
  QList<ItemId> itemIds_;         // Primary storage: stable ItemIds
  ItemStore *itemStore_;          // For resolving ItemIds to items

  void updateItemsVisibility();
  void updateItemsOpacity();
};

/**
 * @brief Manages a collection of layers.
 *
 * The LayerManager provides centralized control over all layers,
 * including creation, deletion, reordering, and selection.
 */
class LayerManager : public QObject {
  Q_OBJECT

public:
  explicit LayerManager(QGraphicsScene *scene, QObject *parent = nullptr);
  ~LayerManager() override;

  /**
   * @brief Create a new layer
   * @param name The layer name
   * @param type The layer type
   * @return Pointer to the new layer
   */
  Layer *createLayer(const QString &name = "New Layer",
                     Layer::Type type = Layer::Type::Vector);

  /**
   * @brief Delete a layer by index
   * @param index The layer index
   * @return true if successfully deleted
   */
  bool deleteLayer(int index);

  /**
   * @brief Delete a layer by ID
   * @param id The layer UUID
   * @return true if successfully deleted
   */
  bool deleteLayer(const QUuid &id);

  /**
   * @brief Get a layer by index
   * @param index The layer index
   * @return Pointer to the layer, or nullptr
   */
  Layer *layer(int index) const;

  /**
   * @brief Get a layer by ID
   * @param id The layer UUID
   * @return Pointer to the layer, or nullptr
   */
  Layer *layer(const QUuid &id) const;

  /**
   * @brief Get the active (selected) layer
   * @return Pointer to the active layer
   */
  Layer *activeLayer() const;

  /**
   * @brief Set the active layer by index
   * @param index The layer index
   */
  void setActiveLayer(int index);

  /**
   * @brief Set the active layer by ID
   * @param id The layer UUID
   */
  void setActiveLayer(const QUuid &id);

  /**
   * @brief Get the index of the active layer
   * @return The active layer index, or -1
   */
  int activeLayerIndex() const;

  /**
   * @brief Get the total number of layers
   * @return Layer count
   */
  int layerCount() const { return static_cast<int>(layers_.size()); }

  /**
   * @brief Move a layer up in the stack
   * @param index Current layer index
   * @return true if moved successfully
   */
  bool moveLayerUp(int index);

  /**
   * @brief Move a layer down in the stack
   * @param index Current layer index
   * @return true if moved successfully
   */
  bool moveLayerDown(int index);

  /**
   * @brief Find which layer contains a specific item
   * @param item The graphics item
   * @return Pointer to the containing layer, or nullptr
   */
  Layer *findLayerForItem(QGraphicsItem *item) const;

  /**
   * @brief Add an item to the active layer
   * @param item The item to add
   */
  void addItemToActiveLayer(QGraphicsItem *item);

  /**
   * @brief Merge a layer with the one below it
   * @param index The layer index to merge down
   * @return true if merged successfully
   */
  bool mergeDown(int index);

  /**
   * @brief Flatten all layers into one
   * @return Pointer to the resulting layer
   */
  Layer *flattenAll();

  /**
   * @brief Duplicate a layer
   * @param index The layer index to duplicate
   * @return Pointer to the new layer, or nullptr
   */
  Layer *duplicateLayer(int index);

  /**
   * @brief Clear all layers
   */
  void clear();

signals:
  /**
   * @brief Emitted when a layer is added
   * @param layer Pointer to the new layer
   */
  void layerAdded(Layer *layer);

  /**
   * @brief Emitted when a layer is about to be removed
   * @param layer Pointer to the layer being removed
   */
  void layerRemoved(Layer *layer);

  /**
   * @brief Emitted when the active layer changes
   * @param layer Pointer to the new active layer
   */
  void activeLayerChanged(Layer *layer);

  /**
   * @brief Emitted when a layer's properties change
   * @param layer Pointer to the modified layer
   */
  void layerChanged(Layer *layer);

  /**
   * @brief Emitted when layer order changes
   */
  void layerOrderChanged();

private:
  QGraphicsScene *scene_;
  std::vector<std::unique_ptr<Layer>> layers_;
  int activeLayerIndex_;

  void updateLayerZOrder();
};

#endif // LAYER_H
