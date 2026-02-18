/**
 * @file layer_panel.cpp
 * @brief Implementation of the layer panel widget.
 */
#include "layer_panel.h"
#include "../core/item_store.h"
#include "../core/layer.h"
#include "../core/scene_controller.h"
#include "canvas.h"
#include <QDropEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QVBoxLayout>

// LayerTreeWidget implementation
LayerTreeWidget::LayerTreeWidget(QWidget *parent) : QTreeWidget(parent) {}

void LayerTreeWidget::dropEvent(QDropEvent *event) {
  if (!layerManager_) {
    QTreeWidget::dropEvent(event);
    return;
  }

  // Get dragged item before Qt processes the drop
  QList<QTreeWidgetItem *> draggedItems = selectedItems();
  if (draggedItems.isEmpty()) {
    QTreeWidget::dropEvent(event);
    return;
  }

  QTreeWidgetItem *draggedItem = draggedItems.first();

  // Only allow reordering child items (not layers)
  bool isLayer = draggedItem->data(0, LayerPanel::IsLayerRole).toBool();
  if (isLayer) {
    event->ignore();
    return;
  }

  // Get the item being dragged
  ItemId itemId = ItemId::fromString(
      draggedItem->data(0, LayerPanel::ItemIdRole).toString());
  QUuid layerId(draggedItem->data(0, LayerPanel::LayerIdRole).toString());
  int oldItemIndex = draggedItem->data(0, LayerPanel::ItemIndexRole).toInt();

  // Find drop target
  QTreeWidgetItem *dropTarget = itemAt(event->position().toPoint());
  if (!dropTarget) {
    event->ignore();
    return;
  }

  // Determine parent (must be same layer)
  QTreeWidgetItem *targetParent = dropTarget->parent();
  bool droppingOnLayer = dropTarget->data(0, LayerPanel::IsLayerRole).toBool();

  if (droppingOnLayer) {
    targetParent = dropTarget;
  }

  if (!targetParent) {
    event->ignore();
    return;
  }

  // Verify same layer
  QUuid targetLayerId(
      targetParent->data(0, LayerPanel::LayerIdRole).toString());
  if (targetLayerId != layerId) {
    event->ignore();
    return;
  }

  // Calculate new index from drop position
  // Tree shows items in reverse z-order (highest first), so we need to convert
  DropIndicatorPosition dropPos = dropIndicatorPosition();
  int visualIndex = 0;
  if (droppingOnLayer) {
    // Dropping on the layer itself means top of list = highest z
    visualIndex = 0;
  } else {
    visualIndex = targetParent->indexOfChild(dropTarget);
    if (dropPos == QAbstractItemView::BelowItem) {
      visualIndex += 1;
    }
  }

  // Convert visual index (0 = top = highest z) to layer index
  Layer *layer = layerManager_->layer(layerId);
  if (!layer) {
    event->ignore();
    return;
  }
  int totalItems = layer->itemCount();
  int newItemIndex = totalItems - 1 - visualIndex;
  if (newItemIndex < 0)
    newItemIndex = 0;
  if (newItemIndex >= totalItems)
    newItemIndex = totalItems - 1;

  if (newItemIndex != oldItemIndex && itemId.isValid()) {
    layerManager_->reorderItem(itemId, newItemIndex);
  }

  // Don't let Qt handle the default move — we rebuild the tree via refresh
  event->accept();
  emit itemReordered();
}

LayerPanel::LayerPanel(LayerManager *manager, QWidget *parent)
    : QDockWidget("Layers", parent), layerManager_(manager),
      itemStore_(nullptr), canvas_(nullptr), updatingSelection_(false) {
  setupUI();
  refreshLayerList();

  // Connect tree widget reorder signal
  connect(layerTree_, &LayerTreeWidget::itemReordered, this,
          &LayerPanel::refreshLayerList);

  // Connect layer manager signals
  if (layerManager_) {
    connect(layerManager_, &LayerManager::layerAdded, this,
            &LayerPanel::refreshLayerList);
    connect(layerManager_, &LayerManager::layerRemoved, this,
            &LayerPanel::refreshLayerList);
    connect(layerManager_, &LayerManager::layerOrderChanged, this,
            &LayerPanel::refreshLayerList);
    connect(layerManager_, &LayerManager::activeLayerChanged, this,
            &LayerPanel::refreshLayerList);
    connect(layerManager_, &LayerManager::itemOrderChanged, this,
            &LayerPanel::refreshLayerList);
  }
}

