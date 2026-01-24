#include "tool_panel.h"
#include "brush_preview.h"
#include <QColorDialog>
#include <QFrame>

ToolPanel::ToolPanel(QWidget *parent) : QToolBar(parent), brushPreview_(nullptr) {
  setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  setMovable(false);
  setIconSize(QSize(22, 22));

  // Active tool display with modern styling and gradient
  activeToolLabel = new QLabel("✎ Pen", this);
  activeToolLabel->setStyleSheet(R"(
    QLabel { 
      font-weight: 600; 
      padding: 10px 16px; 
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa); 
      color: #ffffff; 
      border-radius: 8px; 
      font-size: 13px;
      border: 1px solid rgba(255, 255, 255, 0.15);
    }
  )");
  addWidget(activeToolLabel);
  addSeparator();

  // Drawing tools with Unicode icons
  actionPen = new QAction("✎ Pen", this);
  actionPen->setToolTip("Freehand draw (P)");
  actionPen->setCheckable(true);
  actionPen->setChecked(true);
  connect(actionPen, &QAction::triggered, this, &ToolPanel::onActionPen);
  addAction(actionPen);

  actionEraser = new QAction("◯ Eraser", this);
  actionEraser->setToolTip("Erase items (E)");
  actionEraser->setCheckable(true);
  connect(actionEraser, &QAction::triggered, this, &ToolPanel::onActionEraser);
  addAction(actionEraser);

  actionText = new QAction("T Text", this);
  actionText->setToolTip("Add text (T)");
  actionText->setCheckable(true);
  connect(actionText, &QAction::triggered, this, &ToolPanel::onActionText);
  addAction(actionText);

  actionFill = new QAction("◉ Fill", this);
  actionFill->setToolTip("Fill shape (F)");
  actionFill->setCheckable(true);
  connect(actionFill, &QAction::triggered, this, &ToolPanel::onActionFill);
  addAction(actionFill);

  addSeparator();

  // Shape tools
  actionLine = new QAction("╱ Line", this);
  actionLine->setToolTip("Draw line (L)");
  actionLine->setCheckable(true);
  connect(actionLine, &QAction::triggered, this, &ToolPanel::onActionLine);
  addAction(actionLine);

  actionArrow = new QAction("➤ Arrow", this);
  actionArrow->setToolTip("Draw arrow (A)");
  actionArrow->setCheckable(true);
  connect(actionArrow, &QAction::triggered, this, &ToolPanel::onActionArrow);
  addAction(actionArrow);

  actionRectangle = new QAction("▢ Rect", this);
  actionRectangle->setToolTip("Draw rectangle (R)");
  actionRectangle->setCheckable(true);
  connect(actionRectangle, &QAction::triggered, this, &ToolPanel::onActionRectangle);
  addAction(actionRectangle);

  actionCircle = new QAction("◯ Circle", this);
  actionCircle->setToolTip("Draw circle (C)");
  actionCircle->setCheckable(true);
  connect(actionCircle, &QAction::triggered, this, &ToolPanel::onActionCircle);
  addAction(actionCircle);

  addSeparator();

  // Navigation tools
  actionSelection = new QAction("⬚ Select", this);
  actionSelection->setToolTip("Select items (S)");
  actionSelection->setCheckable(true);
  connect(actionSelection, &QAction::triggered, this, &ToolPanel::onActionSelection);
  addAction(actionSelection);

  actionPan = new QAction("☰ Pan", this);
  actionPan->setToolTip("Pan canvas (H)");
  actionPan->setCheckable(true);
  connect(actionPan, &QAction::triggered, this, &ToolPanel::onActionPan);
  addAction(actionPan);

  addSeparator();

  // Brush controls
  actionDecreaseBrush = new QAction("−", this);
  actionDecreaseBrush->setToolTip("Decrease size ([)");
  connect(actionDecreaseBrush, &QAction::triggered, this, &ToolPanel::onActionDecreaseBrush);
  addAction(actionDecreaseBrush);

  brushSizeLabel = new QLabel("Size: 3", this);
  brushSizeLabel->setStyleSheet(R"(
    QLabel { 
      padding: 8px 12px; 
      background-color: rgba(255, 255, 255, 0.06); 
      color: #f8f8fc; 
      border-radius: 8px; 
      border: 1px solid rgba(255, 255, 255, 0.08);
      min-width: 60px;
      font-weight: 500;
    }
  )");
  brushSizeLabel->setAlignment(Qt::AlignCenter);
  addWidget(brushSizeLabel);

  actionIncreaseBrush = new QAction("+", this);
  actionIncreaseBrush->setToolTip("Increase size (])");
  connect(actionIncreaseBrush, &QAction::triggered, this, &ToolPanel::onActionIncreaseBrush);
  addAction(actionIncreaseBrush);

  // Visual brush preview
  brushPreview_ = new BrushPreview(this);
  addWidget(brushPreview_);

  addSeparator();

  // Color & opacity
  colorPreview = new QLabel(this);
  colorPreview->setFixedSize(32, 32);
  colorPreview->setStyleSheet(R"(
    QLabel { 
      background-color: #ffffff; 
      border: 2px solid rgba(255, 255, 255, 0.15); 
      border-radius: 8px; 
    }
  )");
  colorPreview->setToolTip("Current color");
  addWidget(colorPreview);

  actionColor = new QAction("🎨 Color", this);
  actionColor->setToolTip("Pick color (K)");
  connect(actionColor, &QAction::triggered, this, &ToolPanel::onActionColor);
  addAction(actionColor);

  opacityLabel = new QLabel("Opacity", this);
  opacityLabel->setStyleSheet("QLabel { color: #a0a0a8; padding: 6px; font-size: 11px; font-weight: 500; }");
  addWidget(opacityLabel);

  opacitySlider = new QSlider(Qt::Horizontal, this);
  opacitySlider->setRange(0, 255);
  opacitySlider->setValue(255);
  opacitySlider->setMaximumWidth(70);
  opacitySlider->setToolTip("Brush opacity");
  connect(opacitySlider, &QSlider::valueChanged, this, &ToolPanel::onOpacityChanged);
  addWidget(opacitySlider);

  addSeparator();

  // Zoom controls
  actionZoomOut = new QAction("−", this);
  actionZoomOut->setToolTip("Zoom out (-)");
  connect(actionZoomOut, &QAction::triggered, this, &ToolPanel::onActionZoomOut);
  addAction(actionZoomOut);

  zoomLabel = new QLabel("100%", this);
  zoomLabel->setStyleSheet(R"(
    QLabel { 
      padding: 8px 12px; 
      background-color: rgba(255, 255, 255, 0.06); 
      color: #f8f8fc; 
      border-radius: 8px; 
      border: 1px solid rgba(255, 255, 255, 0.08);
      min-width: 55px;
      font-weight: 500;
    }
  )");
  zoomLabel->setAlignment(Qt::AlignCenter);
  addWidget(zoomLabel);

  actionZoomIn = new QAction("+", this);
  actionZoomIn->setToolTip("Zoom in (+)");
  connect(actionZoomIn, &QAction::triggered, this, &ToolPanel::onActionZoomIn);
  addAction(actionZoomIn);

  actionZoomReset = new QAction("⟲ 1:1", this);
  actionZoomReset->setToolTip("Reset zoom (0)");
  connect(actionZoomReset, &QAction::triggered, this, &ToolPanel::onActionZoomReset);
  addAction(actionZoomReset);

  actionGrid = new QAction("⊞ Grid", this);
  actionGrid->setToolTip("Toggle grid (G)");
  actionGrid->setCheckable(true);
  connect(actionGrid, &QAction::triggered, this, &ToolPanel::onActionGrid);
  addAction(actionGrid);

  actionFilledShapes = new QAction("◼ Filled", this);
  actionFilledShapes->setToolTip("Toggle filled shapes (B)");
  actionFilledShapes->setCheckable(true);
  connect(actionFilledShapes, &QAction::triggered, this, &ToolPanel::onActionFilledShapes);
  addAction(actionFilledShapes);

  addSeparator();

  // Edit actions
  actionUndo = new QAction("↶ Undo", this);
  actionUndo->setToolTip("Undo (Ctrl+Z)");
  connect(actionUndo, &QAction::triggered, this, &ToolPanel::onActionUndo);
  addAction(actionUndo);

  actionRedo = new QAction("↷ Redo", this);
  actionRedo->setToolTip("Redo (Ctrl+Y)");
  connect(actionRedo, &QAction::triggered, this, &ToolPanel::onActionRedo);
  addAction(actionRedo);

  addSeparator();

  QAction *actionCopy = new QAction("⧉ Copy", this);
  actionCopy->setToolTip("Copy (Ctrl+C)");
  connect(actionCopy, &QAction::triggered, this, &ToolPanel::copyAction);
  addAction(actionCopy);

  QAction *actionCut = new QAction("✂ Cut", this);
  actionCut->setToolTip("Cut (Ctrl+X)");
  connect(actionCut, &QAction::triggered, this, &ToolPanel::cutAction);
  addAction(actionCut);

  QAction *actionPaste = new QAction("📋 Paste", this);
  actionPaste->setToolTip("Paste (Ctrl+V)");
  connect(actionPaste, &QAction::triggered, this, &ToolPanel::pasteAction);
  addAction(actionPaste);

  QAction *actionDuplicate = new QAction("⊕ Dup", this);
  actionDuplicate->setToolTip("Duplicate (Ctrl+D)");
  connect(actionDuplicate, &QAction::triggered, this, &ToolPanel::duplicateAction);
  addAction(actionDuplicate);

  QAction *actionDelete = new QAction("✕ Del", this);
  actionDelete->setToolTip("Delete (Del)");
  connect(actionDelete, &QAction::triggered, this, &ToolPanel::deleteAction);
  addAction(actionDelete);

  addSeparator();

  // File actions
  actionNew = new QAction("📄 New", this);
  actionNew->setToolTip("New canvas (Ctrl+N)");
  connect(actionNew, &QAction::triggered, this, &ToolPanel::onActionNew);
  addAction(actionNew);

  actionOpen = new QAction("📂 Open", this);
  actionOpen->setToolTip("Open image (Ctrl+O)");
  connect(actionOpen, &QAction::triggered, this, &ToolPanel::onActionOpen);
  addAction(actionOpen);

  actionSave = new QAction("💾 Save", this);
  actionSave->setToolTip("Save (Ctrl+S)");
  connect(actionSave, &QAction::triggered, this, &ToolPanel::onActionSave);
  addAction(actionSave);

  actionClear = new QAction("🗑 Clear", this);
  actionClear->setToolTip("Clear canvas");
  connect(actionClear, &QAction::triggered, this, &ToolPanel::onActionClear);
  addAction(actionClear);

  addSeparator();

  // Position display with modern styling
  positionLabel = new QLabel("X: 0  Y: 0", this);
  positionLabel->setStyleSheet(R"(
    QLabel { 
      padding: 8px 12px; 
      background-color: rgba(0, 0, 0, 0.3); 
      color: #a0a0a8; 
      border-radius: 8px; 
      border: 1px solid rgba(255, 255, 255, 0.05);
      min-width: 100px;
      font-size: 11px;
      font-weight: 500;
    }
  )");
  positionLabel->setAlignment(Qt::AlignCenter);
  addWidget(positionLabel);
}

