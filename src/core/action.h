/**
 * @file action.h
 * @brief Undo/Redo action system for canvas operations.
 *
 * All actions use ItemId-based storage for safe undo/redo.
 * Item pointers are NEVER cached - always resolved via ItemStore.
 */
#ifndef ACTION_H
#define ACTION_H

#include <QBrush>
#include <QColor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QImage>
#include <QPen>
#include <QPointF>
#include <QPointer>
#include <QString>
#include <QUuid>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include "item_id.h"

class ItemStore;

/**
 * @brief Abstract base class for all undoable actions.
 */
class Action {
public:
  Action() = default;
  virtual ~Action();

  virtual void undo() = 0;
  virtual void redo() = 0;
  virtual QString description() const { return "Action"; }

  /**
   * @brief Report all ItemIds this action may keep parked as undo snapshots.
   *
   * Used by UndoRedoManager to notify stores when an action is discarded
   * (evicted from history), so parked snapshot items can be released.
   */
  virtual void collectReferencedItems(QVector<ItemId> &out) const {
    Q_UNUSED(out);
  }

  /**
   * @brief Approximate memory this history entry keeps alive (captured
   *        images, parked snapshot items), in bytes.
   *
   * UndoRedoManager evicts the oldest entries once the sum exceeds its
   * memory budget. The default covers small bookkeeping-only actions.
   */
  virtual std::size_t memoryCost() const { return kBaseActionCost; }

  static constexpr std::size_t kBaseActionCost = 256;

  /// Rough heap size of an item tree (pixmap pixels, path elements).
  static std::size_t estimateItemBytes(const QGraphicsItem *item);
};

/**
 * @brief Action for adding items to the scene.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class DrawAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  DrawAction(const ItemId &id, ItemStore *store, ItemCallback onAdd = {},
             ItemCallback onRemove = {});
  ~DrawAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Draw"; }
  void collectReferencedItems(QVector<ItemId> &out) const override {
    out.append(itemId_);
  }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for removing items from the scene.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class DeleteAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  DeleteAction(const ItemId &id, ItemStore *store, ItemCallback onAdd = {},
               ItemCallback onRemove = {});
  ~DeleteAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Delete"; }
  void collectReferencedItems(QVector<ItemId> &out) const override {
    out.append(itemId_);
  }
  /// Includes the parked snapshot of the deleted item.
  std::size_t memoryCost() const override { return memoryCost_; }

private:
  std::size_t memoryCost_ = kBaseActionCost;
  ItemId itemId_;
  ItemStore *itemStore_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for moving items on the scene.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class MoveAction : public Action {
public:
  MoveAction(const ItemId &id, ItemStore *store, const QPointF &oldPos,
             const QPointF &newPos);
  ~MoveAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Move"; }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  QPointF oldPos_;
  QPointF newPos_;
};

/**
 * @brief Composite action that groups multiple actions together.
 */
/**
 * @brief Undoable change of state that isn't an item (e.g. the canvas
 *        size), expressed as a pair of callbacks.
 */
class CallbackAction : public Action {
public:
  CallbackAction(QString description, std::function<void()> undoFn,
                 std::function<void()> redoFn)
      : description_(std::move(description)), undo_(std::move(undoFn)),
        redo_(std::move(redoFn)) {}

  void undo() override {
    if (undo_)
      undo_();
  }
  void redo() override {
    if (redo_)
      redo_();
  }
  QString description() const override { return description_; }

private:
  QString description_;
  std::function<void()> undo_;
  std::function<void()> redo_;
};

class CompositeAction : public Action {
public:
  CompositeAction();
  ~CompositeAction() override;

  void addAction(std::unique_ptr<Action> action);
  bool isEmpty() const { return actions_.empty(); }

  void undo() override;
  void redo() override;
  QString description() const override { return "Composite Action"; }
  void collectReferencedItems(QVector<ItemId> &out) const override;
  std::size_t memoryCost() const override;

private:
  std::vector<std::unique_ptr<Action>> actions_;
};

/**
 * @brief Action for applying fill/color style changes to an item.
 * Items are tracked by ItemId only - never by raw pointer.
 */
class FillAction : public Action {
public:
  struct PixmapTintState {
    bool enabled = false;
    QColor color;
    qreal strength = 0.0;
  };