LayerPanel::~LayerPanel() = default;

void LayerPanel::setCanvas(Canvas *canvas) {
  canvas_ = canvas;
  if (canvas_ && canvas_->scene()) {
    connect(canvas_->scene(), &QGraphicsScene::selectionChanged, this,
            &LayerPanel::onCanvasSelectionChanged);
  }
  // Refresh tree when items are actually added/removed (not on every canvas
  // modification, which fires too often and destroys tree state mid-click).
  if (canvas_ && canvas_->sceneController()) {
    connect(canvas_->sceneController(), &SceneController::itemAdded, this,
            &LayerPanel::refreshLayerList);
    connect(canvas_->sceneController(), &SceneController::itemRemoved, this,
            &LayerPanel::refreshLayerList);
    connect(canvas_->sceneController(), &SceneController::itemRestored, this,
            &LayerPanel::refreshLayerList);
  }
}

void LayerPanel::setItemStore(ItemStore *store) { itemStore_ = store; }

void LayerPanel::setupUI() {
  QWidget *container = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(container);
  mainLayout->setContentsMargins(8, 8, 8, 8);
  mainLayout->setSpacing(8);

  // Layer tree
  layerTree_ = new LayerTreeWidget(container);
  layerTree_->setLayerManager(layerManager_);
  layerTree_->setHeaderHidden(true);
  layerTree_->setColumnCount(1);
  layerTree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  layerTree_->setDragDropMode(QAbstractItemView::InternalMove);
  layerTree_->setDefaultDropAction(Qt::MoveAction);
  layerTree_->setDragEnabled(true);
  layerTree_->setAcceptDrops(true);
  layerTree_->setDropIndicatorShown(true);
  layerTree_->setExpandsOnDoubleClick(false);
  layerTree_->setIndentation(16);
  layerTree_->setMaximumHeight(350);
  layerTree_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(layerTree_, &QTreeWidget::itemSelectionChanged, this,
          &LayerPanel::onTreeSelectionChanged);
  connect(layerTree_, &QWidget::customContextMenuRequested, this,
          &LayerPanel::onLayerTreeContextMenuRequested);
  mainLayout->addWidget(layerTree_);

  // Layer controls row 1 - Add/Delete/Duplicate/Merge
  QHBoxLayout *controlsRow1 = new QHBoxLayout();
  controlsRow1->setSpacing(4);

  addButton_ = new QPushButton(QString::fromUtf8("＋"), container);
  addButton_->setToolTip("Add new layer");
  addButton_->setMinimumSize(40, 40);
  connect(addButton_, &QPushButton::clicked, this, &LayerPanel::onAddLayer);
  controlsRow1->addWidget(addButton_);

  deleteButton_ = new QPushButton(QString::fromUtf8("−"), container);
  deleteButton_->setToolTip("Delete layer");
  deleteButton_->setMinimumSize(40, 40);
  connect(deleteButton_, &QPushButton::clicked, this,
          &LayerPanel::onDeleteLayer);
  controlsRow1->addWidget(deleteButton_);

  duplicateButton_ = new QPushButton(QString::fromUtf8("⧉"), container);
  duplicateButton_->setToolTip("Duplicate layer");
  duplicateButton_->setMinimumSize(40, 40);
  connect(duplicateButton_, &QPushButton::clicked, this,
          &LayerPanel::onDuplicateLayer);
  controlsRow1->addWidget(duplicateButton_);

  mergeButton_ = new QPushButton(QString::fromUtf8("⊕"), container);
  mergeButton_->setToolTip("Merge with layer below");
  mergeButton_->setMinimumSize(40, 40);
  connect(mergeButton_, &QPushButton::clicked, this, &LayerPanel::onMergeDown);
  controlsRow1->addWidget(mergeButton_);

  controlsRow1->addStretch();
  mainLayout->addLayout(controlsRow1);

  // Layer controls row 2 - Move/Visibility/Lock
  QHBoxLayout *controlsRow2 = new QHBoxLayout();
  controlsRow2->setSpacing(4);

  moveUpButton_ = new QPushButton(QString::fromUtf8("▲"), container);
  moveUpButton_->setToolTip("Move layer up");
  moveUpButton_->setMinimumSize(40, 40);
  connect(moveUpButton_, &QPushButton::clicked, this,
          &LayerPanel::onMoveLayerUp);
  controlsRow2->addWidget(moveUpButton_);

  moveDownButton_ = new QPushButton(QString::fromUtf8("▼"), container);
  moveDownButton_->setToolTip("Move layer down");
  moveDownButton_->setMinimumSize(40, 40);
  connect(moveDownButton_, &QPushButton::clicked, this,
          &LayerPanel::onMoveLayerDown);
  controlsRow2->addWidget(moveDownButton_);

  visibilityButton_ =
      new QPushButton(QString::fromUtf8("\xF0\x9F\x91\x81"), container);
  visibilityButton_->setToolTip("Toggle visibility");
  visibilityButton_->setMinimumSize(40, 40);
  visibilityButton_->setCheckable(true);
  connect(visibilityButton_, &QPushButton::clicked, this,
          &LayerPanel::onVisibilityToggled);
  controlsRow2->addWidget(visibilityButton_);

  lockButton_ =
      new QPushButton(QString::fromUtf8("\xF0\x9F\x94\x92"), container);
  lockButton_->setToolTip("Toggle lock");
  lockButton_->setMinimumSize(40, 40);
  lockButton_->setCheckable(true);
  connect(lockButton_, &QPushButton::clicked, this, &LayerPanel::onLockToggled);
  controlsRow2->addWidget(lockButton_);

  controlsRow2->addStretch();
  mainLayout->addLayout(controlsRow2);

  // Opacity control
  QGroupBox *opacityGroup = new QGroupBox("Layer Opacity", container);
  QHBoxLayout *opacityLayout = new QHBoxLayout(opacityGroup);
  opacityLayout->setContentsMargins(8, 12, 8, 8);

  opacitySlider_ = new QSlider(Qt::Horizontal, opacityGroup);
  opacitySlider_->setRange(0, 100);
  opacitySlider_->setValue(100);
  opacitySlider_->setMinimumHeight(24);
  connect(opacitySlider_, &QSlider::valueChanged, this,
          &LayerPanel::onOpacityChanged);
  opacityLayout->addWidget(opacitySlider_);

  opacityLabel_ = new QLabel("100%", opacityGroup);
  opacityLabel_->setMinimumWidth(45);
  opacityLayout->addWidget(opacityLabel_);

  mainLayout->addWidget(opacityGroup);

  mainLayout->addStretch();

  container->setLayout(mainLayout);
  setWidget(container);
  setMinimumWidth(200);
  setMaximumWidth(280);

  // Modern flat style with enhanced polish
  setStyleSheet(R"(
    QDockWidget {
      background-color: #1a1a1e;
      color: #f8f8fc;
      font-weight: 500;
    }
    QDockWidget::title {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a2a30, stop:1 #242428);
      padding: 12px 14px;
      font-weight: 600;
      border-bottom: 1px solid rgba(255, 255, 255, 0.06);
    }
    QTreeWidget {
      background-color: #161618;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 10px;
      padding: 6px;
      outline: none;
    }
    QTreeWidget::item {
      padding: 6px 8px;
      border-radius: 6px;
      margin: 1px;
    }
    QTreeWidget::item:hover {
      background-color: rgba(255, 255, 255, 0.06);
    }
    QTreeWidget::item:selected {
      background-color: #3b82f6;
      color: #ffffff;
    }
    QTreeWidget::branch {
      background: transparent;
    }
    QTreeWidget::branch:has-children:!has-siblings:closed,
    QTreeWidget::branch:closed:has-children:has-siblings {
      image: none;
      border-image: none;
    }
    QTreeWidget::branch:open:has-children:!has-siblings,
    QTreeWidget::branch:open:has-children:has-siblings {
      image: none;
      border-image: none;
    }
    QPushButton {
      background-color: rgba(255, 255, 255, 0.06);
      color: #e0e0e6;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 8px;
      padding: 10px;
      min-height: 26px;
      font-weight: 500;
    }
    QPushButton:hover {
      background-color: rgba(255, 255, 255, 0.1);
      border: 1px solid rgba(59, 130, 246, 0.3);
      color: #f8f8fc;
    }
    QPushButton:pressed {
      background-color: rgba(255, 255, 255, 0.04);
    }
    QPushButton:checked {
      background-color: #3b82f6;
      color: #ffffff;
      border: 1px solid #60a5fa;
    }
    QPushButton:checked:hover {
      background-color: #60a5fa;
    }
    QPushButton:disabled {
      background-color: rgba(255, 255, 255, 0.02);
      color: #555560;
      border: 1px solid rgba(255, 255, 255, 0.03);
    }
    QGroupBox {
      color: #a0a0a8;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 10px;
      margin-top: 18px;
      padding-top: 14px;
      font-weight: 500;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 12px;
      padding: 0 8px;
      color: #f8f8fc;
    }
    QSlider::groove:horizontal {
      background: #28282e;
      height: 8px;
      border-radius: 4px;
    }
    QSlider::handle:horizontal {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #60a5fa, stop:1 #3b82f6);
      width: 18px;
      height: 18px;
      margin: -5px 0;
      border-radius: 9px;
      border: 2px solid rgba(255, 255, 255, 0.15);
    }
    QSlider::handle:horizontal:hover {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #93c5fd, stop:1 #60a5fa);
    }
    QSlider::sub-page:horizontal {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa);
      border-radius: 4px;
    }
    QLabel {
      color: #f8f8fc;
    }
  )");
}

