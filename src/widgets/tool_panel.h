#ifndef TOOLPANEL_H
#define TOOLPANEL_H

#include <QAction>
#include <QColor>
#include <QToolBar>

class ToolPanel : public QToolBar {
  Q_OBJECT

public:
  explicit ToolPanel(QWidget *parent = nullptr);

signals:
  // Signals for shape selections
  void shapeSelected(const QString &shapeType);
  void rectangleSelected();
  void circleSelected();
  void lineSelected();
  void selectionSelected(); // New signal for selection

  // Other tool signals
  void penSelected();
  void eraserSelected();
  void colorSelected(const QColor &color);
  void increaseBrushSize();
  void decreaseBrushSize();
  void clearCanvas();
  void undoAction();
  void copyAction();
  void cutAction();
  void pasteAction();

private:
  // Actions for shapes
  QAction *actionRectangle;
  QAction *actionCircle;
  QAction *actionLine;
  QAction *actionSelection; // New QAction for selection

  // Actions for other tools
  QAction *actionPen;
  QAction *actionEraser;
  QAction *actionColor;
  QAction *actionIncreaseBrush;
  QAction *actionDecreaseBrush;
  QAction *actionClear;
  QAction *actionUndo;

private slots:
  // Slots for shape actions
  void onActionRectangle();
  void onActionCircle();
  void onActionLine();
  void onActionSelection(); // New slot for selection

  // Slots for other tools
  void onActionPen();
  void onActionEraser();
  void onActionColor();
  void onActionIncreaseBrush();
  void onActionDecreaseBrush();
  void onActionClear();
  void onActionUndo();
};

#endif // TOOLPANEL_H