  FillAction(const ItemId &id, ItemStore *store, const QBrush &oldBrush,
             const QBrush &newBrush);
  FillAction(const ItemId &id, ItemStore *store, const QPen &oldPen,
             const QPen &newPen);
  FillAction(const ItemId &id, ItemStore *store, const QColor &oldColor,
             const QColor &newColor);
  FillAction(const ItemId &id, ItemStore *store, const QString &oldTheme,
             const QString &newTheme);
  FillAction(const ItemId &id, ItemStore *store,
             const PixmapTintState &oldTintState,
             const PixmapTintState &newTintState);
  ~FillAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Fill"; }

private:
  enum class Property {
    Brush,
    Pen,
    TextColor,
    MermaidTheme,
    PixmapTint,
  };

  void applyBrush(const QBrush &brush);
  void applyPen(const QPen &pen);
  void applyTextColor(const QColor &color);
  void applyMermaidTheme(const QString &theme);
  void applyPixmapTint(const PixmapTintState &state);

  ItemId itemId_;
  ItemStore *itemStore_;
  Property property_;
  QBrush oldBrush_;
  QBrush newBrush_;
  QPen oldPen_;
  QPen newPen_;
  QColor oldColor_;
  QColor newColor_;
  QString oldTheme_;
  QString newTheme_;
  PixmapTintState oldTintState_;
  PixmapTintState newTintState_;
};

/**
 * @brief Action for changing a pixmap item's pixels.
 * Stores full before/after image snapshots for undo/redo.
 */
class RasterPixmapAction : public Action {
public:
  RasterPixmapAction(const ItemId &id, ItemStore *store, const QImage &oldImage,
                     const QImage &newImage);
  ~RasterPixmapAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Raster Edit"; }
  std::size_t memoryCost() const override {
    return kBaseActionCost + static_cast<std::size_t>(oldImage_.sizeInBytes()) +
           static_cast<std::size_t>(newImage_.sizeInBytes());
  }

private:
  ItemId itemId_;
  ItemStore *itemStore_;
  QImage oldImage_;
  QImage newImage_;
};

class QGraphicsItemGroup;

class LayerManager;

/**
 * @brief Action for reordering an item's z-position within its layer.
 * Tracks the layer by UUID and item by ItemId.
 */
class ReorderAction : public Action {
public:
  ReorderAction(const ItemId &itemId, const QUuid &layerId, int oldIndex,
                int newIndex, LayerManager *layerManager);
  ~ReorderAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Reorder"; }

private:
  ItemId itemId_;
  QUuid layerId_;
  int oldIndex_;
  int newIndex_;
  LayerManager *layerManager_;
};

/**
 * @brief Action for grouping multiple items together.
 * All items tracked by ItemId only - never by raw pointer.
 */
class GroupAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  GroupAction(const ItemId &groupId, const QList<ItemId> &itemIds,
              ItemStore *store, const QList<QPointF> &originalPositions,
              ItemCallback onAdd = {}, ItemCallback onRemove = {});
  ~GroupAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Group"; }
  void collectReferencedItems(QVector<ItemId> &out) const override {
    out.append(groupId_);
    out.append(itemIds_);
  }

private:
  ItemId groupId_;
  QList<ItemId> itemIds_;
  ItemStore *itemStore_;
  QList<QPointF> originalPositions_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

/**
 * @brief Action for ungrouping a group into individual items.
 * All items tracked by ItemId only - never by raw pointer.
 */
class UngroupAction : public Action {
public:
  using ItemCallback = std::function<void(QGraphicsItem *)>;

  UngroupAction(const ItemId &groupId, const QList<ItemId> &itemIds,
                ItemStore *store, const QPointF &groupPosition,
                ItemCallback onAdd = {}, ItemCallback onRemove = {});
  ~UngroupAction() override;

  void undo() override;
  void redo() override;
  QString description() const override { return "Ungroup"; }
  void collectReferencedItems(QVector<ItemId> &out) const override {
    out.append(groupId_);
    out.append(itemIds_);
  }

private:
  ItemId groupId_;
  QList<ItemId> itemIds_;
  ItemStore *itemStore_;
  QPointF groupPosition_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

#endif // ACTION_H