void LayerPanel::refreshLayerList() {
  if (!layerManager_)
    return;

  // Don't rebuild the tree during a selection interaction — it would
  // destroy the items the user is clicking/dragging.
  if (updatingSelection_)
    return;

  // Block signals to prevent infinite recursion
  layerTree_->blockSignals(true);

  // Remember expanded state
  QSet<QUuid> expandedLayers;
  for (int i = 0; i < layerTree_->topLevelItemCount(); ++i) {
    QTreeWidgetItem *item = layerTree_->topLevelItem(i);
    if (item->isExpanded()) {
      expandedLayers.insert(QUuid(item->data(0, LayerIdRole).toString()));
    }
  }

  layerTree_->clear();

  // Add layers in reverse order (top layer first in tree)
  for (int i = layerManager_->layerCount() - 1; i >= 0; --i) {
    Layer *layer = layerManager_->layer(i);
    if (!layer)
      continue;

    QString prefix;
    if (layer->isVisible()) {
      prefix = QString::fromUtf8("\xF0\x9F\x91\x81 ");
    } else {
      prefix = "   ";
    }
    if (layer->isLocked()) {
      prefix += QString::fromUtf8("\xF0\x9F\x94\x92 ");
    }

    QTreeWidgetItem *layerItem = new QTreeWidgetItem(layerTree_);
    layerItem->setText(0, prefix + layer->name());
    layerItem->setData(0, LayerIdRole, layer->id().toString());
    layerItem->setData(0, IsLayerRole, true);
    // Layers accept drops (items can be reordered within)
    layerItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                        Qt::ItemIsDropEnabled);

    // Add items in reverse order (top item first = last in itemIds)
    const QList<ItemId> &ids = layer->itemIds();
    for (int j = ids.size() - 1; j >= 0; --j) {
      QTreeWidgetItem *childItem = new QTreeWidgetItem(layerItem);
      childItem->setText(0, itemDescription(ids[j]));
      childItem->setData(0, ItemIdRole, ids[j].toString());
      childItem->setData(0, LayerIdRole, layer->id().toString());
      childItem->setData(0, IsLayerRole, false);
      childItem->setData(0, ItemIndexRole, j);
      // Items can be dragged and selected
      childItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                          Qt::ItemIsDragEnabled);
    }

    // Expand if was previously expanded, or expand by default
    if (expandedLayers.contains(layer->id()) || expandedLayers.isEmpty()) {
      layerItem->setExpanded(true);
    }
  }

  // Highlight active layer
  int activeIndex = layerManager_->activeLayerIndex();
  if (activeIndex >= 0) {
    int treeIndex = layerManager_->layerCount() - 1 - activeIndex;
    if (treeIndex >= 0 && treeIndex < layerTree_->topLevelItemCount()) {
      QTreeWidgetItem *activeItem = layerTree_->topLevelItem(treeIndex);
      QFont boldFont = activeItem->font(0);
      boldFont.setBold(true);
      activeItem->setFont(0, boldFont);
    }
  }

  layerTree_->blockSignals(false);

  updateButtonStates();
  updatePropertyControls();
}

