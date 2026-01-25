#include "tool_panel.h"
#include "brush_preview.h"
#include <QColorDialog>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMouseEvent>
#include <QToolButton>
#include <QScrollArea>

// Helper to create a tool button from an action
static QToolButton* createToolButton(QAction *action, QWidget *parent) {
  QToolButton *btn = new QToolButton(parent);
  btn->setDefaultAction(action);
  btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  btn->setFixedSize(56, 56);
  btn->setIconSize(QSize(20, 20));
  return btn;
}

// Helper to create a horizontal separator
static QFrame* createSeparator(QWidget *parent) {
  QFrame *line = new QFrame(parent);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setStyleSheet("QFrame { background-color: rgba(255, 255, 255, 0.1); max-height: 1px; margin: 4px 8px; }");
  return line;
}

ToolPanel::ToolPanel(QWidget *parent) : QDockWidget("Tools", parent), brushPreview_(nullptr) {
  setObjectName("ToolPanel");
  setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  
  // Create scroll area for content
  QScrollArea *scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setFrameShape(QFrame::NoFrame);
  
  QWidget *container = new QWidget(scrollArea);
  QVBoxLayout *mainLayout = new QVBoxLayout(container);
  mainLayout->setContentsMargins(8, 8, 8, 8);
  mainLayout->setSpacing(6);
  mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

  // === DRAWING TOOLS ===
  actionPen = new QAction("✎ Pen", this);
  actionPen->setToolTip("Freehand draw (P)");
  actionPen->setCheckable(true);
  actionPen->setChecked(true);
  connect(actionPen, &QAction::triggered, this, &ToolPanel::onActionPen);

  actionEraser = new QAction("⌫ Eraser", this);
  actionEraser->setToolTip("Erase items (E)");
  actionEraser->setCheckable(true);
  connect(actionEraser, &QAction::triggered, this, &ToolPanel::onActionEraser);

  actionText = new QAction("T Text", this);
  actionText->setToolTip("Add text (T)");
  actionText->setCheckable(true);
  connect(actionText, &QAction::triggered, this, &ToolPanel::onActionText);

  actionFill = new QAction("◉ Fill", this);
  actionFill->setToolTip("Fill existing shapes with color (F)");
  actionFill->setCheckable(true);
  connect(actionFill, &QAction::triggered, this, &ToolPanel::onActionFill);

  // Drawing tools grid (2x2)
  QGridLayout *drawGrid = new QGridLayout();
  drawGrid->setSpacing(4);
  drawGrid->addWidget(createToolButton(actionPen, container), 0, 0);
  drawGrid->addWidget(createToolButton(actionEraser, container), 0, 1);
  drawGrid->addWidget(createToolButton(actionText, container), 1, 0);
  drawGrid->addWidget(createToolButton(actionFill, container), 1, 1);
  mainLayout->addLayout(drawGrid);

  mainLayout->addWidget(createSeparator(container));

  // === SHAPE TOOLS ===
  actionLine = new QAction("╱ Line", this);
  actionLine->setToolTip("Draw line (L)");
  actionLine->setCheckable(true);
  connect(actionLine, &QAction::triggered, this, &ToolPanel::onActionLine);

  actionArrow = new QAction("➤ Arrow", this);
  actionArrow->setToolTip("Draw arrow (A)");
  actionArrow->setCheckable(true);
  connect(actionArrow, &QAction::triggered, this, &ToolPanel::onActionArrow);

  actionRectangle = new QAction("▢ Rect", this);
  actionRectangle->setToolTip("Draw rectangle (R)");
  actionRectangle->setCheckable(true);
  connect(actionRectangle, &QAction::triggered, this, &ToolPanel::onActionRectangle);

  actionCircle = new QAction("◯ Circle", this);
  actionCircle->setToolTip("Draw circle (C)");
  actionCircle->setCheckable(true);
  connect(actionCircle, &QAction::triggered, this, &ToolPanel::onActionCircle);

  // Shape tools grid (2x2)
  QGridLayout *shapeGrid = new QGridLayout();
  shapeGrid->setSpacing(4);
  shapeGrid->addWidget(createToolButton(actionLine, container), 0, 0);
  shapeGrid->addWidget(createToolButton(actionArrow, container), 0, 1);
  shapeGrid->addWidget(createToolButton(actionRectangle, container), 1, 0);
  shapeGrid->addWidget(createToolButton(actionCircle, container), 1, 1);
  mainLayout->addLayout(shapeGrid);

  mainLayout->addWidget(createSeparator(container));

  // === NAVIGATION TOOLS ===
  actionSelection = new QAction("⬚ Select", this);
  actionSelection->setToolTip("Select items (V)");
  actionSelection->setCheckable(true);
  connect(actionSelection, &QAction::triggered, this, &ToolPanel::onActionSelection);

  actionPan = new QAction("☰ Pan", this);
  actionPan->setToolTip("Pan canvas (H)");
  actionPan->setCheckable(true);
  connect(actionPan, &QAction::triggered, this, &ToolPanel::onActionPan);

  QHBoxLayout *navLayout = new QHBoxLayout();
  navLayout->setSpacing(4);
  navLayout->addWidget(createToolButton(actionSelection, container));
  navLayout->addWidget(createToolButton(actionPan, container));
  mainLayout->addLayout(navLayout);

  mainLayout->addWidget(createSeparator(container));

  // === BRUSH CONTROLS ===
  actionDecreaseBrush = new QAction("−", this);
  actionDecreaseBrush->setToolTip("Decrease size ([)");
  connect(actionDecreaseBrush, &QAction::triggered, this, &ToolPanel::onActionDecreaseBrush);

  actionIncreaseBrush = new QAction("+", this);
  actionIncreaseBrush->setToolTip("Increase size (])");
  connect(actionIncreaseBrush, &QAction::triggered, this, &ToolPanel::onActionIncreaseBrush);

  QHBoxLayout *brushSizeLayout = new QHBoxLayout();
  brushSizeLayout->setSpacing(4);
  brushSizeLayout->setAlignment(Qt::AlignCenter);
  
  QToolButton *decBtn = new QToolButton(container);
  decBtn->setDefaultAction(actionDecreaseBrush);
  decBtn->setFixedSize(56, 56);
  brushSizeLayout->addWidget(decBtn);

  brushSizeLabel = new QLabel("Size: 3", container);
  brushSizeLabel->setStyleSheet(R"(
    QLabel { 
      padding: 6px 10px; 
      background-color: rgba(255, 255, 255, 0.06); 
      color: #f8f8fc; 
      border-radius: 6px; 
      border: 1px solid rgba(255, 255, 255, 0.08);
      font-weight: 500;
    }
  )");
  brushSizeLabel->setAlignment(Qt::AlignCenter);
  brushSizeLabel->setMinimumWidth(60);
  brushSizeLayout->addWidget(brushSizeLabel);

  QToolButton *incBtn = new QToolButton(container);
  incBtn->setDefaultAction(actionIncreaseBrush);
  incBtn->setFixedSize(56, 56);
  brushSizeLayout->addWidget(incBtn);

  mainLayout->addLayout(brushSizeLayout);

  // Brush preview - centered
  QHBoxLayout *brushPreviewLayout = new QHBoxLayout();
  brushPreviewLayout->setAlignment(Qt::AlignCenter);
  brushPreview_ = new BrushPreview(container);
  brushPreviewLayout->addWidget(brushPreview_);
  mainLayout->addLayout(brushPreviewLayout);

  mainLayout->addWidget(createSeparator(container));

  // === COLOR & OPACITY ===
  QHBoxLayout *colorLayout = new QHBoxLayout();
  colorLayout->setAlignment(Qt::AlignCenter);

  colorPreview = new QLabel(container);
  colorPreview->setFixedSize(44, 44);
  colorPreview->setStyleSheet(R"(
    QLabel { 
      background-color: #ffffff; 
      border: 2px solid rgba(255, 255, 255, 0.15); 
      border-radius: 8px; 
    }
    QLabel:hover {
      border: 2px solid #3b82f6;
    }
  )");
  colorPreview->setToolTip("Click to pick color (K)");
  colorPreview->setCursor(Qt::PointingHandCursor);
  colorPreview->installEventFilter(this);
  colorLayout->addWidget(colorPreview);

  mainLayout->addLayout(colorLayout);

  // Opacity slider - centered
  opacityLabel = new QLabel("Opacity", container);
  opacityLabel->setStyleSheet("QLabel { color: #a0a0a8; font-size: 11px; font-weight: 500; }");
  opacityLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(opacityLabel);

  QHBoxLayout *opacityLayout = new QHBoxLayout();
  opacityLayout->setAlignment(Qt::AlignCenter);
  opacitySlider = new QSlider(Qt::Horizontal, container);
  opacitySlider->setRange(0, 255);
  opacitySlider->setValue(255);
  opacitySlider->setMinimumWidth(80);
  opacitySlider->setMaximumWidth(100);
  opacitySlider->setToolTip("Brush opacity");
  connect(opacitySlider, &QSlider::valueChanged, this, &ToolPanel::onOpacityChanged);
  opacityLayout->addWidget(opacitySlider);
  mainLayout->addLayout(opacityLayout);

  mainLayout->addWidget(createSeparator(container));

  // === ZOOM CONTROLS ===
  actionZoomOut = new QAction("−", this);
  actionZoomOut->setToolTip("Zoom out (−)");
  connect(actionZoomOut, &QAction::triggered, this, &ToolPanel::onActionZoomOut);

  actionZoomIn = new QAction("+", this);
  actionZoomIn->setToolTip("Zoom in (+)");
  connect(actionZoomIn, &QAction::triggered, this, &ToolPanel::onActionZoomIn);

  actionZoomReset = new QAction("⟲ 1:1", this);
  actionZoomReset->setToolTip("Reset zoom (0)");
  connect(actionZoomReset, &QAction::triggered, this, &ToolPanel::onActionZoomReset);

  QHBoxLayout *zoomLayout = new QHBoxLayout();
  zoomLayout->setSpacing(4);
  zoomLayout->setAlignment(Qt::AlignCenter);

  QToolButton *zoomOutBtn = new QToolButton(container);
  zoomOutBtn->setDefaultAction(actionZoomOut);
  zoomOutBtn->setFixedSize(56, 56);
  zoomLayout->addWidget(zoomOutBtn);

  zoomLabel = new QLabel("100%", container);
  zoomLabel->setStyleSheet(R"(
    QLabel { 
      padding: 6px 10px; 
      background-color: rgba(255, 255, 255, 0.06); 
      color: #f8f8fc; 
      border-radius: 6px; 
      border: 1px solid rgba(255, 255, 255, 0.08);
      font-weight: 500;
    }
  )");
  zoomLabel->setAlignment(Qt::AlignCenter);
  zoomLabel->setMinimumWidth(55);
  zoomLayout->addWidget(zoomLabel);

  QToolButton *zoomInBtn = new QToolButton(container);
  zoomInBtn->setDefaultAction(actionZoomIn);
  zoomInBtn->setFixedSize(56, 56);
  zoomLayout->addWidget(zoomInBtn);

  mainLayout->addLayout(zoomLayout);

  QHBoxLayout *zoomResetLayout = new QHBoxLayout();
  zoomResetLayout->setAlignment(Qt::AlignCenter);
  zoomResetLayout->addWidget(createToolButton(actionZoomReset, container));
  mainLayout->addLayout(zoomResetLayout);

  // Grid and Filled toggles
  actionGrid = new QAction("⊞ Grid", this);
  actionGrid->setToolTip("Toggle grid (G)");
  actionGrid->setCheckable(true);
  connect(actionGrid, &QAction::triggered, this, &ToolPanel::onActionGrid);

  actionFilledShapes = new QAction("◼ Filled", this);
  actionFilledShapes->setToolTip("Toggle filled shapes (B)");
  actionFilledShapes->setCheckable(true);
  connect(actionFilledShapes, &QAction::triggered, this, &ToolPanel::onActionFilledShapes);

  QHBoxLayout *toggleLayout = new QHBoxLayout();
  toggleLayout->setSpacing(4);
  toggleLayout->addWidget(createToolButton(actionGrid, container));
  toggleLayout->addWidget(createToolButton(actionFilledShapes, container));
  mainLayout->addLayout(toggleLayout);

  mainLayout->addWidget(createSeparator(container));

  // === EDIT ACTIONS ===
  actionUndo = new QAction("↶ Undo", this);
  actionUndo->setToolTip("Undo (Ctrl+Z)");
  connect(actionUndo, &QAction::triggered, this, &ToolPanel::onActionUndo);

  actionRedo = new QAction("↷ Redo", this);
  actionRedo->setToolTip("Redo (Ctrl+Y)");
  connect(actionRedo, &QAction::triggered, this, &ToolPanel::onActionRedo);

  QHBoxLayout *undoRedoLayout = new QHBoxLayout();
  undoRedoLayout->setSpacing(4);
  undoRedoLayout->addWidget(createToolButton(actionUndo, container));
  undoRedoLayout->addWidget(createToolButton(actionRedo, container));
  mainLayout->addLayout(undoRedoLayout);

  // Copy/Cut/Paste/Dup/Del
  QAction *actionCopy = new QAction("⧉ Copy", this);
  actionCopy->setToolTip("Copy (Ctrl+C)");
  connect(actionCopy, &QAction::triggered, this, &ToolPanel::copyAction);

  QAction *actionCut = new QAction("✂ Cut", this);
  actionCut->setToolTip("Cut (Ctrl+X)");
  connect(actionCut, &QAction::triggered, this, &ToolPanel::cutAction);

  QAction *actionPaste = new QAction("📋 Paste", this);
  actionPaste->setToolTip("Paste (Ctrl+V)");
  connect(actionPaste, &QAction::triggered, this, &ToolPanel::pasteAction);

  QAction *actionDuplicate = new QAction("⊕ Dup", this);
  actionDuplicate->setToolTip("Duplicate (Ctrl+D)");
  connect(actionDuplicate, &QAction::triggered, this, &ToolPanel::duplicateAction);

  QAction *actionDelete = new QAction("✕ Del", this);
  actionDelete->setToolTip("Delete (Del)");
  connect(actionDelete, &QAction::triggered, this, &ToolPanel::deleteAction);

  QGridLayout *editGrid = new QGridLayout();
  editGrid->setSpacing(4);
  editGrid->addWidget(createToolButton(actionCopy, container), 0, 0);
  editGrid->addWidget(createToolButton(actionCut, container), 0, 1);
  editGrid->addWidget(createToolButton(actionPaste, container), 1, 0);
  editGrid->addWidget(createToolButton(actionDuplicate, container), 1, 1);
  mainLayout->addLayout(editGrid);

  QHBoxLayout *deleteLayout = new QHBoxLayout();
  deleteLayout->setAlignment(Qt::AlignCenter);
  deleteLayout->addWidget(createToolButton(actionDelete, container));
  mainLayout->addLayout(deleteLayout);

  mainLayout->addWidget(createSeparator(container));

  // === FILE ACTIONS ===
  actionNew = new QAction("📄 New", this);
  actionNew->setToolTip("New canvas (Ctrl+N)");
  connect(actionNew, &QAction::triggered, this, &ToolPanel::onActionNew);

  actionOpen = new QAction("📂 Open", this);
  actionOpen->setToolTip("Open image (Ctrl+O)");
  connect(actionOpen, &QAction::triggered, this, &ToolPanel::onActionOpen);

  actionSave = new QAction("💾 Save", this);
  actionSave->setToolTip("Save (Ctrl+S)");
  connect(actionSave, &QAction::triggered, this, &ToolPanel::onActionSave);

  actionClear = new QAction("🗑 Clear", this);
  actionClear->setToolTip("Clear canvas");
  connect(actionClear, &QAction::triggered, this, &ToolPanel::onActionClear);

  QGridLayout *fileGrid = new QGridLayout();
  fileGrid->setSpacing(4);
  fileGrid->addWidget(createToolButton(actionNew, container), 0, 0);
  fileGrid->addWidget(createToolButton(actionOpen, container), 0, 1);
  fileGrid->addWidget(createToolButton(actionSave, container), 1, 0);
  fileGrid->addWidget(createToolButton(actionClear, container), 1, 1);
  mainLayout->addLayout(fileGrid);

  mainLayout->addWidget(createSeparator(container));

  // === STATUS DISPLAY ===
  activeToolLabel = new QLabel("✎ Pen", container);
  activeToolLabel->setStyleSheet(R"(
    QLabel { 
      font-weight: 600; 
      padding: 8px 12px; 
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa); 
      color: #ffffff; 
      border-radius: 6px; 
      font-size: 12px;
      border: 1px solid rgba(255, 255, 255, 0.15);
    }
  )");
  activeToolLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(activeToolLabel);

  positionLabel = new QLabel("X: 0  Y: 0", container);
  positionLabel->setStyleSheet(R"(
    QLabel { 
      padding: 6px 10px; 
      background-color: rgba(0, 0, 0, 0.3); 
      color: #a0a0a8; 
      border-radius: 6px; 
      border: 1px solid rgba(255, 255, 255, 0.05);
      font-size: 11px;
      font-weight: 500;
    }
  )");
  positionLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(positionLabel);

  mainLayout->addStretch();
  
  scrollArea->setWidget(container);
  setWidget(scrollArea);
  
  setMinimumWidth(145);
  setMaximumWidth(200);

  // Styling
  setStyleSheet(R"(
    QDockWidget {
      background-color: #1a1a1e;
      color: #f8f8fc;
      font-weight: 500;
    }
    QDockWidget::title {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a2a30, stop:1 #242428);
      padding: 10px 12px;
      font-weight: 600;
      border-bottom: 1px solid rgba(255, 255, 255, 0.06);
    }
    QScrollArea {
      background-color: #1a1a1e;
      border: none;
    }
    QToolButton {
      background-color: rgba(255, 255, 255, 0.06);
      color: #e0e0e6;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 6px;
      padding: 4px;
      min-width: 56px;
      min-height: 56px;
      max-width: 56px;
      max-height: 56px;
      font-weight: 500;
      font-size: 10px;
    }
    QToolButton:hover {
      background-color: rgba(255, 255, 255, 0.1);
      border: 1px solid rgba(59, 130, 246, 0.3);
    }
    QToolButton:pressed {
      background-color: rgba(255, 255, 255, 0.04);
    }
    QToolButton:checked {
      background-color: #3b82f6;
      color: #ffffff;
      border: 1px solid #60a5fa;
    }
    QSlider::groove:horizontal {
      background: #28282e;
      height: 6px;
      border-radius: 3px;
    }
    QSlider::handle:horizontal {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #60a5fa, stop:1 #3b82f6);
      width: 14px;
      height: 14px;
      margin: -4px 0;
      border-radius: 7px;
      border: 1px solid rgba(255, 255, 255, 0.15);
    }
    QSlider::sub-page:horizontal {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa);
      border-radius: 3px;
    }
  )");
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
  static const QHash<QString, QString> toolIcons = {
    {"Pen", "✎"}, {"Eraser", "⌫"}, {"Text", "T"}, {"Fill", "◉"},
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
      border-radius: 6px; 
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

bool ToolPanel::eventFilter(QObject *obj, QEvent *event) {
  if (obj == colorPreview && event->type() == QEvent::MouseButtonRelease) {
    onActionColor();
    return true;
  }
  return QDockWidget::eventFilter(obj, event);
}
