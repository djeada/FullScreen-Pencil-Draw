// main_window.cpp
#include "main_window.h"
#include "../widgets/canvas.h"
#include "../widgets/tool_panel.h"
#include <QApplication>
#include <QColorDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)),
      _toolPanel(new ToolPanel(this)), _statusLabel(nullptr) {

  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_canvas);
  setCentralWidget(centralWidget);

  this->addToolBar(Qt::LeftToolBarArea, _toolPanel);
  this->setWindowTitle("FullScreen Pencil Draw - Professional Edition");
  this->resize(1400, 900);

  setupStatusBar();
  setupConnections();

  _toolPanel->updateBrushSizeDisplay(_canvas->getCurrentBrushSize());
  _toolPanel->updateColorDisplay(_canvas->getCurrentColor());
  _toolPanel->updateZoomDisplay(_canvas->getCurrentZoom());
  _toolPanel->updateOpacityDisplay(_canvas->getCurrentOpacity());
}

MainWindow::~MainWindow() {}

void MainWindow::setupStatusBar() {
  _statusLabel = new QLabel("Ready | P:Pen E:Eraser T:Text F:Fill L:Line A:Arrow R:Rect C:Circle S:Select H:Pan G:Grid | Ctrl+Scroll:Zoom", this);
  statusBar()->addWidget(_statusLabel);
  statusBar()->setStyleSheet("QStatusBar { background-color: #2d2d2d; color: #ffffff; }");
}

void MainWindow::setupConnections() {
  // Tool selections
  connect(_toolPanel, &ToolPanel::penSelected, _canvas, &Canvas::setPenTool);
  connect(_toolPanel, &ToolPanel::eraserSelected, _canvas, &Canvas::setEraserTool);
  connect(_toolPanel, &ToolPanel::textSelected, _canvas, &Canvas::setTextTool);
  connect(_toolPanel, &ToolPanel::fillSelected, _canvas, &Canvas::setFillTool);
  connect(_toolPanel, &ToolPanel::arrowSelected, _canvas, &Canvas::setArrowTool);
  connect(_toolPanel, &ToolPanel::panSelected, _canvas, &Canvas::setPanTool);
  connect(_toolPanel, &ToolPanel::colorSelected, _canvas, &Canvas::setPenColor);
  connect(_toolPanel, &ToolPanel::opacitySelected, _canvas, &Canvas::setOpacity);

  // Shape tools
  connect(_toolPanel, &ToolPanel::rectangleSelected, _canvas, [this]() { _canvas->setShape("Rectangle"); });
  connect(_toolPanel, &ToolPanel::circleSelected, _canvas, [this]() { _canvas->setShape("Circle"); });
  connect(_toolPanel, &ToolPanel::lineSelected, _canvas, [this]() { _canvas->setShape("Line"); });
  connect(_toolPanel, &ToolPanel::selectionSelected, _canvas, [this]() { _canvas->setShape("Selection"); });

  // Edit operations
  connect(_toolPanel, &ToolPanel::copyAction, _canvas, &Canvas::copySelectedItems);
  connect(_toolPanel, &ToolPanel::cutAction, _canvas, &Canvas::cutSelectedItems);
  connect(_toolPanel, &ToolPanel::pasteAction, _canvas, &Canvas::pasteItems);
  connect(_toolPanel, &ToolPanel::duplicateAction, _canvas, &Canvas::duplicateSelectedItems);
  connect(_toolPanel, &ToolPanel::deleteAction, _canvas, &Canvas::deleteSelectedItems);
  connect(_toolPanel, &ToolPanel::selectAllAction, _canvas, &Canvas::selectAll);

  // Brush controls
  connect(_toolPanel, &ToolPanel::increaseBrushSize, _canvas, &Canvas::increaseBrushSize);
  connect(_toolPanel, &ToolPanel::decreaseBrushSize, _canvas, &Canvas::decreaseBrushSize);

  // Canvas feedback to ToolPanel
  connect(_canvas, &Canvas::brushSizeChanged, this, &MainWindow::onBrushSizeChanged);
  connect(_canvas, &Canvas::colorChanged, this, &MainWindow::onColorChanged);
  connect(_canvas, &Canvas::zoomChanged, this, &MainWindow::onZoomChanged);
  connect(_canvas, &Canvas::opacityChanged, this, &MainWindow::onOpacityChanged);
  connect(_canvas, &Canvas::cursorPositionChanged, this, &MainWindow::onCursorPositionChanged);

  // Undo/Redo
  connect(_toolPanel, &ToolPanel::undoAction, _canvas, &Canvas::undoLastAction);
  connect(_toolPanel, &ToolPanel::redoAction, _canvas, &Canvas::redoLastAction);

  // Zoom
  connect(_toolPanel, &ToolPanel::zoomInAction, _canvas, &Canvas::zoomIn);
  connect(_toolPanel, &ToolPanel::zoomOutAction, _canvas, &Canvas::zoomOut);
  connect(_toolPanel, &ToolPanel::zoomResetAction, _canvas, &Canvas::zoomReset);
  connect(_toolPanel, &ToolPanel::toggleGridAction, _canvas, &Canvas::toggleGrid);

  // File operations
  connect(_toolPanel, &ToolPanel::saveAction, _canvas, &Canvas::saveToFile);
  connect(_toolPanel, &ToolPanel::openAction, _canvas, &Canvas::openFile);
  connect(_toolPanel, &ToolPanel::newCanvasAction, this, &MainWindow::onNewCanvas);
  connect(_toolPanel, &ToolPanel::clearCanvas, _canvas, &Canvas::clearCanvas);
}

