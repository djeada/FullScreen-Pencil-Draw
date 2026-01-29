#ifndef TOOLPANEL_H
#define TOOLPANEL_H

#include <QAction>
#include <QColor>
#include <QDockWidget>
#include <QLabel>
#include <QSlider>
#include <QToolButton>

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
  void penSelected();
  void eraserSelected();
  void textSelected();
  void fillSelected();
  void arrowSelected();
  void curvedArrowSelected();
  void panSelected();
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
  void copyAction();
  void cutAction();
  void pasteAction();
  void duplicateAction();
  void deleteAction();
  void selectAllAction();

private:
  QAction *actionRectangle;
  QAction *actionCircle;
  QAction *actionLine;
  QAction *actionSelection;
  QAction *actionPen;
  QAction *actionEraser;
  QAction *actionText;
  QAction *actionFill;
  QAction *actionArrow;
  QAction *actionCurvedArrow;
  QAction *actionPan;
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
  void onOpacityChanged(int value);

public slots:
  void onActionRectangle();
  void onActionCircle();
  void onActionLine();
  void onActionSelection();
  void onActionPen();
  void onActionEraser();
  void onActionText();
  void onActionFill();
  void onActionArrow();
  void onActionCurvedArrow();
  void onActionPan();
};

#endif // TOOLPANEL_H
