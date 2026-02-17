/**
 * @file scene_controller.h
 * @brief Centralized controller for all scene mutations.
 *
 * SceneController is the single entry point for add/remove/move/modify
 * operations on the graphics scene. It ensures:
 * - All items are properly registered with ItemStore
 * - Deletions are deferred to prevent use-after-free
 * - Undo/redo operations go through a consistent path
 */
#ifndef SCENE_CONTROLLER_H
#define SCENE_CONTROLLER_H

#include "item_id.h"
#include "item_ref.h"
#include "item_store.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QObject>
#include <QPointF>
#include <QTransform>
#include <functional>
#include <memory>
#include <vector>

class Action;
class Layer;
class LayerManager;

/**
 * @brief Callback type for item lifecycle events.
 */
using ItemCallback = std::function<void(const ItemId &, QGraphicsItem *)>;

/**
 * @brief Central controller for all scene modifications.
 *
 * SceneController enforces the invariant that all scene mutations go through
 * a single path, ensuring items are properly tracked and deletions are safe.
 *
 * Key responsibilities:
 * - Adding items to the scene (via ItemStore registration)
 * - Removing items from the scene (via deferred deletion)
 * - Moving and transforming items
 * - Coordinating with undo/redo system
 * - Managing layer assignments
 */
class SceneController : public QObject {
  Q_OBJECT

public:
  explicit SceneController(QGraphicsScene *scene, QObject *parent = nullptr);
  ~SceneController() override;

  /**
   * @brief Get the ItemStore managed by this controller
   * @return Pointer to the ItemStore
   */
  ItemStore *itemStore() const { return itemStore_; }

  /**
   * @brief Get the associated scene
   * @return Pointer to the QGraphicsScene
   */
  QGraphicsScene *scene() const { return scene_; }

  /**
   * @brief Set the layer manager for layer-aware operations
   * @param layerManager Pointer to the LayerManager
   */
  void setLayerManager(LayerManager *layerManager);

  // ========== Item Creation ==========

  /**
   * @brief Add an item to the scene
   * @param item The item to add (caller retains ownership until registered)
   * @return The assigned ItemId
   *
   * This registers the item with ItemStore, adds it to the scene,
   * and optionally assigns it to the active layer.
   */
  ItemId addItem(QGraphicsItem *item);

  /**
   * @brief Add an item to the scene with a specific layer
   * @param item The item to add
   * @param layer The layer to add to (nullptr for no layer tracking)
   * @return The assigned ItemId
   */
  ItemId addItem(QGraphicsItem *item, Layer *layer);

  // ========== Item Removal ==========

  /**
   * @brief Remove an item from the scene
   * @param id The ItemId of the item to remove
   * @param keepForUndo If true, keep snapshot for undo
   * @return true if the item was found and removed
   *
   * The item is removed from the scene immediately but deletion
   * is deferred until the next safe point.
   */
  bool removeItem(const ItemId &id, bool keepForUndo = true);

  /**
   * @brief Remove an item from the scene (by pointer lookup)
   * @param item The item to remove
   * @param keepForUndo If true, keep snapshot for undo
   * @return true if the item was found and removed
   */
  bool removeItem(QGraphicsItem *item, bool keepForUndo = true);

  /**
   * @brief Restore a previously removed item
   * @param id The ItemId of the item to restore
   * @return true if the item was successfully restored
   */
  bool restoreItem(const ItemId &id);

  // ========== Item Access ==========

  /**
   * @brief Get an item by its ID
   * @param id The ItemId to look up
   * @return Pointer to the item, or nullptr if not found
   */
  QGraphicsItem *item(const ItemId &id) const;

  /**
   * @brief Create an ItemRef for safe access
   * @param id The ItemId to reference
   * @return An ItemRef that resolves at use time
   */
  ItemRef ref(const ItemId &id) const;

  /**
   * @brief Get the ItemId for a given item
   * @param item The item to look up
   * @return The ItemId, or a null ItemId if not found
   */
  ItemId idForItem(QGraphicsItem *item) const;

  // ========== Item Modification ==========

  /**
   * @brief Move an item to a new position
   * @param id The ItemId of the item to move
   * @param newPos The new position
   * @return true if the item was found and moved
   */
  bool moveItem(const ItemId &id, const QPointF &newPos);

  /**
   * @brief Apply a transform to an item
   * @param id The ItemId of the item to transform
   * @param transform The transform to apply
   * @return true if the item was found and transformed
   */
  bool transformItem(const ItemId &id, const QTransform &transform);

  /**
   * @brief Scale all items in a layer around its center
   * @param layer The layer whose items should be scaled
   * @param sx Horizontal scale factor
   * @param sy Vertical scale factor
   * @return The number of items that were scaled
   */
  int scaleLayer(Layer *layer, qreal sx, qreal sy);

  // ========== Deferred Deletion ==========

  /**
   * @brief Flush all pending deletions
   *
   * Call this at a safe point (e.g., after event handling) to
   * actually delete items that were scheduled for removal.
   */
  void flushDeletions();

  /**
   * @brief Schedule a flush at the end of the current event
   *
   * Uses Qt's event loop to defer deletion to a safe point.
   */
  void scheduleDeletionFlush();

  // ========== Bulk Operations ==========

  /**
   * @brief Clear all items from the scene
   */
  void clearAll();

  /**
   * @brief Get all registered ItemIds
   * @return Vector of all active ItemIds
   */
  std::vector<ItemId> allItemIds() const;

signals:
  /**
   * @brief Emitted when an item is added to the scene
   * @param id The ItemId of the added item
   */
  void itemAdded(const ItemId &id);

  /**
   * @brief Emitted when an item is removed from the scene
   * @param id The ItemId of the removed item
   */
  void itemRemoved(const ItemId &id);

  /**
   * @brief Emitted when an item is modified
   * @param id The ItemId of the modified item
   */
  void itemModified(const ItemId &id);

  /**
   * @brief Emitted when an item is restored
   * @param id The ItemId of the restored item
   */
  void itemRestored(const ItemId &id);

private:
  QGraphicsScene *scene_;
  ItemStore *itemStore_;
  LayerManager *layerManager_;
  bool deletionFlushScheduled_;

  void doFlushDeletions();
};

#endif // SCENE_CONTROLLER_H