void MainWindow::onBrushSizeChanged(int size) { _toolPanel->updateBrushSizeDisplay(size); }
void MainWindow::onColorChanged(const QColor &color) { _toolPanel->updateColorDisplay(color); }
void MainWindow::onZoomChanged(double zoom) { _toolPanel->updateZoomDisplay(zoom); }
void MainWindow::onOpacityChanged(int opacity) { _toolPanel->updateOpacityDisplay(opacity); }
void MainWindow::onCursorPositionChanged(const QPointF &pos) { _toolPanel->updatePositionDisplay(pos); }

void MainWindow::onNewCanvas() {
  bool ok;
  int width = QInputDialog::getInt(this, "New Canvas", "Width:", 1920, 100, 10000, 100, &ok);
  if (!ok) return;
  int height = QInputDialog::getInt(this, "New Canvas", "Height:", 1080, 100, 10000, 100, &ok);
  if (!ok) return;
  QColor bgColor = QColorDialog::getColor(Qt::black, this, "Background Color");
  if (!bgColor.isValid()) return;
  _canvas->newCanvas(width, height, bgColor);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  // Standard shortcuts
  if (event->matches(QKeySequence::Copy)) { _canvas->copySelectedItems(); }
  else if (event->matches(QKeySequence::Cut)) { _canvas->cutSelectedItems(); }
  else if (event->matches(QKeySequence::Paste)) { _canvas->pasteItems(); }
  else if (event->matches(QKeySequence::Undo)) { _canvas->undoLastAction(); }
  else if (event->matches(QKeySequence::Redo)) { _canvas->redoLastAction(); }
  else if (event->matches(QKeySequence::Save)) { _canvas->saveToFile(); }
  else if (event->matches(QKeySequence::Open)) { _canvas->openFile(); }
  else if (event->matches(QKeySequence::New)) { onNewCanvas(); }
  else if (event->matches(QKeySequence::SelectAll)) { _canvas->selectAll(); }
  else if (event->key() == Qt::Key_Escape) { this->close(); }
  else if (event->key() == Qt::Key_Delete) { _canvas->deleteSelectedItems(); }
  // Tool shortcuts
  else if (event->key() == Qt::Key_P) { _toolPanel->onActionPen(); }
  else if (event->key() == Qt::Key_E) { _toolPanel->onActionEraser(); }
  else if (event->key() == Qt::Key_T) { _toolPanel->onActionText(); }
  else if (event->key() == Qt::Key_F) { _toolPanel->onActionFill(); }
  else if (event->key() == Qt::Key_L) { _toolPanel->onActionLine(); }
  else if (event->key() == Qt::Key_A && !(event->modifiers() & Qt::ControlModifier)) { _toolPanel->onActionArrow(); }
  else if (event->key() == Qt::Key_R) { _toolPanel->onActionRectangle(); }
  else if (event->key() == Qt::Key_C && !(event->modifiers() & Qt::ControlModifier)) { _toolPanel->onActionCircle(); }
  else if (event->key() == Qt::Key_S && !(event->modifiers() & Qt::ControlModifier)) { _toolPanel->onActionSelection(); }
  else if (event->key() == Qt::Key_H) { _toolPanel->onActionPan(); }
  else if (event->key() == Qt::Key_G) { _canvas->toggleGrid(); }
  else if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) { _canvas->duplicateSelectedItems(); }
  // Brush size
  else if (event->key() == Qt::Key_BracketRight) { _canvas->increaseBrushSize(); }
  else if (event->key() == Qt::Key_BracketLeft) { _canvas->decreaseBrushSize(); }
  // Zoom
  else if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) { _canvas->zoomIn(); }
  else if (event->key() == Qt::Key_Minus) { _canvas->zoomOut(); }
  else if (event->key() == Qt::Key_0) { _canvas->zoomReset(); }
  else { QMainWindow::keyPressEvent(event); }
}
