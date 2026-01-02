#include "tool_panel.h"
#include <QColorDialog>
#include <QFrame>

ToolPanel::ToolPanel(QWidget *parent) : QToolBar(parent) {
  setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  setMovable(false);

  // Active tool display
  activeToolLabel = new QLabel("Tool: Pen", this);
  activeToolLabel->setStyleSheet(
      "QLabel { font-weight: bold; padding: 5px; background-color: #2d2d2d; "
      "color: #ffffff; border-radius: 3px; }");
  addWidget(activeToolLabel);
  addSeparator();

  // Pen action
  actionPen = new QAction("Pen", this);
  actionPen->setToolTip("Draw freehand (P)");
  actionPen->setCheckable(true);
  actionPen->setChecked(true);
  connect(actionPen, &QAction::triggered, this, &ToolPanel::onActionPen);
  addAction(actionPen);

  // Eraser action
  actionEraser = new QAction("Eraser", this);
  actionEraser->setToolTip("Erase items (E)");
  actionEraser->setCheckable(true);
  connect(actionEraser, &QAction::triggered, this, &ToolPanel::onActionEraser);
  addAction(actionEraser);

  addSeparator();

  // Shape tools
  actionLine = new QAction("Line", this);
  actionLine->setToolTip("Draw straight line (L)");
  actionLine->setCheckable(true);
  connect(actionLine, &QAction::triggered, this, &ToolPanel::onActionLine);
  addAction(actionLine);

  actionRectangle = new QAction("Rectangle", this);
  actionRectangle->setToolTip("Draw rectangle (R)");
  actionRectangle->setCheckable(true);
  connect(actionRectangle, &QAction::triggered, this,
          &ToolPanel::onActionRectangle);
  addAction(actionRectangle);

  actionCircle = new QAction("Circle", this);
  actionCircle->setToolTip("Draw circle/ellipse (C)");
  actionCircle->setCheckable(true);
  connect(actionCircle, &QAction::triggered, this, &ToolPanel::onActionCircle);
  addAction(actionCircle);

  actionSelection = new QAction("Select", this);
  actionSelection->setToolTip("Select items (S)");
  actionSelection->setCheckable(true);
  connect(actionSelection, &QAction::triggered, this,
          &ToolPanel::onActionSelection);
  addAction(actionSelection);

  addSeparator();

  // Brush size controls
  actionDecreaseBrush = new QAction("−", this);
  actionDecreaseBrush->setToolTip("Decrease brush size ([)");
  connect(actionDecreaseBrush, &QAction::triggered, this,
          &ToolPanel::onActionDecreaseBrush);
  addAction(actionDecreaseBrush);

  brushSizeLabel = new QLabel("Size: 3", this);
  brushSizeLabel->setStyleSheet(
      "QLabel { padding: 5px; background-color: #3d3d3d; color: #ffffff; "
      "border-radius: 3px; min-width: 50px; }");
  brushSizeLabel->setAlignment(Qt::AlignCenter);
  addWidget(brushSizeLabel);

  actionIncreaseBrush = new QAction("+", this);
  actionIncreaseBrush->setToolTip("Increase brush size (])");
  connect(actionIncreaseBrush, &QAction::triggered, this,
          &ToolPanel::onActionIncreaseBrush);
  addAction(actionIncreaseBrush);

  addSeparator();

  // Color controls
  colorPreview = new QLabel(this);
  colorPreview->setFixedSize(24, 24);
  colorPreview->setStyleSheet(
      "QLabel { background-color: #ffffff; border: 2px solid #888888; "
      "border-radius: 3px; }");
  colorPreview->setToolTip("Current color");
  addWidget(colorPreview);

  actionColor = new QAction("Color", this);
  actionColor->setToolTip("Pick color (K)");
  connect(actionColor, &QAction::triggered, this, &ToolPanel::onActionColor);
  addAction(actionColor);

  addSeparator();

  // Zoom controls
  actionZoomOut = new QAction("Zoom−", this);
  actionZoomOut->setToolTip("Zoom out (-)");
  connect(actionZoomOut, &QAction::triggered, this, &ToolPanel::onActionZoomOut);
  addAction(actionZoomOut);

  actionZoomIn = new QAction("Zoom+", this);
  actionZoomIn->setToolTip("Zoom in (+)");
  connect(actionZoomIn, &QAction::triggered, this, &ToolPanel::onActionZoomIn);
  addAction(actionZoomIn);

  addSeparator();

  // Edit actions
  actionUndo = new QAction("Undo", this);
  actionUndo->setToolTip("Undo last action (Ctrl+Z)");
  connect(actionUndo, &QAction::triggered, this, &ToolPanel::onActionUndo);
  addAction(actionUndo);

  actionRedo = new QAction("Redo", this);
  actionRedo->setToolTip("Redo last undone action (Ctrl+Y)");
  connect(actionRedo, &QAction::triggered, this, &ToolPanel::onActionRedo);
  addAction(actionRedo);

  addSeparator();

  // Copy action
  QAction *actionCopy = new QAction("Copy", this);
  actionCopy->setToolTip("Copy selected items (Ctrl+C)");
  connect(actionCopy, &QAction::triggered, this, &ToolPanel::copyAction);
  addAction(actionCopy);

  // Cut action
  QAction *actionCut = new QAction("Cut", this);
  actionCut->setToolTip("Cut selected items (Ctrl+X)");
  connect(actionCut, &QAction::triggered, this, &ToolPanel::cutAction);
  addAction(actionCut);

  // Paste action
  QAction *actionPaste = new QAction("Paste", this);
  actionPaste->setToolTip("Paste items (Ctrl+V)");
  connect(actionPaste, &QAction::triggered, this, &ToolPanel::pasteAction);
  addAction(actionPaste);

  addSeparator();

  // File actions
  actionSave = new QAction("Save", this);
  actionSave->setToolTip("Save canvas to file (Ctrl+S)");
  connect(actionSave, &QAction::triggered, this, &ToolPanel::onActionSave);
  addAction(actionSave);

  actionClear = new QAction("Clear", this);
  actionClear->setToolTip("Clear canvas");
  connect(actionClear, &QAction::triggered, this, &ToolPanel::onActionClear);
  addAction(actionClear);
}

