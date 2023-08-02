#include "tool_panel.h"

ToolPanel::ToolPanel(QWidget *parent) : QToolBar(parent) {
  actionPen = new QAction(QIcon(":/icons/pen.png"), "Pen",
                          this); // Assuming you have an icon file at the path
  connect(actionPen, &QAction::triggered, this, &ToolPanel::onActionPen);
  addAction(actionPen);

  actionEraser =
      new QAction(QIcon(":/icons/eraser.png"), "Eraser",
                  this); // Assuming you have an icon file at the path
  connect(actionEraser, &QAction::triggered, this, &ToolPanel::onActionEraser);
  addAction(actionEraser);
}

void ToolPanel::onActionPen() { emit penSelected(); }

void ToolPanel::onActionEraser() { emit eraserSelected(); }
