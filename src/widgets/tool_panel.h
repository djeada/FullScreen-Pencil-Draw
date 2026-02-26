#ifndef TOOLPANEL_H
#define TOOLPANEL_H

#include <QAction>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QSlider>
#include <QToolButton>

#include "../core/brush_tip.h"

class BrushPreview;

class ToolPanel : public QDockWidget {
  Q_OBJECT

public:
  explicit ToolPanel(QWidget *parent = nullptr);
  void updateBrushSizeDisplay(int size);
  void updateColorDisplay(const QColor &color);
  void updateZoomDisplay(double zoom);
  void updateOpacityDisplay(int opacity);
  void updatePositionDisplay(const QPointF &pos);
  void updateFilledShapesDisplay(bool filled);
  void setActiveTool(const QString &toolName);

signals:
  void shapeSelected(const QString &shapeType);
  void rectangleSelected();
  void circleSelected();
  void lineSelected();
  void selectionSelected();
  void lassoSelectionSelected();
  void penSelected();
  void eraserSelected();
  void textSelected();
  void fillSelected();
  void colorSelectSelected();
  void arrowSelected();
  void curvedArrowSelected();
  void panSelected();
  void mermaidSelected();
  void bezierSelected();
  void textOnPathSelected();
  void colorSelected(const QColor &color);
  void opacitySelected(int opacity);
  void increaseBrushSize();
  void decreaseBrushSize();
  void clearCanvas();
  void undoAction();
  void redoAction();
  void saveAction();
  void openAction();
  void newCanvasAction();
  void zoomInAction();
  void zoomOutAction();
  void zoomResetAction();
  void toggleGridAction();
  void toggleFilledShapesAction();
  void fillBrushSelected(const QBrush &brush);
  void copyAction();
  void cutAction();
  void pasteAction();
  void duplicateAction();
  void deleteAction();
  void selectAllAction();
  void pressureSensitivityToggled();
  void brushTipSelected(const BrushTip &tip);

private:
  QAction *actionRectangle;
  QAction *actionCircle;
  QAction *actionLine;
  QAction *actionSelection;
  QAction *actionLassoSelection;
  QAction *actionPen;
  QAction *actionEraser;
  QAction *actionText;
  QAction *actionMermaid;
  QAction *actionFill;
  QAction *actionColorSelect;
  QAction *actionArrow;
  QAction *actionCurvedArrow;
  QAction *actionPan;
  QAction *actionBezier;
  QAction *actionTextOnPath;
  QAction *actionIncreaseBrush;
  QAction *actionDecreaseBrush;
  QAction *actionClear;
  QAction *actionUndo;
  QAction *actionRedo;
  QAction *actionSave;
  QAction *actionOpen;
  QAction *actionNew;
  QAction *actionZoomIn;
  QAction *actionZoomOut;
  QAction *actionZoomReset;
  QAction *actionGrid;
  QAction *actionFilledShapes;

  QLabel *brushSizeLabel;
  QLabel *colorPreview;
  QLabel *activeToolLabel;
  QLabel *zoomLabel;
  QLabel *opacityLabel;
  QLabel *positionLabel;
  QSlider *opacitySlider;
  BrushPreview *brushPreview_;
  QCheckBox *pressureSensitivityCheckBox_;
  QComboBox *fillStyleCombo_;
  QComboBox *brushTipCombo_;
  QColor currentColor_;

  void clearActiveToolStyles();
  bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
  void onActionColor();
  void onActionIncreaseBrush();
  void onActionDecreaseBrush();
  void onActionClear();
  void onActionUndo();
  void onActionRedo();
  void onActionSave();
  void onActionOpen();
  void onActionNew();
  void onActionZoomIn();
  void onActionZoomOut();
  void onActionZoomReset();
  void onActionGrid();
  void onActionFilledShapes();
  void onFillStyleChanged(int index);
  void onBrushTipChanged(int index);
  void onOpacityChanged(int value);

public slots:
  void onActionRectangle();
  void onActionCircle();
  void onActionLine();
  void onActionSelection();
  void onActionLassoSelection();
  void onActionPen();
  void onActionEraser();
  void onActionText();
  void onActionMermaid();
  void onActionFill();
  void onActionColorSelect();
  void onActionArrow();
  void onActionCurvedArrow();
  void onActionPan();
  void onActionBezier();
  void onActionTextOnPath();
};

#endif // TOOLPANEL_H
