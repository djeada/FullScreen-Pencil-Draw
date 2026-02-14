/**
 * @file item_id.h
 * @brief Stable identifier type for graphics items.
 *
 * ItemId provides a stable, unique identifier for every graphics item in the
 * application. Unlike raw pointers, ItemIds remain valid even after items are
 * deleted and can be used safely across undo/redo operations.
 */
#ifndef ITEM_ID_H
#define ITEM_ID_H

#include <QHash>
#include <QUuid>
#include <functional>

/**
 * @brief Stable identifier for graphics items.
 *
 * ItemId wraps a QUuid to provide a unique, stable identifier for every
 * graphics item. Unlike raw pointers:
 * - ItemIds remain valid after item deletion (for undo/redo)
 * - ItemIds can be compared without accessing memory
 * - ItemIds can be serialized for save/load operations
 *
 * @note ItemId is intended to be a lightweight value type that can be
 * freely copied, compared, and stored.
 */
class ItemId {
public:
  /**
   * @brief Construct a null (invalid) ItemId
   */
  ItemId() : uuid_() {}

  /**
   * @brief Construct an ItemId from an existing QUuid
   * @param uuid The UUID to use
   */
  explicit ItemId(const QUuid &uuid) : uuid_(uuid) {}

  /**
   * @brief Generate a new unique ItemId
   * @return A new, unique ItemId
   */
  static ItemId generate() { return ItemId(QUuid::createUuid()); }

  /**
   * @brief Check if this ItemId is valid (non-null)
   * @return true if the ItemId is valid
   */
  bool isValid() const { return !uuid_.isNull(); }

  /**
   * @brief Check if this ItemId is null (invalid)
   * @return true if the ItemId is null
   */
  bool isNull() const { return uuid_.isNull(); }

  /**
   * @brief Get the underlying QUuid
   * @return The QUuid
   */
  const QUuid &uuid() const { return uuid_; }

  /**
   * @brief Convert to string representation
   * @return String representation of the ItemId
   */
  QString toString() const { return uuid_.toString(QUuid::WithoutBraces); }

  /**
   * @brief Create an ItemId from a string representation
   * @param str The string to parse
   * @return The parsed ItemId (may be null if parsing fails)
   */
  static ItemId fromString(const QString &str) {
    return ItemId(QUuid::fromString(str));
  }

  // Comparison operators
  bool operator==(const ItemId &other) const { return uuid_ == other.uuid_; }
  bool operator!=(const ItemId &other) const { return uuid_ != other.uuid_; }
  bool operator<(const ItemId &other) const { return uuid_ < other.uuid_; }

private:
  QUuid uuid_;
};

/**
 * @brief Hash function for ItemId (for use in QHash and std::unordered_map)
 */
inline uint qHash(const ItemId &id, uint seed = 0) {
  return qHash(id.uuid(), seed);
}

namespace std {
/**
 * @brief std::hash specialization for ItemId
 */
template <> struct hash<ItemId> {
  size_t operator()(const ItemId &id) const { return qHash(id.uuid()); }
};
} // namespace std

#endif // ITEM_ID_H
