#ifndef TOOLPANEL_H
#define TOOLPANEL_H

#include <QAction>
#include <QToolBar>

class ToolPanel : public QToolBar {
  Q_OBJECT

public:
  explicit ToolPanel(QWidget *parent = nullptr);

signals:
  void penSelected();
  void eraserSelected();

private:
  QAction *actionPen;
  QAction *actionEraser;

private slots:
  void onActionPen();
  void onActionEraser();
};

#endif // TOOLPANEL_H
