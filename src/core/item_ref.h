/**
 * @file item_ref.h
 * @brief Lightweight handle for safe item access.
 *
 * ItemRef provides a safe way to reference items without storing raw pointers.
 * It resolves the ItemId to a pointer only when accessed, returning nullptr
 * if the item has been deleted.
 */
#ifndef ITEM_REF_H
#define ITEM_REF_H

#include "item_id.h"
#include "item_store.h"
#include <QGraphicsItem>

/**
 * @brief Lightweight handle for resolving ItemId to item pointer at use time.
 *
 * ItemRef is designed to replace raw `QGraphicsItem*` storage in subsystems.
 * Instead of storing a pointer that may become invalid, subsystems store
 * an ItemRef and resolve it when needed.
 *
 * Usage:
 * @code
 * ItemRef ref(store, itemId);
 * if (QGraphicsItem *item = ref.get()) {
 *   // Use the item safely
 * } else {
 *   // Item has been deleted
 * }
 * @endcode
 *
 * @note ItemRef does NOT extend item lifetime. It only provides safe access.
 */
class ItemRef {
public:
  /**
   * @brief Construct a null ItemRef
   */
  ItemRef() : store_(nullptr), id_() {}

  /**
   * @brief Construct an ItemRef for a specific item
   * @param store The ItemStore to resolve from
   * @param id The ItemId to reference
   */
  ItemRef(ItemStore *store, const ItemId &id) : store_(store), id_(id) {}

  /**
   * @brief Resolve the reference to a pointer
   * @return The item pointer, or nullptr if deleted or invalid
   */
  QGraphicsItem *get() const {
    return (store_ && id_.isValid()) ? store_->item(id_) : nullptr;
  }

  /**
   * @brief Resolve the reference to a typed pointer
   * @tparam T The expected item type
   * @return The typed item pointer, or nullptr if deleted/invalid/wrong type
   */
  template <typename T> T *getAs() const {
    return dynamic_cast<T *>(get());
  }

  /**
   * @brief Check if the referenced item still exists
   * @return true if the item is valid and accessible
   */
  bool isValid() const { return get() != nullptr; }

  /**
   * @brief Check if this reference is null (no ID assigned)
   * @return true if no ItemId is assigned
   */
  bool isNull() const { return !id_.isValid(); }

  /**
   * @brief Get the ItemId
   * @return The referenced ItemId
   */
  const ItemId &id() const { return id_; }

  /**
   * @brief Dereference operator for convenient access
   * @return The item pointer (may be nullptr)
   */
  QGraphicsItem *operator->() const { return get(); }

  /**
   * @brief Conversion to bool for validity checking
   * @return true if the item is valid
   */
  explicit operator bool() const { return isValid(); }

  // Comparison operators (compare by ID)
  bool operator==(const ItemRef &other) const { return id_ == other.id_; }
  bool operator!=(const ItemRef &other) const { return id_ != other.id_; }

private:
  ItemStore *store_;
  ItemId id_;
};

#endif // ITEM_REF_H
