/**
 * @file item_store.h
 * @brief Central ownership and lifecycle management for all graphics items.
 *
 * ItemStore is the single source of truth for item lifetimes. It provides:
 * - Unique ItemId assignment for every item
 * - Centralized creation and destruction of items
 * - Deferred deletion to prevent use-after-free during event handling
 * - Safe lookup of items by their stable ItemId
 */
#ifndef ITEM_STORE_H
#define ITEM_STORE_H

#include "item_id.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QObject>
#include <QPointer>
#include <memory>
#include <unordered_map>
#include <vector>

/**
 * @brief Central registry and owner of all graphics items.
 *
 * ItemStore implements the single source of truth pattern for item lifecycle:
 * - All items are registered with the store upon creation
 * - Items are looked up by stable ItemId, never cached as raw pointers
 * - Deletion is deferred to a safe point (not during paint/event handling)
 *
 * @note Only SceneController should call ItemStore mutation methods directly.
 * Other subsystems should use SceneController's API.
 */
class ItemStore : public QObject {
  Q_OBJECT

public:
  explicit ItemStore(QGraphicsScene *scene, QObject *parent = nullptr);
  ~ItemStore() override;

  /**
   * @brief Register an existing item with the store
   * @param item The item to register (ownership is NOT transferred)
   * @return The assigned ItemId
   *
   * @note The item is added to the scene if not already present.
   * The store does not take ownership; it only tracks the item.
   */
  ItemId registerItem(QGraphicsItem *item);

  /**
   * @brief Unregister an item from the store
   * @param id The ItemId to unregister
   * @return The item pointer (or nullptr if not found)
   *
   * @note This removes the item from tracking but does NOT delete it.
   * Use scheduleDelete() for deferred deletion.
   */
  QGraphicsItem *unregisterItem(const ItemId &id);

  /**
   * @brief Look up an item by its ItemId
   * @param id The ItemId to look up
   * @return Pointer to the item, or nullptr if not found or deleted
   */
  QGraphicsItem *item(const ItemId &id) const;

  /**
   * @brief Check if an item exists and is valid
   * @param id The ItemId to check
   * @return true if the item exists and is valid
   */
  bool contains(const ItemId &id) const;

  /**
   * @brief Get the ItemId for a given item
   * @param item The item to look up
   * @return The ItemId, or a null ItemId if not found
   */
  ItemId idForItem(QGraphicsItem *item) const;

  /**
   * @brief Schedule an item for deferred deletion
   * @param id The ItemId of the item to delete
   *
   * @note The item is removed from the scene immediately but not
   * destroyed until flushDeletions() is called.
   */
  void scheduleDelete(const ItemId &id);

  /**
   * @brief Schedule an item for deferred deletion, keeping a snapshot
   * @param id The ItemId of the item to delete
   * @param keepSnapshot If true, the item is kept for potential undo
   *
   * @note When keepSnapshot is true, the item is removed from the scene
   * but its memory is preserved for undo operations.
   */
  void scheduleDelete(const ItemId &id, bool keepSnapshot);

  /**
   * @brief Process all pending deletions
   *
   * This should be called at a safe point (e.g., after event handling
   * is complete) to actually delete queued items.
   */
  void flushDeletions();

  /**
   * @brief Restore a previously deleted item (for undo operations)
   * @param id The ItemId of the item to restore
   * @return true if the item was successfully restored
   */
  bool restoreItem(const ItemId &id);

  /**
   * @brief Check if an item is pending deletion
   * @param id The ItemId to check
   * @return true if the item is scheduled for deletion
   */
  bool isPendingDeletion(const ItemId &id) const;

  /**
   * @brief Get the number of tracked items
   * @return The count of registered items
   */
  int itemCount() const;

  /**
   * @brief Get all registered ItemIds
   * @return Vector of all active ItemIds
   */
  std::vector<ItemId> allItemIds() const;

  /**
   * @brief Get the associated scene
   * @return Pointer to the QGraphicsScene
   */
  QGraphicsScene *scene() const { return scene_; }

  /**
   * @brief Clear all items from the store
   *
   * This unregisters and schedules deletion of all items.
   */
  void clear();

signals:
  /**
   * @brief Emitted when an item is registered
   * @param id The ItemId of the registered item
   */
  void itemRegistered(const ItemId &id);

  /**
   * @brief Emitted when an item is about to be deleted
   * @param id The ItemId of the item being deleted
   */
  void itemAboutToBeDeleted(const ItemId &id);

  /**
   * @brief Emitted when an item is restored
   * @param id The ItemId of the restored item
   */
  void itemRestored(const ItemId &id);

private:
  QPointer<QGraphicsScene> scene_;

  // Primary storage: ItemId -> QGraphicsItem*
  std::unordered_map<ItemId, QGraphicsItem *> items_;

  // Reverse lookup: QGraphicsItem* -> ItemId
  std::unordered_map<QGraphicsItem *, ItemId> reverseMap_;

  // Items removed from scene but kept for potential undo
  std::unordered_map<ItemId, QGraphicsItem *> snapshotItems_;

  // Items scheduled for permanent deletion
  std::vector<std::pair<ItemId, QGraphicsItem *>> deletionQueue_;
};

#endif // ITEM_STORE_H
