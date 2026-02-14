/**
 * @file layer_panel.h
 * @brief Layer panel widget for managing layers.
 */
#ifndef LAYER_PANEL_H
#define LAYER_PANEL_H

#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>

class LayerManager;
class Layer;

/**
 * @brief A dock widget panel for managing layers.
 *
 * Provides a visual interface for creating, deleting, reordering,
 * and modifying layer properties.
 */
class LayerPanel : public QDockWidget {
  Q_OBJECT

public:
  explicit LayerPanel(LayerManager *manager, QWidget *parent = nullptr);
  ~LayerPanel() override;

public slots:
  /**
   * @brief Refresh the layer list display
   */
  void refreshLayerList();

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
  void onLayerSelectionChanged();
  void onOpacityChanged(int value);
  void onVisibilityToggled();
  void onLockToggled();

private:
  LayerManager *layerManager_;
  QListWidget *layerList_;
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

  void setupUI();
  void updateButtonStates();
  void updatePropertyControls();
};

#endif // LAYER_PANEL_H