void ToolPanel::clearActiveToolStyles() {
  actionPen->setChecked(false);
  actionEraser->setChecked(false);
  actionLine->setChecked(false);
  actionRectangle->setChecked(false);
  actionCircle->setChecked(false);
  actionSelection->setChecked(false);
}

void ToolPanel::setActiveTool(const QString &toolName) {
  activeToolLabel->setText("Tool: " + toolName);
}

void ToolPanel::updateBrushSizeDisplay(int size) {
  brushSizeLabel->setText(QString("Size: %1").arg(size));
}

void ToolPanel::updateColorDisplay(const QColor &color) {
  colorPreview->setStyleSheet(QString("QLabel { background-color: %1; border: "
                                       "2px solid #888888; border-radius: 3px; }")
                                   .arg(color.name()));
}

void ToolPanel::onActionPen() {
  clearActiveToolStyles();
  actionPen->setChecked(true);
  setActiveTool("Pen");
  emit penSelected();
}

void ToolPanel::onActionEraser() {
  clearActiveToolStyles();
  actionEraser->setChecked(true);
  setActiveTool("Eraser");
  emit eraserSelected();
}

void ToolPanel::onActionColor() {
  QColor color = QColorDialog::getColor(Qt::white, this, "Select Color");
  if (color.isValid()) {
    updateColorDisplay(color);
    emit colorSelected(color);
  }
}

void ToolPanel::onActionRectangle() {
  clearActiveToolStyles();
  actionRectangle->setChecked(true);
  setActiveTool("Rectangle");
  emit shapeSelected("Rectangle");
  emit rectangleSelected();
}

void ToolPanel::onActionCircle() {
  clearActiveToolStyles();
  actionCircle->setChecked(true);
  setActiveTool("Circle");
  emit shapeSelected("Circle");
  emit circleSelected();
}

void ToolPanel::onActionLine() {
  clearActiveToolStyles();
  actionLine->setChecked(true);
  setActiveTool("Line");
  emit shapeSelected("Line");
  emit lineSelected();
}

void ToolPanel::onActionSelection() {
  clearActiveToolStyles();
  actionSelection->setChecked(true);
  setActiveTool("Select");
  emit shapeSelected("Selection");
  emit selectionSelected();
}

void ToolPanel::onActionIncreaseBrush() { emit increaseBrushSize(); }

void ToolPanel::onActionDecreaseBrush() { emit decreaseBrushSize(); }

void ToolPanel::onActionClear() { emit clearCanvas(); }

void ToolPanel::onActionUndo() { emit undoAction(); }

void ToolPanel::onActionRedo() { emit redoAction(); }

void ToolPanel::onActionSave() { emit saveAction(); }

void ToolPanel::onActionZoomIn() { emit zoomInAction(); }

void ToolPanel::onActionZoomOut() { emit zoomOutAction(); }
