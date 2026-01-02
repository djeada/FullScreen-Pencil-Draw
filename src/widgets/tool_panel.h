#ifndef TOOLPANEL_H
#define TOOLPANEL_H

#include <QAction>
#include <QColor>
#include <QLabel>
#include <QToolBar>

class ToolPanel : public QToolBar {
  Q_OBJECT

public:
  explicit ToolPanel(QWidget *parent = nullptr);
  void updateBrushSizeDisplay(int size);
  void updateColorDisplay(const QColor &color);
  void setActiveTool(const QString &toolName);

signals:
  // Signals for shape selections
  void shapeSelected(const QString &shapeType);
  void rectangleSelected();
  void circleSelected();
  void lineSelected();
  void selectionSelected();

  // Other tool signals
  void penSelected();
  void eraserSelected();
  void colorSelected(const QColor &color);
  void increaseBrushSize();
  void decreaseBrushSize();
  void clearCanvas();
  void undoAction();
  void redoAction();
  void saveAction();
  void zoomInAction();
  void zoomOutAction();
  void copyAction();
  void cutAction();
  void pasteAction();

private:
  // Actions for shapes
  QAction *actionRectangle;
  QAction *actionCircle;
  QAction *actionLine;
  QAction *actionSelection;

  // Actions for other tools
  QAction *actionPen;
  QAction *actionEraser;
  QAction *actionColor;
  QAction *actionIncreaseBrush;
  QAction *actionDecreaseBrush;
  QAction *actionClear;
  QAction *actionUndo;
  QAction *actionRedo;
  QAction *actionSave;
  QAction *actionZoomIn;
  QAction *actionZoomOut;

  // Status displays
  QLabel *brushSizeLabel;
  QLabel *colorPreview;
  QLabel *activeToolLabel;

  // Helper method
  void clearActiveToolStyles();

private slots:
  void onActionColor();
  void onActionIncreaseBrush();
  void onActionDecreaseBrush();
  void onActionClear();
  void onActionUndo();
  void onActionRedo();
  void onActionSave();
  void onActionZoomIn();
  void onActionZoomOut();

public slots:
  // Slots for shape actions (public for keyboard shortcuts)
  void onActionRectangle();
  void onActionCircle();
  void onActionLine();
  void onActionSelection();
  void onActionPen();
  void onActionEraser();
};

#endif // TOOLPANEL_H
