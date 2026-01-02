// main_window.cpp
#include "main_window.h"
#include "../widgets/canvas.h"
#include "../widgets/tool_panel.h"
#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)),
      _toolPanel(new ToolPanel(this)), _statusLabel(nullptr) {

  // Set up the layout for the main window
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);

  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_canvas);
  setCentralWidget(centralWidget);

  // Add the ToolPanel to the left side as a toolbar
  this->addToolBar(Qt::LeftToolBarArea, _toolPanel);

  // Set main window properties
  this->setWindowTitle("FullScreen Pencil Draw");

  // Setup status bar and connections
  setupStatusBar();
  setupConnections();

  // Initialize tool panel displays
  _toolPanel->updateBrushSizeDisplay(_canvas->getCurrentBrushSize());
  _toolPanel->updateColorDisplay(_canvas->getCurrentColor());
}

MainWindow::~MainWindow() {}

void MainWindow::setupStatusBar() {
  _statusLabel = new QLabel("Ready | Ctrl+Z: Undo | Ctrl+Y: Redo | Ctrl+S: "
                            "Save | Ctrl+Scroll: Zoom",
                            this);
  statusBar()->addWidget(_statusLabel);
  statusBar()->setStyleSheet(
      "QStatusBar { background-color: #2d2d2d; color: #ffffff; }");
}

void MainWindow::setupConnections() {
  // Connect ToolPanel signals to Canvas slots
  connect(_toolPanel, &ToolPanel::penSelected, _canvas, &Canvas::setPenTool);
  connect(_toolPanel, &ToolPanel::eraserSelected, _canvas,
          &Canvas::setEraserTool);
  connect(_toolPanel, &ToolPanel::colorSelected, _canvas, &Canvas::setPenColor);

  // Shape tools
  connect(_toolPanel, &ToolPanel::rectangleSelected, _canvas,
          [this]() { _canvas->setShape("Rectangle"); });
  connect(_toolPanel, &ToolPanel::circleSelected, _canvas,
          [this]() { _canvas->setShape("Circle"); });
  connect(_toolPanel, &ToolPanel::lineSelected, _canvas,
          [this]() { _canvas->setShape("Line"); });
  connect(_toolPanel, &ToolPanel::selectionSelected, _canvas,
          [this]() { _canvas->setShape("Selection"); });

  // Copy, Cut, Paste
  connect(_toolPanel, &ToolPanel::copyAction, _canvas,
          &Canvas::copySelectedItems);
  connect(_toolPanel, &ToolPanel::cutAction, _canvas, &Canvas::cutSelectedItems);
  connect(_toolPanel, &ToolPanel::pasteAction, _canvas, &Canvas::pasteItems);

  // Brush size controls
  connect(_toolPanel, &ToolPanel::increaseBrushSize, _canvas,
          &Canvas::increaseBrushSize);
  connect(_toolPanel, &ToolPanel::decreaseBrushSize, _canvas,
          &Canvas::decreaseBrushSize);

  // Canvas to ToolPanel updates
  connect(_canvas, &Canvas::brushSizeChanged, this,
          &MainWindow::onBrushSizeChanged);
  connect(_canvas, &Canvas::colorChanged, this, &MainWindow::onColorChanged);

  // Undo/Redo
  connect(_toolPanel, &ToolPanel::undoAction, _canvas, &Canvas::undoLastAction);
  connect(_toolPanel, &ToolPanel::redoAction, _canvas, &Canvas::redoLastAction);

  // Zoom
  connect(_toolPanel, &ToolPanel::zoomInAction, _canvas, &Canvas::zoomIn);
  connect(_toolPanel, &ToolPanel::zoomOutAction, _canvas, &Canvas::zoomOut);

  // Save and Clear
  connect(_toolPanel, &ToolPanel::saveAction, _canvas, &Canvas::saveToFile);
  connect(_toolPanel, &ToolPanel::clearCanvas, _canvas, &Canvas::clearCanvas);
}

void MainWindow::onBrushSizeChanged(int size) {
  _toolPanel->updateBrushSizeDisplay(size);
}

void MainWindow::onColorChanged(const QColor &color) {
  _toolPanel->updateColorDisplay(color);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  // Standard shortcuts
  if (event->matches(QKeySequence::Copy)) {
    _canvas->copySelectedItems();
  } else if (event->matches(QKeySequence::Cut)) {
    _canvas->cutSelectedItems();
  } else if (event->matches(QKeySequence::Paste)) {
    _canvas->pasteItems();
  } else if (event->matches(QKeySequence::Undo)) {
    _canvas->undoLastAction();
  } else if (event->matches(QKeySequence::Redo)) {
    _canvas->redoLastAction();
  } else if (event->matches(QKeySequence::Save)) {
    _canvas->saveToFile();
  } else if (event->key() == Qt::Key_Escape) {
    this->close();
  }
  // Tool shortcuts
  else if (event->key() == Qt::Key_P) {
    _toolPanel->onActionPen();
  } else if (event->key() == Qt::Key_E) {
    _toolPanel->onActionEraser();
  } else if (event->key() == Qt::Key_L) {
    _toolPanel->onActionLine();
  } else if (event->key() == Qt::Key_R) {
    _toolPanel->onActionRectangle();
  } else if (event->key() == Qt::Key_C && !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionCircle();
  } else if (event->key() == Qt::Key_S && !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionSelection();
  }
  // Brush size shortcuts
  else if (event->key() == Qt::Key_BracketRight) {
    _canvas->increaseBrushSize();
  } else if (event->key() == Qt::Key_BracketLeft) {
    _canvas->decreaseBrushSize();
  }
  // Zoom shortcuts
  else if (event->key() == Qt::Key_Plus ||
           event->key() == Qt::Key_Equal) {
    _canvas->zoomIn();
  } else if (event->key() == Qt::Key_Minus) {
    _canvas->zoomOut();
  } else {
    QMainWindow::keyPressEvent(event);
  }
}
