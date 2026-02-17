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

private:
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
class CompositeAction : public Action {
public:
  CompositeAction();
  ~CompositeAction() override;

  void addAction(std::unique_ptr<Action> action);
  bool isEmpty() const { return actions_.empty(); }

  void undo() override;
  void redo() override;
  QString description() const override { return "Composite Action"; }

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

private:
  ItemId groupId_;
  QList<ItemId> itemIds_;
  ItemStore *itemStore_;
  QPointF groupPosition_;
  ItemCallback onAdd_;
  ItemCallback onRemove_;
};

#endif // ACTION_H
