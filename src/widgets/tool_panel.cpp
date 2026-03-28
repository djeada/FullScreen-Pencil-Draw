#include "tool_panel.h"
#include "../core/theme_manager.h"
#include "animated_button.h"
#include "brush_preview.h"
#include <QColorDialog>
#include <QConicalGradient>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QRadialGradient>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

// Helper to create a tool button from an action
static AnimatedToolButton *createToolButton(QAction *action, QWidget *parent) {
  auto *btn = new AnimatedToolButton(parent);
  btn->setDefaultAction(action);
  btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  btn->setFixedSize(56, 56);
  btn->setIconSize(QSize(20, 20));
  btn->setProperty("toolTile", true);
  btn->setVariant(AnimatedButtonBase::Variant::PanelTile);
  return btn;
}

// Helper to create a horizontal separator
static QFrame *createSeparator(QWidget *parent) {
  QFrame *line = new QFrame(parent);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setObjectName("panelSeparator");
  line->setFixedHeight(1);
  return line;
}

ToolPanel::ToolPanel(QWidget *parent)
    : QDockWidget("Tools", parent), brushTipTextLabel_(nullptr),
      fillStyleTextLabel_(nullptr), brushPreview_(nullptr),
      fillStyleCombo_(nullptr), brushTipCombo_(nullptr),
      currentColor_(Qt::white) {
  setObjectName("ToolPanel");
  setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetFloatable);
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
  mainLayout->setAlignment(Qt::AlignTop);

  // === DRAWING TOOLS ===
  actionPen = new QAction("✎ Pen", this);
  actionPen->setToolTip("Freehand draw (P)");
  actionPen->setCheckable(true);
  actionPen->setChecked(true);
  connect(actionPen, &QAction::triggered, this, &ToolPanel::onActionPen);

  actionHighlighter = new QAction("▉ Highlight", this);
  actionHighlighter->setToolTip("Highlight text and strokes (I)");
  actionHighlighter->setCheckable(true);
  connect(actionHighlighter, &QAction::triggered, this,
          &ToolPanel::onActionHighlighter);

  actionEraser = new QAction("⌫ Eraser", this);
  actionEraser->setToolTip("Erase items (E)");
  actionEraser->setCheckable(true);
  connect(actionEraser, &QAction::triggered, this, &ToolPanel::onActionEraser);

  actionText = new QAction("T Text", this);
  actionText->setToolTip("Add text (T)");
  actionText->setCheckable(true);
  connect(actionText, &QAction::triggered, this, &ToolPanel::onActionText);

  actionMermaid = new QAction("⬡ Mermaid", this);
  actionMermaid->setToolTip("Add Mermaid diagram (M)");
  actionMermaid->setCheckable(true);
  connect(actionMermaid, &QAction::triggered, this,
          &ToolPanel::onActionMermaid);

  actionFill = new QAction("◉ Fill", this);
  actionFill->setToolTip("Fill existing shapes with color (F)");
  actionFill->setCheckable(true);
  connect(actionFill, &QAction::triggered, this, &ToolPanel::onActionFill);

  actionColorSelect = new QAction("◎ Select", this);
  actionColorSelect->setToolTip("Select pixels by color (Q)");
  actionColorSelect->setCheckable(true);
  connect(actionColorSelect, &QAction::triggered, this,
          &ToolPanel::onActionColorSelect);

  // Drawing tools grid (4x2) - wrapped for centering
  QWidget *drawGridWidget = new QWidget(container);
  QGridLayout *drawGrid = new QGridLayout(drawGridWidget);
  drawGrid->setSpacing(4);
  drawGrid->setContentsMargins(0, 0, 0, 0);
  drawGrid->addWidget(createToolButton(actionPen, drawGridWidget), 0, 0);
  drawGrid->addWidget(createToolButton(actionHighlighter, drawGridWidget), 0,
                      1);
  drawGrid->addWidget(createToolButton(actionEraser, drawGridWidget), 1, 0);
  drawGrid->addWidget(createToolButton(actionText, drawGridWidget), 1, 1);
  drawGrid->addWidget(createToolButton(actionFill, drawGridWidget), 2, 0);
  drawGrid->addWidget(createToolButton(actionMermaid, drawGridWidget), 2, 1);
  drawGrid->addWidget(createToolButton(actionColorSelect, drawGridWidget), 3,
                      0);
  drawGridWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(drawGridWidget, 0, Qt::AlignHCenter);

  mainLayout->addWidget(createSeparator(container));

  // === SHAPE TOOLS ===
  actionLine = new QAction("╱ Line", this);
  actionLine->setToolTip("Draw line (L)");
  actionLine->setCheckable(true);
  connect(actionLine, &QAction::triggered, this, &ToolPanel::onActionLine);

  actionArrow = new QAction("➤ Arrow", this);
  actionArrow->setToolTip("Draw straight arrow (A)");
  actionArrow->setCheckable(true);
  connect(actionArrow, &QAction::triggered, this, &ToolPanel::onActionArrow);

  actionCurvedArrow = new QAction("↪ Curve", this);
  actionCurvedArrow->setToolTip(
      "Draw curved arrow (Shift+A). While dragging: press Shift once to flip "
      "and lock bend side. Alt more bend, Ctrl less bend.");
  actionCurvedArrow->setCheckable(true);
  connect(actionCurvedArrow, &QAction::triggered, this,
          &ToolPanel::onActionCurvedArrow);

  actionWire = new QAction("⏚ Wire", this);
  actionWire->setToolTip("Draw wire between electronics pins (W)");
  actionWire->setCheckable(true);
  connect(actionWire, &QAction::triggered, this, &ToolPanel::onActionWire);

  actionRectangle = new QAction("▢ Rect", this);
  actionRectangle->setToolTip("Draw rectangle (R)");
  actionRectangle->setCheckable(true);
  connect(actionRectangle, &QAction::triggered, this,
          &ToolPanel::onActionRectangle);

  actionCircle = new QAction("◯ Circle", this);
  actionCircle->setToolTip("Draw circle (C)");
  actionCircle->setCheckable(true);
  connect(actionCircle, &QAction::triggered, this, &ToolPanel::onActionCircle);

  actionBezier = new QAction("⌇ Bezier", this);
  actionBezier->setToolTip("Draw Bezier path (Shift+B)");
  actionBezier->setCheckable(true);
  connect(actionBezier, &QAction::triggered, this, &ToolPanel::onActionBezier);

  actionTextOnPath = new QAction("⌇T TxtPath", this);
  actionTextOnPath->setToolTip("Place text along a path (Shift+T)");
  actionTextOnPath->setCheckable(true);
  connect(actionTextOnPath, &QAction::triggered, this,
          &ToolPanel::onActionTextOnPath);

  // Shape tools grid (3x2) - wrapped for centering
  QWidget *shapeGridWidget = new QWidget(container);
  QGridLayout *shapeGrid = new QGridLayout(shapeGridWidget);
  shapeGrid->setSpacing(4);
  shapeGrid->setContentsMargins(0, 0, 0, 0);
  shapeGrid->addWidget(createToolButton(actionLine, shapeGridWidget), 0, 0);
  shapeGrid->addWidget(createToolButton(actionArrow, shapeGridWidget), 0, 1);
  shapeGrid->addWidget(createToolButton(actionCurvedArrow, shapeGridWidget), 1,
                       0);
  shapeGrid->addWidget(createToolButton(actionRectangle, shapeGridWidget), 1,
                       1);
  shapeGrid->addWidget(createToolButton(actionCircle, shapeGridWidget), 2, 0);
  shapeGrid->addWidget(createToolButton(actionBezier, shapeGridWidget), 2, 1);
  shapeGrid->addWidget(createToolButton(actionTextOnPath, shapeGridWidget), 3,
                       0);
  shapeGrid->addWidget(createToolButton(actionWire, shapeGridWidget), 3, 1);
  shapeGridWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(shapeGridWidget, 0, Qt::AlignHCenter);

  mainLayout->addWidget(createSeparator(container));

  // === NAVIGATION TOOLS ===
  actionSelection = new QAction("⬚ Select", this);
  actionSelection->setToolTip("Select items (V)");
  actionSelection->setCheckable(true);
  connect(actionSelection, &QAction::triggered, this,
          &ToolPanel::onActionSelection);

  actionLassoSelection = new QAction("⛶ Lasso", this);
  actionLassoSelection->setToolTip("Lasso selection (Shift+S)");
  actionLassoSelection->setCheckable(true);
  connect(actionLassoSelection, &QAction::triggered, this,
          &ToolPanel::onActionLassoSelection);

  actionPan = new QAction("☰ Pan", this);
  actionPan->setToolTip("Pan canvas (H)");
  actionPan->setCheckable(true);
  connect(actionPan, &QAction::triggered, this, &ToolPanel::onActionPan);

  QWidget *navWidget = new QWidget(container);
  QHBoxLayout *navLayout = new QHBoxLayout(navWidget);
  navLayout->setSpacing(4);
  navLayout->setContentsMargins(0, 0, 0, 0);
  navLayout->addWidget(createToolButton(actionSelection, navWidget));
  navLayout->addWidget(createToolButton(actionLassoSelection, navWidget));
  navLayout->addWidget(createToolButton(actionPan, navWidget));
  navWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(navWidget, 0, Qt::AlignHCenter);

  mainLayout->addWidget(createSeparator(container));

  // === BRUSH CONTROLS ===
  actionDecreaseBrush = new QAction("−", this);
  actionDecreaseBrush->setToolTip("Decrease size ([)");
  connect(actionDecreaseBrush, &QAction::triggered, this,
          &ToolPanel::onActionDecreaseBrush);

  actionIncreaseBrush = new QAction("+", this);
  actionIncreaseBrush->setToolTip("Increase size (])");
  connect(actionIncreaseBrush, &QAction::triggered, this,
          &ToolPanel::onActionIncreaseBrush);

  QWidget *brushSizeWidget = new QWidget(container);
  QHBoxLayout *brushSizeLayout = new QHBoxLayout(brushSizeWidget);
  brushSizeLayout->setSpacing(4);
  brushSizeLayout->setContentsMargins(0, 0, 0, 0);

  auto *decBtn = new AnimatedToolButton(brushSizeWidget);
  decBtn->setDefaultAction(actionDecreaseBrush);
  decBtn->setFixedSize(40, 40);
  decBtn->setVariant(AnimatedButtonBase::Variant::Compact);
  brushSizeLayout->addWidget(decBtn);

  brushSizeLabel = new QLabel("3", brushSizeWidget);
  brushSizeLabel->setStyleSheet(R"(
    QLabel { 
      padding: 4px 6px; 
      background-color: rgba(255, 255, 255, 0.06); 
      color: #f8f8fc; 
      border-radius: 6px; 
      border: 1px solid rgba(255, 255, 255, 0.08);
      font-weight: 500;
    }
  )");
  brushSizeLabel->setAlignment(Qt::AlignCenter);
  brushSizeLabel->setFixedWidth(46);
  brushSizeLayout->addWidget(brushSizeLabel);

  auto *incBtn = new AnimatedToolButton(brushSizeWidget);
  incBtn->setDefaultAction(actionIncreaseBrush);
  incBtn->setFixedSize(40, 40);
  incBtn->setVariant(AnimatedButtonBase::Variant::Compact);
  brushSizeLayout->addWidget(incBtn);

  brushSizeWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(brushSizeWidget, 0, Qt::AlignHCenter);

  // Brush preview - centered
  QHBoxLayout *brushPreviewLayout = new QHBoxLayout();
  brushPreviewLayout->setContentsMargins(0, 0, 0, 0);
  brushPreviewLayout->setAlignment(Qt::AlignCenter);
  brushPreview_ = new BrushPreview(container);
  brushPreviewLayout->addWidget(brushPreview_);
  mainLayout->addLayout(brushPreviewLayout);

  // Brush tip selector
  brushTipTextLabel_ = new QLabel("Brush Tip", container);
  brushTipTextLabel_->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(brushTipTextLabel_);

  brushTipCombo_ = new QComboBox(container);
  brushTipCombo_->addItem("● Round", static_cast<int>(BrushTipShape::Round));
  brushTipCombo_->addItem("⌿ Chisel", static_cast<int>(BrushTipShape::Chisel));
  brushTipCombo_->addItem("✦ Stamp", static_cast<int>(BrushTipShape::Stamp));
  brushTipCombo_->addItem("▒ Textured",
                          static_cast<int>(BrushTipShape::Textured));
  brushTipCombo_->setToolTip(
      "Select brush tip shape (Round, Chisel for calligraphy, Stamp, "
      "Textured)");
  brushTipCombo_->setMaximumWidth(140);
  brushTipCombo_->setStyleSheet(R"(
    QComboBox {
      background-color: rgba(255, 255, 255, 0.06);
      color: #e0e0e6;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 6px;
      padding: 4px 8px;
      font-size: 11px;
    }
    QComboBox:hover {
      border: 1px solid rgba(59, 130, 246, 0.3);
    }
    QComboBox::drop-down {
      border: none;
    }
    QComboBox QAbstractItemView {
      background-color: #2a2a30;
      color: #e0e0e6;
      selection-background-color: #3b82f6;
      border: 1px solid rgba(255, 255, 255, 0.1);
    }
  )");
  connect(brushTipCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ToolPanel::onBrushTipChanged);
  mainLayout->addWidget(brushTipCombo_, 0, Qt::AlignHCenter);

  mainLayout->addWidget(createSeparator(container));

  // === COLOR & OPACITY ===
  QHBoxLayout *colorLayout = new QHBoxLayout();
  colorLayout->setContentsMargins(0, 0, 0, 0);
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
  opacityLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(opacityLabel);

  QHBoxLayout *opacityLayout = new QHBoxLayout();
  opacityLayout->setContentsMargins(0, 0, 0, 0);
  opacityLayout->setAlignment(Qt::AlignCenter);
  opacitySlider = new QSlider(Qt::Horizontal, container);
  opacitySlider->setRange(0, 255);
  opacitySlider->setValue(255);
  opacitySlider->setMinimumWidth(80);
  opacitySlider->setMaximumWidth(100);
  opacitySlider->setToolTip("Brush opacity");
  connect(opacitySlider, &QSlider::valueChanged, this,
          &ToolPanel::onOpacityChanged);
  opacityLayout->addWidget(opacitySlider);
  mainLayout->addLayout(opacityLayout);

  // === PRESSURE SENSITIVITY ===
  pressureSensitivityCheckBox_ = new QCheckBox("Pressure", container);
  pressureSensitivityCheckBox_->setToolTip(
      "Enable pressure sensitivity for stylus input");
  connect(pressureSensitivityCheckBox_, &QCheckBox::toggled, this,
          &ToolPanel::pressureSensitivityToggled);
  mainLayout->addWidget(pressureSensitivityCheckBox_, 0, Qt::AlignCenter);

  mainLayout->addWidget(createSeparator(container));

  // === ZOOM CONTROLS ===
  actionZoomOut = new QAction("−", this);
  actionZoomOut->setToolTip("Zoom out (−)");
  connect(actionZoomOut, &QAction::triggered, this,
          &ToolPanel::onActionZoomOut);

  actionZoomIn = new QAction("+", this);
  actionZoomIn->setToolTip("Zoom in (+)");
  connect(actionZoomIn, &QAction::triggered, this, &ToolPanel::onActionZoomIn);

  actionZoomReset = new QAction("⟲ 1:1", this);
  actionZoomReset->setToolTip("Reset zoom (0)");
  connect(actionZoomReset, &QAction::triggered, this,
          &ToolPanel::onActionZoomReset);

  QWidget *zoomWidget = new QWidget(container);
  QHBoxLayout *zoomLayout = new QHBoxLayout(zoomWidget);
  zoomLayout->setSpacing(4);
  zoomLayout->setContentsMargins(0, 0, 0, 0);

  auto *zoomOutBtn = new AnimatedToolButton(zoomWidget);
  zoomOutBtn->setDefaultAction(actionZoomOut);
  zoomOutBtn->setFixedSize(40, 40);
  zoomOutBtn->setVariant(AnimatedButtonBase::Variant::Compact);
  zoomLayout->addWidget(zoomOutBtn);

  zoomLabel = new QLabel("100%", zoomWidget);
  zoomLabel->setStyleSheet(R"(
    QLabel { 
      padding: 4px 6px; 
      background-color: rgba(255, 255, 255, 0.06); 
      color: #f8f8fc; 
      border-radius: 6px; 
      border: 1px solid rgba(255, 255, 255, 0.08);
      font-weight: 500;
    }
  )");
  zoomLabel->setAlignment(Qt::AlignCenter);
  zoomLabel->setFixedWidth(52);
  zoomLayout->addWidget(zoomLabel);

  auto *zoomInBtn = new AnimatedToolButton(zoomWidget);
  zoomInBtn->setDefaultAction(actionZoomIn);
  zoomInBtn->setFixedSize(40, 40);
  zoomInBtn->setVariant(AnimatedButtonBase::Variant::Compact);
  zoomLayout->addWidget(zoomInBtn);

  zoomWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(zoomWidget, 0, Qt::AlignHCenter);

  mainLayout->addWidget(createToolButton(actionZoomReset, container), 0,
                        Qt::AlignHCenter);

  // Grid and Filled toggles
  actionGrid = new QAction("⊞ Grid", this);
  actionGrid->setToolTip("Toggle grid (G)");
  actionGrid->setCheckable(true);
  connect(actionGrid, &QAction::triggered, this, &ToolPanel::onActionGrid);

  actionFilledShapes = new QAction("◼ Filled", this);
  actionFilledShapes->setToolTip("Toggle filled shapes (B)");
  actionFilledShapes->setCheckable(true);
  connect(actionFilledShapes, &QAction::triggered, this,
          &ToolPanel::onActionFilledShapes);

  QWidget *toggleWidget = new QWidget(container);
  QHBoxLayout *toggleLayout = new QHBoxLayout(toggleWidget);
  toggleLayout->setSpacing(4);
  toggleLayout->setContentsMargins(0, 0, 0, 0);
  toggleLayout->addWidget(createToolButton(actionGrid, toggleWidget));
  toggleLayout->addWidget(createToolButton(actionFilledShapes, toggleWidget));
  toggleWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(toggleWidget, 0, Qt::AlignHCenter);

  // Fill style selector
  fillStyleTextLabel_ = new QLabel("Fill Style", container);
  fillStyleTextLabel_->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(fillStyleTextLabel_);

  fillStyleCombo_ = new QComboBox(container);
  fillStyleCombo_->addItem("Solid", static_cast<int>(Qt::SolidPattern));
  fillStyleCombo_->addItem("Linear Gradient", -1);
  fillStyleCombo_->addItem("Radial Gradient", -2);
  fillStyleCombo_->addItem("Conical Gradient", -3);
  fillStyleCombo_->addItem("Dense", static_cast<int>(Qt::Dense4Pattern));
  fillStyleCombo_->addItem("Cross", static_cast<int>(Qt::CrossPattern));
  fillStyleCombo_->addItem("Diagonal Cross",
                           static_cast<int>(Qt::DiagCrossPattern));
  fillStyleCombo_->addItem("Horizontal Lines",
                           static_cast<int>(Qt::HorPattern));
  fillStyleCombo_->addItem("Vertical Lines", static_cast<int>(Qt::VerPattern));
  fillStyleCombo_->addItem("Forward Diagonal",
                           static_cast<int>(Qt::FDiagPattern));
  fillStyleCombo_->addItem("Backward Diagonal",
                           static_cast<int>(Qt::BDiagPattern));
  fillStyleCombo_->setToolTip("Select fill style for shapes and the fill tool");
  fillStyleCombo_->setMaximumWidth(140);
  fillStyleCombo_->setStyleSheet(R"(
    QComboBox {
      background-color: rgba(255, 255, 255, 0.06);
      color: #e0e0e6;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 6px;
      padding: 4px 8px;
      font-size: 11px;
    }
    QComboBox:hover {
      border: 1px solid rgba(59, 130, 246, 0.3);
    }
    QComboBox::drop-down {
      border: none;
    }
    QComboBox QAbstractItemView {
      background-color: #2a2a30;
      color: #e0e0e6;
      selection-background-color: #3b82f6;
      border: 1px solid rgba(255, 255, 255, 0.1);
    }
  )");
  connect(fillStyleCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ToolPanel::onFillStyleChanged);
  mainLayout->addWidget(fillStyleCombo_, 0, Qt::AlignHCenter);

  mainLayout->addWidget(createSeparator(container));

  // === EDIT ACTIONS ===
  actionUndo = new QAction("↶ Undo", this);
  actionUndo->setToolTip("Undo (Ctrl+Z)");
  connect(actionUndo, &QAction::triggered, this, &ToolPanel::onActionUndo);

  actionRedo = new QAction("↷ Redo", this);
  actionRedo->setToolTip("Redo (Ctrl+Y)");
  connect(actionRedo, &QAction::triggered, this, &ToolPanel::onActionRedo);

  QWidget *undoRedoWidget = new QWidget(container);
  QHBoxLayout *undoRedoLayout = new QHBoxLayout(undoRedoWidget);
  undoRedoLayout->setSpacing(4);
  undoRedoLayout->setContentsMargins(0, 0, 0, 0);
  undoRedoLayout->addWidget(createToolButton(actionUndo, undoRedoWidget));
  undoRedoLayout->addWidget(createToolButton(actionRedo, undoRedoWidget));
  undoRedoWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(undoRedoWidget, 0, Qt::AlignHCenter);

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
  connect(actionDuplicate, &QAction::triggered, this,
          &ToolPanel::duplicateAction);

  QAction *actionDelete = new QAction("✕ Del", this);
  actionDelete->setToolTip("Delete (Del)");
  connect(actionDelete, &QAction::triggered, this, &ToolPanel::deleteAction);

  QWidget *editGridWidget = new QWidget(container);
  QGridLayout *editGrid = new QGridLayout(editGridWidget);
  editGrid->setSpacing(4);
  editGrid->setContentsMargins(0, 0, 0, 0);
  editGrid->addWidget(createToolButton(actionCopy, editGridWidget), 0, 0);
  editGrid->addWidget(createToolButton(actionCut, editGridWidget), 0, 1);
  editGrid->addWidget(createToolButton(actionPaste, editGridWidget), 1, 0);
  editGrid->addWidget(createToolButton(actionDuplicate, editGridWidget), 1, 1);
  editGridWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(editGridWidget, 0, Qt::AlignHCenter);

  mainLayout->addWidget(createToolButton(actionDelete, container), 0,
                        Qt::AlignHCenter);

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

  QWidget *fileGridWidget = new QWidget(container);
  QGridLayout *fileGrid = new QGridLayout(fileGridWidget);
  fileGrid->setSpacing(4);
  fileGrid->setContentsMargins(0, 0, 0, 0);
  fileGrid->addWidget(createToolButton(actionNew, fileGridWidget), 0, 0);
  fileGrid->addWidget(createToolButton(actionOpen, fileGridWidget), 0, 1);
  fileGrid->addWidget(createToolButton(actionSave, fileGridWidget), 1, 0);
  fileGrid->addWidget(createToolButton(actionClear, fileGridWidget), 1, 1);
  fileGridWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  mainLayout->addWidget(fileGridWidget, 0, Qt::AlignHCenter);

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

  // Leave enough room for content plus the vertical scrollbar without
  // clipping button rows when docked.
  setMinimumWidth(236);
  resize(236, height());
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          [this]() { applyTheme(); });
  applyTheme();
}

