#ifndef TOOLPANEL_H
#define TOOLPANEL_H

#include <QAction>
#include <QToolBar>
#include <QColor>

class ToolPanel : public QToolBar {
  Q_OBJECT

public:
  explicit ToolPanel(QWidget *parent = nullptr);

signals:
  void penSelected();
  void eraserSelected();
  void colorSelected(const QColor &color);
  void rectangleSelected();
  void circleSelected();
  void lineSelected();
  void increaseBrushSize();
  void decreaseBrushSize();
  void clearCanvas();
  void undoAction();

private:
  QAction *actionPen;
  QAction *actionEraser;
  QAction *actionColor;
  QAction *actionRectangle;
  QAction *actionCircle;
  QAction *actionLine;
  QAction *actionIncreaseBrush;
  QAction *actionDecreaseBrush;
  QAction *actionClear;
  QAction *actionUndo;

private slots:
  void onActionPen();
  void onActionEraser();
  void onActionColor();
  void onActionRectangle();
  void onActionCircle();
  void onActionLine();
  void onActionIncreaseBrush();
  void onActionDecreaseBrush();
  void onActionClear();
  void onActionUndo();
};

#endif // TOOLPANEL_H
