/**
 * @file layer_panel.cpp
 * @brief Implementation of the layer panel widget.
 */
#include "layer_panel.h"
#include "../core/layer.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

LayerPanel::LayerPanel(LayerManager *manager, QWidget *parent)
    : QDockWidget("Layers", parent), layerManager_(manager) {
  setupUI();
  refreshLayerList();

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
  }
}

LayerPanel::~LayerPanel() = default;

void LayerPanel::setupUI() {
  QWidget *container = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(container);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(4);

  // Layer list
  layerList_ = new QListWidget(container);
  layerList_->setDragDropMode(QAbstractItemView::InternalMove);
  layerList_->setSelectionMode(QAbstractItemView::SingleSelection);
  layerList_->setMaximumHeight(200);
  connect(layerList_, &QListWidget::currentRowChanged, this,
          &LayerPanel::onLayerSelectionChanged);
  mainLayout->addWidget(layerList_);

  // Layer controls row 1
  QHBoxLayout *controlsRow1 = new QHBoxLayout();
  controlsRow1->setSpacing(2);

  addButton_ = new QPushButton("+", container);
  addButton_->setToolTip("Add new layer");
  addButton_->setMaximumWidth(30);
  connect(addButton_, &QPushButton::clicked, this, &LayerPanel::onAddLayer);
  controlsRow1->addWidget(addButton_);

  deleteButton_ = new QPushButton("-", container);
  deleteButton_->setToolTip("Delete layer");
  deleteButton_->setMaximumWidth(30);
  connect(deleteButton_, &QPushButton::clicked, this, &LayerPanel::onDeleteLayer);
  controlsRow1->addWidget(deleteButton_);

  duplicateButton_ = new QPushButton("D", container);
  duplicateButton_->setToolTip("Duplicate layer");
  duplicateButton_->setMaximumWidth(30);
  connect(duplicateButton_, &QPushButton::clicked, this, &LayerPanel::onDuplicateLayer);
  controlsRow1->addWidget(duplicateButton_);

  mergeButton_ = new QPushButton("M", container);
  mergeButton_->setToolTip("Merge down");
  mergeButton_->setMaximumWidth(30);
  connect(mergeButton_, &QPushButton::clicked, this, &LayerPanel::onMergeDown);
  controlsRow1->addWidget(mergeButton_);

  controlsRow1->addStretch();
  mainLayout->addLayout(controlsRow1);

  // Layer controls row 2
  QHBoxLayout *controlsRow2 = new QHBoxLayout();
  controlsRow2->setSpacing(2);

  moveUpButton_ = new QPushButton("^", container);
  moveUpButton_->setToolTip("Move layer up");
  moveUpButton_->setMaximumWidth(30);
  connect(moveUpButton_, &QPushButton::clicked, this, &LayerPanel::onMoveLayerUp);
  controlsRow2->addWidget(moveUpButton_);

  moveDownButton_ = new QPushButton("v", container);
  moveDownButton_->setToolTip("Move layer down");
  moveDownButton_->setMaximumWidth(30);
  connect(moveDownButton_, &QPushButton::clicked, this, &LayerPanel::onMoveLayerDown);
  controlsRow2->addWidget(moveDownButton_);

  visibilityButton_ = new QPushButton("V", container);
  visibilityButton_->setToolTip("Toggle visibility");
  visibilityButton_->setMaximumWidth(30);
  visibilityButton_->setCheckable(true);
  connect(visibilityButton_, &QPushButton::clicked, this, &LayerPanel::onVisibilityToggled);
  controlsRow2->addWidget(visibilityButton_);

  lockButton_ = new QPushButton("L", container);
  lockButton_->setToolTip("Toggle lock");
  lockButton_->setMaximumWidth(30);
  lockButton_->setCheckable(true);
  connect(lockButton_, &QPushButton::clicked, this, &LayerPanel::onLockToggled);
  controlsRow2->addWidget(lockButton_);

  controlsRow2->addStretch();
  mainLayout->addLayout(controlsRow2);

  // Opacity control
  QGroupBox *opacityGroup = new QGroupBox("Opacity", container);
  QHBoxLayout *opacityLayout = new QHBoxLayout(opacityGroup);
  opacityLayout->setContentsMargins(4, 4, 4, 4);

  opacitySlider_ = new QSlider(Qt::Horizontal, opacityGroup);
  opacitySlider_->setRange(0, 100);
  opacitySlider_->setValue(100);
  connect(opacitySlider_, &QSlider::valueChanged, this, &LayerPanel::onOpacityChanged);
  opacityLayout->addWidget(opacitySlider_);

  opacityLabel_ = new QLabel("100%", opacityGroup);
  opacityLabel_->setMinimumWidth(35);
  opacityLayout->addWidget(opacityLabel_);

  mainLayout->addWidget(opacityGroup);

  mainLayout->addStretch();
  
  container->setLayout(mainLayout);
  setWidget(container);
  setMinimumWidth(180);
  setMaximumWidth(250);

  // Style
  setStyleSheet(R"(
    QDockWidget {
      background-color: #2d2d2d;
      color: #ffffff;
    }
    QDockWidget::title {
      background-color: #3d3d3d;
      padding: 6px;
    }
    QListWidget {
      background-color: #1d1d1d;
      color: #ffffff;
      border: 1px solid #4d4d4d;
    }
    QListWidget::item:selected {
      background-color: #0066cc;
    }
    QPushButton {
      background-color: #3d3d3d;
      color: #ffffff;
      border: 1px solid #4d4d4d;
      padding: 4px;
      min-height: 20px;
    }
    QPushButton:hover {
      background-color: #4d4d4d;
    }
    QPushButton:pressed {
      background-color: #2d2d2d;
    }
    QPushButton:checked {
      background-color: #0066cc;
    }
    QGroupBox {
      color: #ffffff;
      border: 1px solid #4d4d4d;
      margin-top: 6px;
      padding-top: 6px;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 6px;
      padding: 0 3px;
    }
    QSlider::groove:horizontal {
      background: #3d3d3d;
      height: 4px;
      border-radius: 2px;
    }
    QSlider::handle:horizontal {
      background: #0066cc;
      width: 12px;
      margin: -4px 0;
      border-radius: 6px;
    }
    QLabel {
      color: #ffffff;
    }
  )");
}