void ToolPanel::applyTheme() {
  const bool darkTheme = ThemeManager::instance().isDarkTheme();

  if (darkTheme) {
    setStyleSheet(R"(
      QDockWidget {
        background-color: #10161d;
        color: #f4efe8;
        font-weight: 500;
      }
      QDockWidget::title {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                    stop:0 #17212b, stop:0.55 #121922, stop:1 #10161d);
        padding: 12px 14px;
        font-weight: 700;
        letter-spacing: 0.6px;
        border-bottom: 1px solid rgba(255, 244, 230, 0.08);
      }
      QScrollArea {
        background-color: #10161d;
        border: none;
      }
      QWidget {
        background-color: #10161d;
      }
      QFrame#panelSeparator {
        background-color: rgba(249, 115, 22, 0.22);
        margin: 8px 10px;
      }
      QToolButton {
        background-color: rgba(255, 248, 240, 0.04);
        color: #f4efe8;
        border: 1px solid rgba(255, 244, 230, 0.08);
        border-radius: 12px;
        padding: 5px;
        font-weight: 600;
      }
      QToolButton[toolTile="true"] {
        min-width: 56px;
        min-height: 56px;
        max-width: 56px;
        max-height: 56px;
        font-size: 10px;
        background-color: #17212b;
        border: 1px solid rgba(255, 244, 230, 0.08);
      }
      QToolButton:hover {
        background-color: #1d2934;
        border: 1px solid rgba(249, 115, 22, 0.35);
      }
      QToolButton:pressed {
        background-color: rgba(249, 115, 22, 0.14);
      }
      QToolButton:checked {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                    stop:0 #fb923c, stop:1 #ea580c);
        color: #fffaf4;
        border: 1px solid rgba(255, 244, 230, 0.3);
      }
      QToolButton:checked:hover {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                    stop:0 #fdba74, stop:1 #f97316);
      }
      QSlider::groove:horizontal {
        background: #1c2630;
        height: 6px;
        border-radius: 3px;
      }
      QSlider::handle:horizontal {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                    stop:0 #fdba74, stop:1 #f97316);
        width: 14px;
        height: 14px;
        margin: -4px 0;
        border-radius: 7px;
        border: 1px solid rgba(255, 244, 230, 0.18);
      }
      QSlider::sub-page:horizontal {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                    stop:0 #f97316, stop:1 #fb923c);
        border-radius: 3px;
      }
    )");
  } else {
    setStyleSheet(R"(
      QDockWidget {
        background-color: #f5efe6;
        color: #31261d;
        font-weight: 500;
      }
      QDockWidget::title {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                    stop:0 #fff9f1, stop:0.55 #f7efe5, stop:1 #efe4d5);
        color: #31261d;
        padding: 12px 14px;
        font-weight: 700;
        letter-spacing: 0.6px;
        border-bottom: 1px solid #ddcfbc;
      }
      QScrollArea {
        background-color: #f5efe6;
        border: none;
      }
      QWidget {
        background-color: #f5efe6;
      }
      QFrame#panelSeparator {
        background-color: rgba(234, 88, 12, 0.18);
        margin: 8px 10px;
      }
      QToolButton {
        background-color: rgba(255, 250, 244, 0.9);
        color: #31261d;
        border: 1px solid #ddcfbc;
        border-radius: 12px;
        padding: 5px;
        font-weight: 600;
      }
      QToolButton[toolTile="true"] {
        min-width: 56px;
        min-height: 56px;
        max-width: 56px;
        max-height: 56px;
        font-size: 10px;
        background-color: #fff9f1;
      }
      QToolButton:hover {
        background-color: #fff4e7;
        border: 1px solid rgba(234, 88, 12, 0.28);
      }
      QToolButton:pressed {
        background-color: #f6dfca;
      }
      QToolButton:checked {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                    stop:0 #fb923c, stop:1 #ea580c);
        color: #fffaf4;
        border: 1px solid rgba(117, 59, 19, 0.15);
      }
      QToolButton:checked:hover {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                    stop:0 #fdba74, stop:1 #f97316);
      }
      QSlider::groove:horizontal {
        background: #e4d6c7;
        height: 6px;
        border-radius: 3px;
      }
      QSlider::handle:horizontal {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                    stop:0 #fb923c, stop:1 #ea580c);
        width: 14px;
        height: 14px;
        margin: -4px 0;
        border-radius: 7px;
        border: 1px solid rgba(117, 59, 19, 0.15);
      }
      QSlider::sub-page:horizontal {
        background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                    stop:0 #f97316, stop:1 #fb923c);
        border-radius: 3px;
      }
    )");
  }

  const QString subtleLabelStyle = darkTheme
                                       ? "QLabel { color: #d0c4b7; font-size: 11px; font-weight: 500; }"
                                       : "QLabel { color: #7a6858; font-size: 11px; font-weight: 600; letter-spacing: 0.3px; }";
  const QString valueLabelStyle =
      darkTheme
          ? R"(QLabel {
               padding: 5px 8px;
               background-color: #17212b;
               color: #fff7ed;
               border-radius: 8px;
               border: 1px solid rgba(255, 244, 230, 0.08);
               font-weight: 600;
             })"
          : R"(QLabel {
               padding: 5px 8px;
               background-color: #fff9f1;
               color: #31261d;
               border-radius: 8px;
               border: 1px solid #ddcfbc;
               font-weight: 600;
             })";
  const QString comboStyle =
      darkTheme
          ? R"(QComboBox {
               background-color: #17212b;
               color: #f4efe8;
               border: 1px solid rgba(255, 244, 230, 0.08);
               border-radius: 8px;
               padding: 5px 8px;
               font-size: 11px;
               font-weight: 600;
             }
             QComboBox:hover {
               border: 1px solid rgba(249, 115, 22, 0.35);
             }
             QComboBox::drop-down {
               border: none;
             }
             QComboBox QAbstractItemView {
               background-color: #17212b;
               color: #f4efe8;
               selection-background-color: #f97316;
               selection-color: #fffaf4;
               border: 1px solid rgba(255, 244, 230, 0.1);
             })"
          : R"(QComboBox {
               background-color: #fff9f1;
               color: #31261d;
               border: 1px solid #ddcfbc;
               border-radius: 8px;
               padding: 5px 8px;
               font-size: 11px;
               font-weight: 600;
             }
             QComboBox:hover {
               border: 1px solid rgba(234, 88, 12, 0.28);
             }
             QComboBox::drop-down {
               border: none;
             }
             QComboBox QAbstractItemView {
               background-color: #fffaf4;
               color: #31261d;
               selection-background-color: #f97316;
               selection-color: #fffaf4;
               border: 1px solid #ddcfbc;
             })";
  const QString positionStyle =
      darkTheme
          ? R"(QLabel {
               padding: 6px 10px;
               background-color: #131b23;
               color: #d0c4b7;
               border-radius: 8px;
               border: 1px solid rgba(255, 244, 230, 0.06);
               font-size: 11px;
               font-weight: 600;
             })"
          : R"(QLabel {
               padding: 6px 10px;
               background-color: #fff9f1;
               color: #7a6858;
               border-radius: 8px;
               border: 1px solid #e3d5c6;
               font-size: 11px;
               font-weight: 600;
             })";

  if (brushSizeLabel) {
    brushSizeLabel->setStyleSheet(valueLabelStyle);
  }
  if (activeToolLabel) {
    activeToolLabel->setStyleSheet(valueLabelStyle);
  }
  if (zoomLabel) {
    zoomLabel->setStyleSheet(valueLabelStyle);
  }
  if (brushTipTextLabel_) {
    brushTipTextLabel_->setStyleSheet(subtleLabelStyle);
  }
  if (fillStyleTextLabel_) {
    fillStyleTextLabel_->setStyleSheet(subtleLabelStyle);
  }
  if (opacityLabel) {
    opacityLabel->setStyleSheet(subtleLabelStyle);
  }
  if (pressureSensitivityCheckBox_) {
    pressureSensitivityCheckBox_->setStyleSheet(
        darkTheme ? "QCheckBox { color: #d0c4b7; font-size: 11px; font-weight: 600; }"
                  : "QCheckBox { color: #7a6858; font-size: 11px; font-weight: 600; }");
  }
  if (brushTipCombo_) {
    brushTipCombo_->setStyleSheet(comboStyle);
  }
  if (fillStyleCombo_) {
    fillStyleCombo_->setStyleSheet(comboStyle);
  }
  if (positionLabel) {
    positionLabel->setStyleSheet(positionStyle);
  }

  updateColorDisplay(currentColor_);
}

