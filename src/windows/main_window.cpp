// main_window.cpp
#include "main_window.h"
#include "../core/auto_save_manager.h"
#include "../core/layer.h"
#include "../core/recent_files_manager.h"
#include "../core/theme_manager.h"
#include "../widgets/canvas.h"
#include "../widgets/layer_panel.h"
#include "../widgets/tool_panel.h"
#include <QApplication>
#include <QColorDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)),
      _toolPanel(new ToolPanel(this)), _layerPanel(nullptr),
      _autoSaveManager(nullptr), _statusLabel(nullptr), 
      _recentFilesMenu(nullptr), _snapToGridAction(nullptr),
      _autoSaveAction(nullptr) {

  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_canvas);
  setCentralWidget(centralWidget);

  this->addToolBar(Qt::LeftToolBarArea, _toolPanel);
  this->setWindowTitle("FullScreen Pencil Draw - Professional Edition");
  this->resize(1400, 900);

  setupMenuBar();
  setupStatusBar();
  setupLayerPanel();
  setupAutoSave();
  setupConnections();

  _toolPanel->updateBrushSizeDisplay(_canvas->getCurrentBrushSize());
  _toolPanel->updateColorDisplay(_canvas->getCurrentColor());
  _toolPanel->updateZoomDisplay(_canvas->getCurrentZoom());
  _toolPanel->updateOpacityDisplay(_canvas->getCurrentOpacity());
}

MainWindow::~MainWindow() {}

void MainWindow::setupStatusBar() {
  _statusLabel = new QLabel("Ready | P:Pen E:Eraser T:Text F:Fill L:Line A:Arrow R:Rect C:Circle S:Select H:Pan | G:Grid B:Filled | Ctrl+Scroll:Zoom", this);
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
  connect(_toolPanel, &ToolPanel::toggleFilledShapesAction, _canvas, &Canvas::toggleFilledShapes);

  // Filled shapes feedback
  connect(_canvas, &Canvas::filledShapesChanged, this, &MainWindow::onFilledShapesChanged);
  
  // Snap to grid feedback
  connect(_canvas, &Canvas::snapToGridChanged, this, &MainWindow::onSnapToGridChanged);

  // File operations
  connect(_toolPanel, &ToolPanel::saveAction, _canvas, &Canvas::saveToFile);
  connect(_toolPanel, &ToolPanel::openAction, _canvas, &Canvas::openFile);
  connect(_toolPanel, &ToolPanel::newCanvasAction, this, &MainWindow::onNewCanvas);
  connect(_toolPanel, &ToolPanel::clearCanvas, _canvas, &Canvas::clearCanvas);
  
  // Recent files
  connect(&RecentFilesManager::instance(), &RecentFilesManager::recentFilesChanged,
          this, &MainWindow::onRecentFilesChanged);
}

void MainWindow::setupMenuBar() {
  QMenuBar *menuBar = this->menuBar();
  
  // File menu
  QMenu *fileMenu = menuBar->addMenu("&File");
  
  QAction *newAction = fileMenu->addAction("&New", QKeySequence::New, this, &MainWindow::onNewCanvas);
  QAction *openAction = fileMenu->addAction("&Open...", QKeySequence::Open, _canvas, &Canvas::openFile);
  
  // Recent Files submenu
  _recentFilesMenu = fileMenu->addMenu("Recent Files");
  updateRecentFilesMenu();
  
  fileMenu->addSeparator();
  
  QAction *saveAction = fileMenu->addAction("&Save...", QKeySequence::Save, _canvas, &Canvas::saveToFile);
  QAction *exportPdfAction = fileMenu->addAction("Export to &PDF...", _canvas, &Canvas::exportToPDF);
  
  fileMenu->addSeparator();
  
  QAction *exitAction = fileMenu->addAction("E&xit", QKeySequence::Quit, this, &QMainWindow::close);
  
  // Edit menu
  QMenu *editMenu = menuBar->addMenu("&Edit");
  
  editMenu->addAction("&Undo", QKeySequence::Undo, _canvas, &Canvas::undoLastAction);
  editMenu->addAction("&Redo", QKeySequence::Redo, _canvas, &Canvas::redoLastAction);
  editMenu->addSeparator();
  editMenu->addAction("Cu&t", QKeySequence::Cut, _canvas, &Canvas::cutSelectedItems);
  editMenu->addAction("&Copy", QKeySequence::Copy, _canvas, &Canvas::copySelectedItems);
  editMenu->addAction("&Paste", QKeySequence::Paste, _canvas, &Canvas::pasteItems);
  editMenu->addSeparator();
  editMenu->addAction("Select &All", QKeySequence::SelectAll, _canvas, &Canvas::selectAll);
  editMenu->addAction("&Delete", QKeySequence::Delete, _canvas, &Canvas::deleteSelectedItems);
  editMenu->addAction("D&uplicate", QKeySequence(Qt::CTRL | Qt::Key_D), _canvas, &Canvas::duplicateSelectedItems);
  
  // View menu
  QMenu *viewMenu = menuBar->addMenu("&View");
  
  viewMenu->addAction("Zoom &In", QKeySequence::ZoomIn, _canvas, &Canvas::zoomIn);
  viewMenu->addAction("Zoom &Out", QKeySequence::ZoomOut, _canvas, &Canvas::zoomOut);
  viewMenu->addAction("&Reset Zoom", QKeySequence(Qt::Key_0), _canvas, &Canvas::zoomReset);
  viewMenu->addSeparator();
  
  QAction *gridAction = viewMenu->addAction("Toggle &Grid", QKeySequence(Qt::Key_G), _canvas, &Canvas::toggleGrid);
  gridAction->setCheckable(true);
  
  _snapToGridAction = viewMenu->addAction("&Snap to Grid", QKeySequence(Qt::CTRL | Qt::Key_G), _canvas, &Canvas::toggleSnapToGrid);
  _snapToGridAction->setCheckable(true);
  
  QAction *filledAction = viewMenu->addAction("Toggle &Filled Shapes", QKeySequence(Qt::Key_B), _canvas, &Canvas::toggleFilledShapes);
  filledAction->setCheckable(true);
  
  viewMenu->addSeparator();
  
  // Theme submenu
  QMenu *themeMenu = viewMenu->addMenu("&Theme");
  QAction *toggleThemeAction = themeMenu->addAction("Toggle &Dark/Light Theme", this, &MainWindow::onToggleTheme);
  toggleThemeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
  
  // Tools menu
  QMenu *toolsMenu = menuBar->addMenu("&Tools");
  
  _autoSaveAction = toolsMenu->addAction("Enable &Auto-Save", [this]() {
    if (_autoSaveManager) {
      _autoSaveManager->setEnabled(!_autoSaveManager->isEnabled());
      _autoSaveAction->setChecked(_autoSaveManager->isEnabled());
    }
  });
  _autoSaveAction->setCheckable(true);
  _autoSaveAction->setChecked(true);  // Enabled by default
}