void LayerPanel::refreshLayerList() {
  if (!layerManager_) return;

  int currentRow = layerList_->currentRow();
  layerList_->clear();

  // Add layers in reverse order (top layer first in list)
  for (int i = layerManager_->layerCount() - 1; i >= 0; --i) {
    Layer *layer = layerManager_->layer(i);
    if (layer) {
      QString prefix = layer->isVisible() ? "[V] " : "[ ] ";
      if (layer->isLocked()) {
        prefix += "[L] ";
      }
      layerList_->addItem(prefix + layer->name());
    }
  }

  // Restore or set selection
  int activeIndex = layerManager_->activeLayerIndex();
  if (activeIndex >= 0) {
    // Convert to list index (reversed)
    int listIndex = layerManager_->layerCount() - 1 - activeIndex;
    layerList_->setCurrentRow(listIndex);
  } else if (currentRow >= 0 && currentRow < layerList_->count()) {
    layerList_->setCurrentRow(currentRow);
  }

  updateButtonStates();
  updatePropertyControls();
}

void LayerPanel::updateButtonStates() {
  if (!layerManager_) return;

  int layerCount = layerManager_->layerCount();
  int activeIndex = layerManager_->activeLayerIndex();

  deleteButton_->setEnabled(layerCount > 1);
  moveUpButton_->setEnabled(activeIndex > 0);
  moveDownButton_->setEnabled(activeIndex < layerCount - 1 && activeIndex >= 0);
  mergeButton_->setEnabled(activeIndex > 0);
  duplicateButton_->setEnabled(activeIndex >= 0);
}

void LayerPanel::updatePropertyControls() {
  if (!layerManager_) return;

  Layer *layer = layerManager_->activeLayer();
  if (layer) {
    opacitySlider_->blockSignals(true);
    opacitySlider_->setValue(static_cast<int>(layer->opacity() * 100));
    opacitySlider_->blockSignals(false);
    opacityLabel_->setText(QString("%1%").arg(static_cast<int>(layer->opacity() * 100)));
    
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
  if (!layerManager_) return;
  
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

void LayerPanel::onLayerSelectionChanged() {
  if (!layerManager_) return;

  int listIndex = layerList_->currentRow();
  if (listIndex >= 0) {
    // Convert from list index (reversed) to layer index
    int layerIndex = layerManager_->layerCount() - 1 - listIndex;
    layerManager_->setActiveLayer(layerIndex);
    emit layerSelected(layerIndex);
  }
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