void ToolPanel::clearActiveToolStyles() {
  actionPen->setChecked(false);
  actionHighlighter->setChecked(false);
  actionEraser->setChecked(false);
  actionText->setChecked(false);
  actionMermaid->setChecked(false);
  actionFill->setChecked(false);
  actionColorSelect->setChecked(false);
  actionLine->setChecked(false);
  actionArrow->setChecked(false);
  actionCurvedArrow->setChecked(false);
  actionWire->setChecked(false);
  actionRectangle->setChecked(false);
  actionCircle->setChecked(false);
  actionSelection->setChecked(false);
  actionLassoSelection->setChecked(false);
  actionBezier->setChecked(false);
  actionTextOnPath->setChecked(false);
  actionPan->setChecked(false);
}

void ToolPanel::setActiveTool(const QString &toolName) {
  static const QHash<QString, QString> toolIcons = {
      {"Pen", "✎"},         {"Highlighter", "▉"}, {"Eraser", "⌫"},
      {"Text", "T"},        {"Mermaid", "⬡"},     {"Fill", "◉"},
      {"ColorSelect", "◎"}, {"Line", "╱"},        {"Arrow", "➤"},
      {"CurvedArrow", "↪"}, {"Wire", "⏚"},        {"Rectangle", "▢"},
      {"Circle", "◯"},      {"Select", "⬚"},      {"LassoSelect", "⛶"},
      {"Pan", "☰"},         {"Bezier", "⌇"},      {"TextOnPath", "⌇T"}};
  QString icon = toolIcons.value(toolName, "•");
  activeToolLabel->setText(icon + " " + toolName);
}

