/**
 * @file pdf_overlay.h
 * @brief Per-page overlay management for PDF annotation.
 *
 * This file defines the PdfOverlay class for storing editable
 * overlay content on top of PDF pages.
 */
#ifndef PDF_OVERLAY_H
#define PDF_OVERLAY_H

#ifdef HAVE_QT_PDF

#include <QGraphicsItem>
#include <QList>
#include <QObject>
#include <memory>
#include <unordered_map>
#include <vector>

#include "action.h"
#include "item_id.h"

class ItemStore;

/**
 * @brief Stores overlay content for a single PDF page.
 *
 * Each PdfPageOverlay contains a list of graphics items that
 * represent user annotations on that page.
 */
class PdfPageOverlay {
public:
  PdfPageOverlay();
  ~PdfPageOverlay();

  // Non-copyable
  PdfPageOverlay(const PdfPageOverlay &) = delete;
  PdfPageOverlay &operator=(const PdfPageOverlay &) = delete;

  // Movable
  PdfPageOverlay(PdfPageOverlay &&) noexcept;
  PdfPageOverlay &operator=(PdfPageOverlay &&) noexcept;

  /**
   * @brief Add an item to this overlay
   * @param item The graphics item to add
   */
  void addItem(QGraphicsItem *item);

  /**
   * @brief Add an item to this overlay by ItemId
   * @param id The ItemId of the item to add
   * @param store The ItemStore to resolve the item from
   */
  void addItem(const ItemId &id, ItemStore *store);

  /**
   * @brief Remove an item from this overlay
   * @param item The item to remove
   * @return true if the item was found and removed
   */
  bool removeItem(QGraphicsItem *item);

  /**
   * @brief Remove an item from this overlay by ItemId
   * @param id The ItemId of the item to remove
   * @return true if the item was found and removed
   */
  bool removeItem(const ItemId &id);

  /**
   * @brief Get all items in this overlay (resolved from ItemStore)
   * @return List of graphics items (resolved at call time, safe from dangling)
   * @note Returns empty list if no ItemStore is set
   */
  QList<QGraphicsItem *> items() const;

  /**
   * @brief Get all ItemIds in this overlay
   * @return List of ItemIds
   */
  const QList<ItemId> &itemIds() const { return itemIds_; }

  /**
   * @brief Check if an item belongs to this overlay
   * @param item The item to check
   * @return true if the item is in this overlay
   */
  bool containsItem(QGraphicsItem *item) const;

  /**
   * @brief Check if an item belongs to this overlay by ItemId
   * @param id The ItemId to check
   * @return true if the item is in this overlay
   */
  bool containsItem(const ItemId &id) const;

  /**
   * @brief Clear all items from the overlay
   * @note Items are not deleted, only removed from the overlay
   */
  void clear();

  /**
   * @brief Get the number of items in this overlay
   * @return Item count
   */
  int itemCount() const { return itemIds_.size(); }

  /**
   * @brief Set visibility of all items in the overlay
   * @param visible Whether items should be visible
   */
  void setVisible(bool visible);

  /**
   * @brief Check if the overlay is visible
   * @return true if visible
   */
  bool isVisible() const { return visible_; }

  /**
   * @brief Set the ItemStore for this overlay (for ID-based operations)
   * @param store The ItemStore to use
   */
  void setItemStore(ItemStore *store);

private:
  QList<ItemId> itemIds_; // Primary storage: stable ItemIds
  ItemStore *itemStore_;  // For resolving ItemIds to items
  bool visible_;
};

/**
 * @brief Manages overlays for all pages in a PDF document.
 *
 * PdfOverlayManager maintains a collection of per-page overlays
 * and coordinates with the undo/redo system.
 */
class PdfOverlayManager : public QObject {
  Q_OBJECT

public:
  explicit PdfOverlayManager(QObject *parent = nullptr);
  ~PdfOverlayManager() override;

  /**
   * @brief Initialize overlays for a document with given page count
   * @param pageCount Number of pages
   */
  void initialize(int pageCount);

  /**
   * @brief Set the ItemStore used for ID-based resolution
   * @param store The ItemStore to use
   */
  void setItemStore(ItemStore *store);

  /**
   * @brief Get the overlay for a specific page
   * @param pageIndex The page index (0-based)
   * @return Pointer to the overlay, or nullptr if invalid
   */
  PdfPageOverlay *overlay(int pageIndex);

  /**
   * @brief Get the overlay for a specific page (const)
   * @param pageIndex The page index (0-based)
   * @return Const pointer to the overlay, or nullptr if invalid
   */
  const PdfPageOverlay *overlay(int pageIndex) const;

  /**
   * @brief Add an item to a page's overlay
   * @param pageIndex The page index
   * @param item The item to add
   */
  void addItemToPage(int pageIndex, QGraphicsItem *item);

  /**
   * @brief Remove an item from its page's overlay
   * @param pageIndex The page index
   * @param item The item to remove
   * @return true if removed
   */
  bool removeItemFromPage(int pageIndex, QGraphicsItem *item);

  /**
   * @brief Find which page contains a specific item
   * @param item The graphics item
   * @return The page index, or -1 if not found
   */
  int findPageForItem(QGraphicsItem *item) const;

  /**
   * @brief Get the number of pages
   * @return Page count
   */
  int pageCount() const { return static_cast<int>(overlays_.size()); }

  /**
   * @brief Clear all overlays
   */
  void clear();

  /**
   * @brief Show overlay for a specific page, hide others
   * @param pageIndex The page index to show
   */
  void showPage(int pageIndex);

  // Undo/Redo support
  /**
   * @brief Get the undo stack for a specific page
   * @param pageIndex The page index
   * @return Reference to the undo stack
   */
  std::vector<std::unique_ptr<Action>> &undoStack(int pageIndex);

  /**
   * @brief Get the redo stack for a specific page
   * @param pageIndex The page index
   * @return Reference to the redo stack
   */
  std::vector<std::unique_ptr<Action>> &redoStack(int pageIndex);

  /**
   * @brief Check if undo is available for a page
   * @param pageIndex The page index
   * @return true if undo is available
   */
  bool canUndo(int pageIndex) const;

  /**
   * @brief Check if redo is available for a page
   * @param pageIndex The page index
   * @return true if redo is available
   */
  bool canRedo(int pageIndex) const;

signals:
  /**
   * @brief Emitted when an overlay is modified
   * @param pageIndex The modified page index
   */
  void overlayModified(int pageIndex);

private:
  std::vector<std::unique_ptr<PdfPageOverlay>> overlays_;
  std::vector<std::vector<std::unique_ptr<Action>>> undoStacks_;
  std::vector<std::vector<std::unique_ptr<Action>>> redoStacks_;
  int currentPage_;
  ItemStore *itemStore_;
};

#endif // HAVE_QT_PDF

#endif // PDF_OVERLAY_H