void LayerPanel::onCanvasSelectionChanged() {
  if (updatingSelection_ || !canvas_ || !canvas_->scene() || !itemStore_)
    return;

  updatingSelection_ = true;
  layerTree_->blockSignals(true);

  // Clear tree selection
  layerTree_->clearSelection();

  // Find and select tree items matching canvas selection
  QList<QGraphicsItem *> selected = canvas_->scene()->selectedItems();
  for (QGraphicsItem *gItem : selected) {
    ItemId id = itemStore_->idForItem(gItem);
    if (!id.isValid())
      continue;

    QString idStr = id.toString();
    // Search all child items in tree
    for (int i = 0; i < layerTree_->topLevelItemCount(); ++i) {
      QTreeWidgetItem *layerItem = layerTree_->topLevelItem(i);
      for (int j = 0; j < layerItem->childCount(); ++j) {
        QTreeWidgetItem *child = layerItem->child(j);
        if (child->data(0, ItemIdRole).toString() == idStr) {
          child->setSelected(true);
          layerTree_->scrollToItem(child);
        }
      }
    }
  }

  layerTree_->blockSignals(false);
  updatingSelection_ = false;
}

void LayerPanel::updateButtonStates() {
  if (!layerManager_)
    return;

  int layerCount = layerManager_->layerCount();
  int activeIndex = layerManager_->activeLayerIndex();

  deleteButton_->setEnabled(layerCount > 1);
  moveUpButton_->setEnabled(activeIndex > 0);
  moveDownButton_->setEnabled(activeIndex < layerCount - 1 && activeIndex >= 0);
  mergeButton_->setEnabled(activeIndex > 0);
  duplicateButton_->setEnabled(activeIndex >= 0);
}