void ToolPanel::updateBrushSizeDisplay(int size) {
  brushSizeLabel->setText(QString::number(size));
  if (brushPreview_) {
    brushPreview_->setBrushSize(size);
  }
}

void ToolPanel::updateColorDisplay(const QColor &color) {
  currentColor_ = color;
  const bool darkTheme = ThemeManager::instance().isDarkTheme();
  colorPreview->setStyleSheet(QString(R"(
    QLabel { 
      background-color: %1; 
      border: 2px solid %2; 
      border-radius: 10px; 
    }
  )")
                                  .arg(color.name())
                                  .arg(darkTheme ? "rgba(255, 244, 230, 0.16)"
                                                 : "#ddcfbc"));
  if (brushPreview_) {
    brushPreview_->setBrushColor(color);
  }
  if (fillStyleCombo_) {
    onFillStyleChanged(fillStyleCombo_->currentIndex());
  }
}

void ToolPanel::updateZoomDisplay(double zoom) {
  zoomLabel->setText(QString("%1%").arg(qRound(zoom)));
}
void ToolPanel::updateOpacityDisplay(int opacity) {
  opacitySlider->setValue(opacity);
}
void ToolPanel::updatePositionDisplay(const QPointF &pos) {
  positionLabel->setText(
      QString("X:%1 Y:%2").arg(qRound(pos.x())).arg(qRound(pos.y())));
}
void ToolPanel::updateFilledShapesDisplay(bool filled) {
  actionFilledShapes->setChecked(filled);
}