void MainWindow::updateRecentFilesMenu() {
  if (!_recentFilesMenu) return;
  
  _recentFilesMenu->clear();
  
  QStringList recentFiles = RecentFilesManager::instance().recentFiles();
  
  if (recentFiles.isEmpty()) {
    QAction *noFilesAction = _recentFilesMenu->addAction("No Recent Files");
    noFilesAction->setEnabled(false);
  } else {
    for (int i = 0; i < recentFiles.size(); ++i) {
      QString filePath = recentFiles.at(i);
      QString fileName = QFileInfo(filePath).fileName();
      QString text = QString("&%1. %2").arg(i + 1).arg(fileName);
      
      QAction *action = _recentFilesMenu->addAction(text);
      action->setData(filePath);
      action->setToolTip(filePath);
      connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }
    
    _recentFilesMenu->addSeparator();
    _recentFilesMenu->addAction("Clear Recent Files", []() {
      RecentFilesManager::instance().clearRecentFiles();
    });
  }
}

void MainWindow::onRecentFilesChanged() {
  updateRecentFilesMenu();
}

void MainWindow::openRecentFile() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString filePath = action->data().toString();
    _canvas->openRecentFile(filePath);
  }
}

void MainWindow::onToggleTheme() {
  ThemeManager::instance().toggleTheme();
}

void MainWindow::setupLayerPanel() {
  if (_canvas && _canvas->layerManager()) {
    _layerPanel = new LayerPanel(_canvas->layerManager(), this);
    addDockWidget(Qt::RightDockWidgetArea, _layerPanel);
  }
}

void MainWindow::onBrushSizeChanged(int size) { _toolPanel->updateBrushSizeDisplay(size); }
void MainWindow::onColorChanged(const QColor &color) { _toolPanel->updateColorDisplay(color); }
void MainWindow::onZoomChanged(double zoom) { _toolPanel->updateZoomDisplay(zoom); }
void MainWindow::onOpacityChanged(int opacity) { _toolPanel->updateOpacityDisplay(opacity); }
void MainWindow::onFilledShapesChanged(bool filled) { _toolPanel->updateFilledShapesDisplay(filled); }
void MainWindow::onCursorPositionChanged(const QPointF &pos) { _toolPanel->updatePositionDisplay(pos); }

void MainWindow::onSnapToGridChanged(bool enabled) {
  if (_snapToGridAction) {
    _snapToGridAction->setChecked(enabled);
  }
}

void MainWindow::setupAutoSave() {
  _autoSaveManager = new AutoSaveManager(_canvas, this);
  
  connect(_autoSaveManager, &AutoSaveManager::autoSavePerformed,
          this, &MainWindow::onAutoSavePerformed);
  
  // Check for recovery on startup
  if (_autoSaveManager->hasAutoSave()) {
    _autoSaveManager->restoreAutoSave();
  }
  
  // Update the menu action state
  if (_autoSaveAction) {
    _autoSaveAction->setChecked(_autoSaveManager->isEnabled());
  }
}

void MainWindow::onAutoSavePerformed(const QString &path) {
  statusBar()->showMessage(QString("Auto-saved to: %1").arg(path), 3000);
}

void MainWindow::onNewCanvas() {
  bool ok;
  int width = QInputDialog::getInt(this, "New Canvas", "Width:", 1920, 100, 10000, 100, &ok);
  if (!ok) return;
  int height = QInputDialog::getInt(this, "New Canvas", "Height:", 1080, 100, 10000, 100, &ok);
  if (!ok) return;
  QColor bgColor = QColorDialog::getColor(Qt::black, this, "Background Color");
  if (!bgColor.isValid()) return;
  _canvas->newCanvas(width, height, bgColor);
  // Refresh layer panel after new canvas
  if (_layerPanel) {
    _layerPanel->refreshLayerList();
  }
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
  else if (event->key() == Qt::Key_B) { _canvas->toggleFilledShapes(); }
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
