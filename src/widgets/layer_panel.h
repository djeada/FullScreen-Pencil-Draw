/**
 * @file layer_panel.h
 * @brief Layer panel widget for managing layers.
 */
#ifndef LAYER_PANEL_H
#define LAYER_PANEL_H

#include "../core/item_id.h"
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTreeWidget>
#include <QUuid>

class LayerManager;
class Layer;
class ItemStore;
class Canvas;

/**
 * @brief Custom tree widget that handles drag-and-drop reordering of items.
 */
class LayerTreeWidget : public QTreeWidget {
  Q_OBJECT

public:
  explicit LayerTreeWidget(QWidget *parent = nullptr);

  void setLayerManager(LayerManager *manager) { layerManager_ = manager; }

signals:
  void itemReordered();

protected:
  void dropEvent(QDropEvent *event) override;

private:
  LayerManager *layerManager_ = nullptr;
};

/**
 * @brief A dock widget panel for managing layers.
 *
 * Provides a visual interface for creating, deleting, reordering,
 * and modifying layer properties. Shows elements nested under their
 * layers with drag-and-drop reordering support.
 */
class LayerPanel : public QDockWidget {
  Q_OBJECT

public:
  explicit LayerPanel(LayerManager *manager, QWidget *parent = nullptr);
  ~LayerPanel() override;

  /**
   * @brief Set the canvas for selection synchronization
   */
  void setCanvas(Canvas *canvas);

  /**
   * @brief Set the ItemStore for item type lookups
   */
  void setItemStore(ItemStore *store);

public slots:
  /**
   * @brief Refresh the layer list display
   */
  void refreshLayerList();

  /**
   * @brief Update selection highlight from canvas selection
   */
  void onCanvasSelectionChanged();

signals:
  /**
   * @brief Emitted when a new layer should be created
   */
  void addLayerRequested();

  /**
   * @brief Emitted when the selected layer should be deleted
   */
  void deleteLayerRequested();

  /**
   * @brief Emitted when a layer is selected
   * @param index The selected layer index
   */
  void layerSelected(int index);

private slots:
  void onAddLayer();
  void onDeleteLayer();
  void onMoveLayerUp();
  void onMoveLayerDown();
  void onDuplicateLayer();
  void onMergeDown();
  void onTreeSelectionChanged();
  void onOpacityChanged(int value);
  void onVisibilityToggled();
  void onLockToggled();
  void onTreeItemDropped(QTreeWidgetItem *item, int fromIndex, int toIndex);

private:
  LayerManager *layerManager_;
  ItemStore *itemStore_;
  Canvas *canvas_;
  LayerTreeWidget *layerTree_;
  QPushButton *addButton_;
  QPushButton *deleteButton_;
  QPushButton *moveUpButton_;
  QPushButton *moveDownButton_;
  QPushButton *duplicateButton_;
  QPushButton *mergeButton_;
  QPushButton *visibilityButton_;
  QPushButton *lockButton_;
  QSlider *opacitySlider_;
  QLabel *opacityLabel_;
  bool updatingSelection_;

  void setupUI();
  void updateButtonStates();
  void updatePropertyControls();
  QString itemDescription(const ItemId &id) const;

public:
  // Custom data roles (public for LayerTreeWidget access)
  static constexpr int LayerIdRole = Qt::UserRole + 1;
  static constexpr int ItemIdRole = Qt::UserRole + 2;
  static constexpr int IsLayerRole = Qt::UserRole + 3;
  static constexpr int ItemIndexRole = Qt::UserRole + 4;
};

#endif // LAYER_PANEL_H