void ToolPanel::onActionPen() {
  clearActiveToolStyles();
  actionPen->setChecked(true);
  setActiveTool("Pen");
  emit penSelected();
}
void ToolPanel::onActionHighlighter() {
  clearActiveToolStyles();
  actionHighlighter->setChecked(true);
  setActiveTool("Highlighter");
  emit highlighterSelected();
}
void ToolPanel::onActionEraser() {
  clearActiveToolStyles();
  actionEraser->setChecked(true);
  setActiveTool("Eraser");
  emit eraserSelected();
}
void ToolPanel::onActionText() {
  clearActiveToolStyles();
  actionText->setChecked(true);
  setActiveTool("Text");
  emit textSelected();
}
void ToolPanel::onActionMermaid() {
  clearActiveToolStyles();
  actionMermaid->setChecked(true);
  setActiveTool("Mermaid");
  emit mermaidSelected();
}
void ToolPanel::onActionFill() {
  clearActiveToolStyles();
  actionFill->setChecked(true);
  setActiveTool("Fill");
  emit fillSelected();
}
void ToolPanel::onActionColorSelect() {
  clearActiveToolStyles();
  actionColorSelect->setChecked(true);
  setActiveTool("ColorSelect");
  emit colorSelectSelected();
}
void ToolPanel::onActionLine() {
  clearActiveToolStyles();
  actionLine->setChecked(true);
  setActiveTool("Line");
  emit shapeSelected("Line");
  emit lineSelected();
}
void ToolPanel::onActionArrow() {
  clearActiveToolStyles();
  actionArrow->setChecked(true);
  setActiveTool("Arrow");
  emit arrowSelected();
}
void ToolPanel::onActionCurvedArrow() {
  clearActiveToolStyles();
  actionCurvedArrow->setChecked(true);
  setActiveTool("CurvedArrow");
  emit curvedArrowSelected();
}
void ToolPanel::onActionWire() {
  clearActiveToolStyles();
  actionWire->setChecked(true);
  setActiveTool("Wire");
  emit wireSelected();
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
void ToolPanel::onActionSelection() {
  clearActiveToolStyles();
  actionSelection->setChecked(true);
  setActiveTool("Select");
  emit shapeSelected("Selection");
  emit selectionSelected();
}
void ToolPanel::onActionLassoSelection() {
  clearActiveToolStyles();
  actionLassoSelection->setChecked(true);
  setActiveTool("LassoSelect");
  emit shapeSelected("LassoSelection");
  emit lassoSelectionSelected();
}
void ToolPanel::onActionPan() {
  clearActiveToolStyles();
  actionPan->setChecked(true);
  setActiveTool("Pan");
  emit panSelected();
}
void ToolPanel::onActionBezier() {
  clearActiveToolStyles();
  actionBezier->setChecked(true);
  setActiveTool("Bezier");
  emit bezierSelected();
}
void ToolPanel::onActionTextOnPath() {
  clearActiveToolStyles();
  actionTextOnPath->setChecked(true);
  setActiveTool("TextOnPath");
  emit textOnPathSelected();
}

void ToolPanel::onActionColor() {
  QColor color = QColorDialog::getColor(Qt::white, this, "Select Color");
  if (color.isValid()) {
    updateColorDisplay(color);
    emit colorSelected(color);
  }
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

void ToolPanel::onFillStyleChanged(int index) {
  if (!fillStyleCombo_)
    return;

  int data = fillStyleCombo_->currentData().toInt();
  QBrush brush;

  if (data == -1) {
    // Linear gradient
    QLinearGradient lg(0, 0, 1, 1);
    lg.setCoordinateMode(QGradient::ObjectBoundingMode);
    lg.setColorAt(0, currentColor_);
    QColor endColor = currentColor_.lighter(180);
    lg.setColorAt(1, endColor);
    brush = QBrush(lg);
  } else if (data == -2) {
    // Radial gradient
    QRadialGradient rg(0.5, 0.5, 0.5);
    rg.setCoordinateMode(QGradient::ObjectBoundingMode);
    rg.setColorAt(0, currentColor_);
    QColor endColor = currentColor_.darker(200);
    rg.setColorAt(1, endColor);
    brush = QBrush(rg);
  } else if (data == -3) {
    // Conical gradient
    QConicalGradient cg(0.5, 0.5, 0);
    cg.setCoordinateMode(QGradient::ObjectBoundingMode);
    cg.setColorAt(0, currentColor_);
    cg.setColorAt(0.5, currentColor_.lighter(160));
    cg.setColorAt(1, currentColor_);
    brush = QBrush(cg);
  } else {
    // Solid or pattern fill
    brush = QBrush(currentColor_, static_cast<Qt::BrushStyle>(data));
  }

  emit fillBrushSelected(brush);
}

void ToolPanel::onBrushTipChanged(int) {
  if (!brushTipCombo_)
    return;
  int data = brushTipCombo_->currentData().toInt();
  BrushTip tip;
  tip.setShape(static_cast<BrushTipShape>(data));
  emit brushTipSelected(tip);
}

bool ToolPanel::eventFilter(QObject *obj, QEvent *event) {
  if (obj == colorPreview && event->type() == QEvent::MouseButtonRelease) {
    onActionColor();
    return true;
  }
  return QDockWidget::eventFilter(obj, event);
}