void ToolPanel::clearActiveToolStyles() {
  actionPen->setChecked(false);
  actionEraser->setChecked(false);
  actionText->setChecked(false);
  actionFill->setChecked(false);
  actionLine->setChecked(false);
  actionArrow->setChecked(false);
  actionRectangle->setChecked(false);
  actionCircle->setChecked(false);
  actionSelection->setChecked(false);
  actionPan->setChecked(false);
}

void ToolPanel::setActiveTool(const QString &toolName) { 
  // Map tool names to Unicode icons for display
  static const QHash<QString, QString> toolIcons = {
    {"Pen", "✎"}, {"Eraser", "◯"}, {"Text", "T"}, {"Fill", "◉"},
    {"Line", "╱"}, {"Arrow", "➤"}, {"Rectangle", "▢"}, {"Circle", "◯"},
    {"Select", "⬚"}, {"Pan", "☰"}
  };
  QString icon = toolIcons.value(toolName, "•");
  activeToolLabel->setText(icon + " " + toolName); 
}

void ToolPanel::updateBrushSizeDisplay(int size) {
  brushSizeLabel->setText(QString("Size: %1").arg(size));
  if (brushPreview_) {
    brushPreview_->setBrushSize(size);
  }
}

void ToolPanel::updateColorDisplay(const QColor &color) {
  colorPreview->setStyleSheet(QString(R"(
    QLabel { 
      background-color: %1; 
      border: 2px solid rgba(255, 255, 255, 0.15); 
      border-radius: 8px; 
    }
  )").arg(color.name()));
  if (brushPreview_) {
    brushPreview_->setBrushColor(color);
  }
}

void ToolPanel::updateZoomDisplay(double zoom) { zoomLabel->setText(QString("%1%").arg(qRound(zoom))); }
void ToolPanel::updateOpacityDisplay(int opacity) { opacitySlider->setValue(opacity); }
void ToolPanel::updatePositionDisplay(const QPointF &pos) { positionLabel->setText(QString("X:%1 Y:%2").arg(qRound(pos.x())).arg(qRound(pos.y()))); }
void ToolPanel::updateFilledShapesDisplay(bool filled) { actionFilledShapes->setChecked(filled); }

void ToolPanel::onActionPen() { clearActiveToolStyles(); actionPen->setChecked(true); setActiveTool("Pen"); emit penSelected(); }
void ToolPanel::onActionEraser() { clearActiveToolStyles(); actionEraser->setChecked(true); setActiveTool("Eraser"); emit eraserSelected(); }
void ToolPanel::onActionText() { clearActiveToolStyles(); actionText->setChecked(true); setActiveTool("Text"); emit textSelected(); }
void ToolPanel::onActionFill() { clearActiveToolStyles(); actionFill->setChecked(true); setActiveTool("Fill"); emit fillSelected(); }
void ToolPanel::onActionLine() { clearActiveToolStyles(); actionLine->setChecked(true); setActiveTool("Line"); emit shapeSelected("Line"); emit lineSelected(); }
void ToolPanel::onActionArrow() { clearActiveToolStyles(); actionArrow->setChecked(true); setActiveTool("Arrow"); emit arrowSelected(); }
void ToolPanel::onActionRectangle() { clearActiveToolStyles(); actionRectangle->setChecked(true); setActiveTool("Rectangle"); emit shapeSelected("Rectangle"); emit rectangleSelected(); }
void ToolPanel::onActionCircle() { clearActiveToolStyles(); actionCircle->setChecked(true); setActiveTool("Circle"); emit shapeSelected("Circle"); emit circleSelected(); }
void ToolPanel::onActionSelection() { clearActiveToolStyles(); actionSelection->setChecked(true); setActiveTool("Select"); emit shapeSelected("Selection"); emit selectionSelected(); }
void ToolPanel::onActionPan() { clearActiveToolStyles(); actionPan->setChecked(true); setActiveTool("Pan"); emit panSelected(); }

void ToolPanel::onActionColor() {
  QColor color = QColorDialog::getColor(Qt::white, this, "Select Color");
  if (color.isValid()) { updateColorDisplay(color); emit colorSelected(color); }
}

void ToolPanel::onOpacityChanged(int value) { 
  int boundedValue = qBound(0, value, 255);
  emit opacitySelected(boundedValue); 
}
void ToolPanel::onActionIncreaseBrush() { emit increaseBrushSize(); }
void ToolPanel::onActionDecreaseBrush() { emit decreaseBrushSize(); }
void ToolPanel::onActionClear() { emit clearCanvas(); }
void ToolPanel::onActionUndo() { emit undoAction(); }
void ToolPanel::onActionRedo() { emit redoAction(); }
void ToolPanel::onActionSave() { emit saveAction(); }
void ToolPanel::onActionOpen() { emit openAction(); }
void ToolPanel::onActionNew() { emit newCanvasAction(); }
void ToolPanel::onActionZoomIn() { emit zoomInAction(); }
void ToolPanel::onActionZoomOut() { emit zoomOutAction(); }
void ToolPanel::onActionZoomReset() { emit zoomResetAction(); }
void ToolPanel::onActionGrid() { emit toggleGridAction(); }
void ToolPanel::onActionFilledShapes() { emit toggleFilledShapesAction(); }
