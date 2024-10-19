#include "tool_panel.h"
#include <QColorDialog>

ToolPanel::ToolPanel(QWidget *parent) : QToolBar(parent) {
  // Pen action
  actionPen = new QAction("Pen", this);
  connect(actionPen, &QAction::triggered, this, &ToolPanel::onActionPen);
  addAction(actionPen);

  // Eraser action
  actionEraser = new QAction("Eraser", this);
  connect(actionEraser, &QAction::triggered, this, &ToolPanel::onActionEraser);
  addAction(actionEraser);

  // Color picker action
  actionColor = new QAction("Color", this);
  connect(actionColor, &QAction::triggered, this, &ToolPanel::onActionColor);
  addAction(actionColor);

  // Rectangle action
  actionRectangle = new QAction("Rectangle", this);
  connect(actionRectangle, &QAction::triggered, this,
          &ToolPanel::onActionRectangle);
  addAction(actionRectangle);

  // Circle action
  actionCircle = new QAction("Circle", this);
  connect(actionCircle, &QAction::triggered, this, &ToolPanel::onActionCircle);
  addAction(actionCircle);

  // Line action
  actionLine = new QAction("Line", this);
  connect(actionLine, &QAction::triggered, this, &ToolPanel::onActionLine);
  addAction(actionLine);

  // Selection action (New)
  actionSelection = new QAction("Selection", this);
  connect(actionSelection, &QAction::triggered, this,
          &ToolPanel::onActionSelection);
  addAction(actionSelection);

  // Increase brush size action
  actionIncreaseBrush = new QAction("Increase Brush", this);
  connect(actionIncreaseBrush, &QAction::triggered, this,
          &ToolPanel::onActionIncreaseBrush);
  addAction(actionIncreaseBrush);

  // Decrease brush size action
  actionDecreaseBrush = new QAction("Decrease Brush", this);
  connect(actionDecreaseBrush, &QAction::triggered, this,
          &ToolPanel::onActionDecreaseBrush);
  addAction(actionDecreaseBrush);

  // Clear canvas action
  actionClear = new QAction("Clear Canvas", this);
  connect(actionClear, &QAction::triggered, this, &ToolPanel::onActionClear);
  addAction(actionClear);

  // Undo action
  actionUndo = new QAction("Undo", this);
  connect(actionUndo, &QAction::triggered, this, &ToolPanel::onActionUndo);
  addAction(actionUndo);

  // Copy action
  QAction *actionCopy = new QAction("Copy", this);
  connect(actionCopy, &QAction::triggered, this, &ToolPanel::copyAction);
  addAction(actionCopy);

  // Cut action
  QAction *actionCut = new QAction("Cut", this);
  connect(actionCut, &QAction::triggered, this, &ToolPanel::cutAction);
  addAction(actionCut);

  // Paste action
  QAction *actionPaste = new QAction("Paste", this);
  connect(actionPaste, &QAction::triggered, this, &ToolPanel::pasteAction);
  addAction(actionPaste);
}

void ToolPanel::onActionPen() { emit penSelected(); }

void ToolPanel::onActionEraser() { emit eraserSelected(); }

void ToolPanel::onActionColor() {
  QColor color = QColorDialog::getColor(Qt::black, this, "Select Color");
  if (color.isValid()) {
    emit colorSelected(color);
  }
}

void ToolPanel::onActionRectangle() {
  emit shapeSelected("Rectangle");
  emit rectangleSelected();
}

void ToolPanel::onActionCircle() {
  emit shapeSelected("Circle");
  emit circleSelected();
}

void ToolPanel::onActionLine() {
  emit shapeSelected("Line");
  emit lineSelected();
}

void ToolPanel::onActionSelection() { // New slot implementation
  emit shapeSelected("Selection");
  emit selectionSelected();
}

void ToolPanel::onActionIncreaseBrush() { emit increaseBrushSize(); }

void ToolPanel::onActionDecreaseBrush() { emit decreaseBrushSize(); }

void ToolPanel::onActionClear() { emit clearCanvas(); }

void ToolPanel::onActionUndo() { emit undoAction(); }