void LayerPanel::updatePropertyControls() {
  if (!layerManager_)
    return;

  Layer *layer = layerManager_->activeLayer();
  if (layer) {
    opacitySlider_->blockSignals(true);
    opacitySlider_->setValue(static_cast<int>(layer->opacity() * 100));
    opacitySlider_->blockSignals(false);
    opacityLabel_->setText(
        QString("%1%").arg(static_cast<int>(layer->opacity() * 100)));

    visibilityButton_->setChecked(layer->isVisible());
    lockButton_->setChecked(layer->isLocked());
  }
}

void LayerPanel::onAddLayer() {
  if (layerManager_) {
    int count = layerManager_->layerCount();
    layerManager_->createLayer(QString("Layer %1").arg(count + 1));
    emit addLayerRequested();
  }
}

void LayerPanel::onDeleteLayer() {
  if (!layerManager_)
    return;

  if (layerManager_->layerCount() <= 1) {
    QMessageBox::warning(this, "Cannot Delete",
                         "Cannot delete the last remaining layer.");
    return;
  }

  int activeIndex = layerManager_->activeLayerIndex();
  if (activeIndex >= 0) {
    Layer *layer = layerManager_->layer(activeIndex);
    if (layer && layer->itemCount() > 0) {
      QMessageBox::StandardButton reply = QMessageBox::question(
          this, "Delete Layer",
          QString("Layer '%1' contains %2 items. Delete anyway?")
              .arg(layer->name())
              .arg(layer->itemCount()),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
      if (reply != QMessageBox::Yes) {
        return;
      }
    }
    layerManager_->deleteLayer(activeIndex);
    emit deleteLayerRequested();
  }
}

void LayerPanel::onMoveLayerUp() {
  if (layerManager_) {
    int activeIndex = layerManager_->activeLayerIndex();
    layerManager_->moveLayerUp(activeIndex);
  }
}

void LayerPanel::onMoveLayerDown() {
  if (layerManager_) {
    int activeIndex = layerManager_->activeLayerIndex();
    layerManager_->moveLayerDown(activeIndex);
  }
}

void LayerPanel::onDuplicateLayer() {
  if (layerManager_) {
    int activeIndex = layerManager_->activeLayerIndex();
    layerManager_->duplicateLayer(activeIndex);
  }
}

void LayerPanel::onMergeDown() {
  if (layerManager_) {
    int activeIndex = layerManager_->activeLayerIndex();
    layerManager_->mergeDown(activeIndex);
  }
}

void LayerPanel::onTreeSelectionChanged() {
  if (!layerManager_ || updatingSelection_)
    return;

  QList<QTreeWidgetItem *> selected = layerTree_->selectedItems();
  if (selected.isEmpty())
    return;

  // Gather all data from tree items BEFORE triggering any signals that
  // could call refreshLayerList() and destroy the tree items.
  QTreeWidgetItem *first = selected.first();
  bool isLayer = first->data(0, IsLayerRole).toBool();
  QUuid layerId(first->data(0, LayerIdRole).toString());

  QList<ItemId> selectedItemIds;
  if (!isLayer) {
    for (QTreeWidgetItem *sel : selected) {
      if (sel->data(0, IsLayerRole).toBool())
        continue;
      selectedItemIds.append(
          ItemId::fromString(sel->data(0, ItemIdRole).toString()));
    }
  }

  // Now safe to trigger signals — tree items may be destroyed after this
  updatingSelection_ = true;
  layerManager_->setActiveLayer(layerId);

  if (!isLayer && canvas_ && canvas_->scene() && itemStore_) {
    canvas_->scene()->clearSelection();
    for (const ItemId &id : selectedItemIds) {
      if (QGraphicsItem *gItem = itemStore_->item(id)) {
        gItem->setSelected(true);
      }
    }
  }

  if (isLayer) {
    emit layerSelected(layerManager_->activeLayerIndex());
  }
  updatingSelection_ = false;

  updateButtonStates();
  updatePropertyControls();
}

void LayerPanel::onOpacityChanged(int value) {
  if (layerManager_) {
    Layer *layer = layerManager_->activeLayer();
    if (layer) {
      layer->setOpacity(value / 100.0);
      opacityLabel_->setText(QString("%1%").arg(value));
    }
  }
}

void LayerPanel::onVisibilityToggled() {
  if (layerManager_) {
    Layer *layer = layerManager_->activeLayer();
    if (layer) {
      layer->setVisible(!layer->isVisible());
      visibilityButton_->setChecked(layer->isVisible());
      refreshLayerList();
    }
  }
}

void LayerPanel::onLockToggled() {
  if (layerManager_) {
    Layer *layer = layerManager_->activeLayer();
    if (layer) {
      layer->setLocked(!layer->isLocked());
      lockButton_->setChecked(layer->isLocked());
      refreshLayerList();
    }
  }
}

void LayerPanel::onTreeItemDropped(QTreeWidgetItem *item, int fromIndex,
                                   int toIndex) {
  Q_UNUSED(item);
  Q_UNUSED(fromIndex);
  Q_UNUSED(toIndex);
  // Handled via dropEvent override in tree widget
}

void LayerPanel::onLayerTreeContextMenuRequested(const QPoint &pos) {
  if (!layerManager_ || !layerTree_) {
    return;
  }

  QTreeWidgetItem *clickedItem = layerTree_->itemAt(pos);
  const QPoint globalPos = layerTree_->viewport()->mapToGlobal(pos);
  QMenu menu(this);

  if (!clickedItem) {
    QAction *addLayerAction = menu.addAction("Add Layer");
    QAction *chosen = menu.exec(globalPos);
    if (chosen == addLayerAction) {
      onAddLayer();
    }
    return;
  }

  // Match common UX: right-click selects the clicked row if it wasn't selected.
  if (!clickedItem->isSelected()) {
    layerTree_->clearSelection();
    clickedItem->setSelected(true);
    layerTree_->setCurrentItem(clickedItem);
  }

  const bool isLayer = clickedItem->data(0, IsLayerRole).toBool();
  const QUuid layerId(clickedItem->data(0, LayerIdRole).toString());

  if (isLayer) {
    Layer *layer = layerManager_->layer(layerId);
    if (!layer) {
      return;
    }
    int layerIndex = -1;
    for (int i = 0; i < layerManager_->layerCount(); ++i) {
      Layer *candidate = layerManager_->layer(i);
      if (candidate && candidate->id() == layerId) {
        layerIndex = i;
        break;
      }
    }

    const int layerCount = layerManager_->layerCount();

    QAction *addLayerAction = menu.addAction("Add Layer");
    menu.addSeparator();
    QAction *renameLayerAction = menu.addAction("Rename Layer...");
    QAction *duplicateLayerAction = menu.addAction("Duplicate Layer");
    QAction *mergeDownAction = menu.addAction("Merge Down");
    QAction *deleteLayerAction = menu.addAction("Delete Layer");
    menu.addSeparator();
    QAction *moveLayerUpAction = menu.addAction("Move Layer Up");
    QAction *moveLayerDownAction = menu.addAction("Move Layer Down");
    menu.addSeparator();

    QAction *toggleVisibilityAction =
        menu.addAction((layer && layer->isVisible()) ? "Hide Layer"
                                                     : "Show Layer");
    toggleVisibilityAction->setCheckable(true);
    toggleVisibilityAction->setChecked(layer && layer->isVisible());

    QAction *toggleLockAction = menu.addAction((layer && layer->isLocked())
                                                   ? "Unlock Layer"
                                                   : "Lock Layer");
    toggleLockAction->setCheckable(true);
    toggleLockAction->setChecked(layer && layer->isLocked());

    deleteLayerAction->setEnabled(layerCount > 1);
    mergeDownAction->setEnabled(layerIndex > 0);
    moveLayerUpAction->setEnabled(layerIndex > 0);
    moveLayerDownAction->setEnabled(layerIndex >= 0 &&
                                    layerIndex < layerCount - 1);

    QAction *chosen = menu.exec(globalPos);
    if (!chosen) {
      return;
    }
    if (chosen == addLayerAction) {
      onAddLayer();
      return;
    }

    if (!layerId.isNull()) {
      layerManager_->setActiveLayer(layerId);
    }

    if (chosen == renameLayerAction) {
      onRenameLayer();
    } else if (chosen == duplicateLayerAction) {
      onDuplicateLayer();
    } else if (chosen == mergeDownAction) {
      onMergeDown();
    } else if (chosen == deleteLayerAction) {
      onDeleteLayer();
    } else if (chosen == moveLayerUpAction) {
      onMoveLayerUp();
    } else if (chosen == moveLayerDownAction) {
      onMoveLayerDown();
    } else if (chosen == toggleVisibilityAction) {
      onVisibilityToggled();
    } else if (chosen == toggleLockAction) {
      onLockToggled();
    }
    return;
  }

  QList<QTreeWidgetItem *> selectedTreeItems = layerTree_->selectedItems();
  QList<ItemId> selectedItemIds;
  bool canBringForward = false;
  bool canSendBackward = false;

  for (QTreeWidgetItem *item : selectedTreeItems) {
    if (!item || item->data(0, IsLayerRole).toBool()) {
      continue;
    }
    ItemId id = ItemId::fromString(item->data(0, ItemIdRole).toString());
    if (!id.isValid()) {
      continue;
    }
    selectedItemIds.append(id);
    if (Layer *owner = layerManager_->findLayerForItem(id)) {
      const int idx = owner->indexOfItem(id);
      if (idx > 0) {
        canSendBackward = true;
      }
      if (idx < owner->itemCount() - 1) {
        canBringForward = true;
      }
    }
  }

  const bool hasItems = !selectedItemIds.isEmpty();
  const bool hasCanvas = canvas_ && canvas_->scene();

  QAction *deleteItemsAction = menu.addAction(
      selectedItemIds.size() > 1 ? "Delete Selected Items" : "Delete Item");
  QAction *mergeItemsAction = menu.addAction("Merge Selected");
  menu.addSeparator();
  QAction *bringToFrontAction = menu.addAction("Bring to Front");
  QAction *bringForwardAction = menu.addAction("Bring Forward");
  QAction *sendBackwardAction = menu.addAction("Send Backward");
  QAction *sendToBackAction = menu.addAction("Send to Back");

  deleteItemsAction->setEnabled(hasItems && hasCanvas);
  mergeItemsAction->setEnabled(hasCanvas && selectedItemIds.size() > 1);
  bringToFrontAction->setEnabled(hasItems && hasCanvas && canBringForward);
  bringForwardAction->setEnabled(hasItems && hasCanvas && canBringForward);
  sendBackwardAction->setEnabled(hasItems && hasCanvas && canSendBackward);
  sendToBackAction->setEnabled(hasItems && hasCanvas && canSendBackward);

  QAction *chosen = menu.exec(globalPos);
  if (!chosen || !hasCanvas) {
    return;
  }

  if (chosen == deleteItemsAction) {
    canvas_->deleteSelectedItems();
  } else if (chosen == mergeItemsAction) {
    canvas_->groupSelectedItems();
  } else if (chosen == bringToFrontAction) {
    canvas_->bringToFront();
  } else if (chosen == bringForwardAction) {
    canvas_->bringForward();
  } else if (chosen == sendBackwardAction) {
    canvas_->sendBackward();
  } else if (chosen == sendToBackAction) {
    canvas_->sendToBack();
  }
}

void LayerPanel::onRenameLayer() {
  if (!layerManager_) {
    return;
  }

  Layer *layer = layerManager_->activeLayer();
  if (!layer) {
    return;
  }

  bool ok = false;
  QString name = QInputDialog::getText(this, "Rename Layer", "Layer name:",
                                       QLineEdit::Normal, layer->name(), &ok);
  if (!ok) {
    return;
  }

  name = name.trimmed();
  if (name.isEmpty() || name == layer->name()) {
    return;
  }

  layer->setName(name);
  refreshLayerList();
}

QString LayerPanel::itemDescription(const ItemId &id) const {
  if (!itemStore_)
    return id.toString().left(8);

  QGraphicsItem *item = itemStore_->item(id);
  if (!item)
    return "(deleted)";

  if (dynamic_cast<QGraphicsRectItem *>(item))
    return "Rectangle";
  if (dynamic_cast<QGraphicsEllipseItem *>(item))
    return "Ellipse";
  if (auto *text = dynamic_cast<QGraphicsTextItem *>(item)) {
    QString t = text->toPlainText().left(20);
    return t.isEmpty() ? "Text" : QString("Text: %1").arg(t);
  }
  if (dynamic_cast<QGraphicsLineItem *>(item))
    return "Line";
  if (dynamic_cast<QGraphicsPathItem *>(item))
    return "Path";
  if (dynamic_cast<QGraphicsPixmapItem *>(item))
    return "Image";
  if (auto *group = dynamic_cast<QGraphicsItemGroup *>(item)) {
    // Detect arrow groups: a line + polygon child pair
    QList<QGraphicsItem *> children = group->childItems();
    if (children.size() == 2) {
      bool hasLine = false, hasPoly = false;
      for (QGraphicsItem *c : children) {
        if (dynamic_cast<QGraphicsLineItem *>(c))
          hasLine = true;
        if (dynamic_cast<QGraphicsPolygonItem *>(c))
          hasPoly = true;
      }
      if (hasLine && hasPoly)
        return "Arrow";
    }
    return "Group";
  }
  if (dynamic_cast<QGraphicsPolygonItem *>(item))
    return "Polygon";

  return "Element";
}
