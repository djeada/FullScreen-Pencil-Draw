// main_window.cpp
#include "main_window.h"
#include "../core/auto_save_manager.h"
#include "../core/layer.h"
#include "../core/recent_files_manager.h"
#include "../core/theme_manager.h"
#include "../widgets/canvas.h"
#include "../widgets/layer_panel.h"
#include "../widgets/tool_panel.h"
#include "../tools/tool_manager.h"
#include <QApplication>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#ifdef HAVE_QT_PDF
#include "../widgets/pdf_viewer.h"
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)),
      _toolPanel(new ToolPanel(this)), _layerPanel(nullptr),
      _autoSaveManager(nullptr), _statusLabel(nullptr), 
      _measurementLabel(nullptr), _recentFilesMenu(nullptr), 
      _snapToGridAction(nullptr), _autoSaveAction(nullptr),
      _rulerAction(nullptr), _measurementAction(nullptr)
#ifdef HAVE_QT_PDF
      , _pdfViewer(nullptr), _centralSplitter(nullptr), _pdfPanel(nullptr),
      _pdfToolBar(nullptr), _pdfPageLabel(nullptr), _pdfDarkModeAction(nullptr),
      _pdfPanelOnLeft(false)  // PDF panel starts on the right by default
#endif
{
#ifdef HAVE_QT_PDF
  // Split ratio constants for canvas/PDF panel
  static constexpr double PDF_PANEL_SPLIT_RATIO = 0.4;  // PDF panel takes 40% when visible
  
  // Create splitter for side-by-side canvas and PDF viewer
  _centralSplitter = new QSplitter(Qt::Horizontal, this);
  _centralSplitter->addWidget(_canvas);
  setupPdfViewer();
  setCentralWidget(_centralSplitter);
  
  // Set initial sizes (canvas takes full space, PDF panel hidden initially)
  _centralSplitter->setSizes({1, 0});
#else
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_canvas);
  setCentralWidget(centralWidget);
#endif

  this->addToolBar(Qt::LeftToolBarArea, _toolPanel);
  this->setWindowTitle("FullScreen Pencil Draw - Professional Edition");
  this->resize(1400, 900);

  setupMenuBar();
  setupStatusBar();
  setupLayerPanel();
  setupAutoSave();
  setupConnections();

#ifdef HAVE_QT_PDF
  setupPdfToolBar();
#endif

  _toolPanel->updateBrushSizeDisplay(_canvas->getCurrentBrushSize());
  _toolPanel->updateColorDisplay(_canvas->getCurrentColor());
  _toolPanel->updateZoomDisplay(_canvas->getCurrentZoom());
  _toolPanel->updateOpacityDisplay(_canvas->getCurrentOpacity());
}

MainWindow::~MainWindow() {}

void MainWindow::setupStatusBar() {
  _statusLabel = new QLabel("✦ Ready | P:Pen E:Eraser T:Text F:Fill L:Line A:Arrow R:Rect C:Circle S:Select H:Pan | G:Grid B:Filled | Ctrl+Scroll:Zoom", this);
  _measurementLabel = new QLabel("", this);
  statusBar()->addWidget(_statusLabel);
  statusBar()->addPermanentWidget(_measurementLabel);
  statusBar()->setStyleSheet(R"(
    QStatusBar { 
      background-color: #161618; 
      color: #a0a0a8; 
      border-top: 1px solid rgba(255, 255, 255, 0.06);
      padding: 6px 12px;
    }
    QStatusBar::item {
      border: none;
    }
    QLabel {
      color: #a0a0a8;
      font-size: 11px;
      font-weight: 500;
    }
  )");
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

#ifdef HAVE_QT_PDF
  // Connect tool selections and settings to PDF viewer as well
  // This allows the same toolbar to control both canvas and PDF viewer
  if (_pdfViewer) {
    // Tool selections - route to PDF viewer so tools work on both renderers
    connect(_toolPanel, &ToolPanel::penSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Pen);
    });
    connect(_toolPanel, &ToolPanel::eraserSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Eraser);
    });
    connect(_toolPanel, &ToolPanel::textSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Text);
    });
    connect(_toolPanel, &ToolPanel::fillSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Fill);
    });
    connect(_toolPanel, &ToolPanel::arrowSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Arrow);
    });
    connect(_toolPanel, &ToolPanel::panSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Pan);
    });
    connect(_toolPanel, &ToolPanel::rectangleSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Rectangle);
    });
    connect(_toolPanel, &ToolPanel::circleSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Circle);
    });
    connect(_toolPanel, &ToolPanel::lineSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Line);
    });
    connect(_toolPanel, &ToolPanel::selectionSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Selection);
    });

    // Connect color selection to PDF viewer
    connect(_toolPanel, &ToolPanel::colorSelected, _pdfViewer, &PdfViewer::setPenColor);
    // Connect brush size changes to PDF viewer
    connect(_canvas, &Canvas::brushSizeChanged, _pdfViewer, &PdfViewer::setPenWidth);
    // Connect filled shapes toggle to PDF viewer
    connect(_canvas, &Canvas::filledShapesChanged, _pdfViewer, &PdfViewer::setFilledShapes);
  }
#endif

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
  
  // Ruler and measurement feedback
  connect(_canvas, &Canvas::rulerVisibilityChanged, this, &MainWindow::onRulerVisibilityChanged);
  connect(_canvas, &Canvas::measurementToolChanged, this, &MainWindow::onMeasurementToolChanged);
  connect(_canvas, &Canvas::measurementUpdated, this, &MainWindow::onMeasurementUpdated);

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
  
  fileMenu->addAction("&New", QKeySequence::New, this, &MainWindow::onNewCanvas);
  fileMenu->addAction("&Open...", QKeySequence::Open, _canvas, &Canvas::openFile);

#ifdef HAVE_QT_PDF
  fileMenu->addAction("Open &PDF...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), this, &MainWindow::onOpenPdf);
#endif
  
  // Recent Files submenu
  _recentFilesMenu = fileMenu->addMenu("Recent Files");
  updateRecentFilesMenu();
  
  fileMenu->addSeparator();
  
  fileMenu->addAction("&Save...", QKeySequence::Save, _canvas, &Canvas::saveToFile);
  fileMenu->addAction("Export to &PDF...", _canvas, &Canvas::exportToPDF);

#ifdef HAVE_QT_PDF
  fileMenu->addAction("Export &Annotated PDF...", this, &MainWindow::onExportAnnotatedPdf);
  fileMenu->addAction("&Close PDF", this, &MainWindow::onClosePdf);
#endif
  
  fileMenu->addSeparator();
  
  fileMenu->addAction("E&xit", QKeySequence::Quit, this, &QMainWindow::close);
  
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
  gridAction->setChecked(_canvas->isGridVisible());
  
  _snapToGridAction = viewMenu->addAction("&Snap to Grid", QKeySequence(Qt::CTRL | Qt::Key_G), _canvas, &Canvas::toggleSnapToGrid);
  _snapToGridAction->setCheckable(true);
  _snapToGridAction->setChecked(_canvas->isSnapToGridEnabled());
  
  QAction *filledAction = viewMenu->addAction("Toggle &Filled Shapes", QKeySequence(Qt::Key_B), _canvas, &Canvas::toggleFilledShapes);
  filledAction->setCheckable(true);
  filledAction->setChecked(_canvas->isFilledShapes());
  
  viewMenu->addSeparator();
  
  _rulerAction = viewMenu->addAction("Show &Ruler", QKeySequence(Qt::CTRL | Qt::Key_R), _canvas, &Canvas::toggleRuler);
  _rulerAction->setCheckable(true);
  _rulerAction->setChecked(_canvas->isRulerVisible());
  
  _measurementAction = viewMenu->addAction("&Measurement Tool", QKeySequence(Qt::Key_M), _canvas, &Canvas::toggleMeasurementTool);
  _measurementAction->setCheckable(true);
  _measurementAction->setChecked(_canvas->isMeasurementToolEnabled());
  
  viewMenu->addSeparator();
  
#ifdef HAVE_QT_PDF
  // Panel order option
  viewMenu->addAction("S&wap Panel Order", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W), this, &MainWindow::swapPanelOrder);
  viewMenu->addSeparator();
#endif
  
  // Theme submenu
  QMenu *themeMenu = viewMenu->addMenu("&Theme");
  QAction *toggleThemeAction = themeMenu->addAction("Toggle &Dark/Light Theme", this, &MainWindow::onToggleTheme);
  toggleThemeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
  
  // Edit menu - add lock/unlock after other edit items
  editMenu->addSeparator();
  editMenu->addAction("&Lock Selected", QKeySequence(Qt::CTRL | Qt::Key_L), _canvas, &Canvas::lockSelectedItems);
  editMenu->addAction("&Unlock All", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L), _canvas, &Canvas::unlockSelectedItems);
  
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

void MainWindow::onRulerVisibilityChanged(bool visible) {
  if (_rulerAction) {
    _rulerAction->setChecked(visible);
  }
}

void MainWindow::onMeasurementToolChanged(bool enabled) {
  if (_measurementAction) {
    _measurementAction->setChecked(enabled);
  }
  if (!enabled && _measurementLabel) {
    _measurementLabel->clear();
  }
}

void MainWindow::onMeasurementUpdated(const QString &measurement) {
  if (_measurementLabel) {
    _measurementLabel->setText(QString("Distance: %1").arg(measurement));
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
#ifdef HAVE_QT_PDF
  // PDF navigation shortcuts (only work when PDF panel is visible)
  else if (event->key() == Qt::Key_PageDown && _pdfPanel && _pdfPanel->isVisible() && _pdfViewer) { _pdfViewer->nextPage(); }
  else if (event->key() == Qt::Key_PageUp && _pdfPanel && _pdfPanel->isVisible() && _pdfViewer) { _pdfViewer->previousPage(); }
  else if (event->key() == Qt::Key_Home && _pdfPanel && _pdfPanel->isVisible() && _pdfViewer) { _pdfViewer->firstPage(); }
  else if (event->key() == Qt::Key_End && _pdfPanel && _pdfPanel->isVisible() && _pdfViewer) { _pdfViewer->lastPage(); }
#endif
  // Brush size
  else if (event->key() == Qt::Key_BracketRight) { _canvas->increaseBrushSize(); }
  else if (event->key() == Qt::Key_BracketLeft) { _canvas->decreaseBrushSize(); }
  // Zoom
  else if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) { _canvas->zoomIn(); }
  else if (event->key() == Qt::Key_Minus) { _canvas->zoomOut(); }
  else if (event->key() == Qt::Key_0) { _canvas->zoomReset(); }
  else { QMainWindow::keyPressEvent(event); }
}

#ifdef HAVE_QT_PDF
void MainWindow::setupPdfViewer() {
  // Create PDF panel with its own layout
  _pdfPanel = new QFrame(this);
  _pdfPanel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  _pdfPanel->setMinimumWidth(200);
  
  QVBoxLayout *pdfLayout = new QVBoxLayout(_pdfPanel);
  pdfLayout->setContentsMargins(0, 0, 0, 0);
  pdfLayout->setSpacing(0);
  
  // Create header label for PDF panel
  QLabel *pdfHeader = new QLabel("PDF Viewer", _pdfPanel);
  pdfHeader->setAlignment(Qt::AlignCenter);
  pdfHeader->setStyleSheet("QLabel { background-color: #3a3a40; color: white; padding: 8px; font-weight: bold; }");
  pdfLayout->addWidget(pdfHeader);
  
  // Create PDF viewer widget
  _pdfViewer = new PdfViewer(_pdfPanel);
  pdfLayout->addWidget(_pdfViewer, 1);
  
  // Add PDF panel to splitter (initially hidden)
  _centralSplitter->addWidget(_pdfPanel);
  _pdfPanel->hide();

  // Connect PDF viewer signals
  connect(_pdfViewer, &PdfViewer::pageChanged, this, &MainWindow::onPdfPageChanged);
  connect(_pdfViewer, &PdfViewer::zoomChanged, this, &MainWindow::onPdfZoomChanged);
  connect(_pdfViewer, &PdfViewer::darkModeChanged, this, &MainWindow::onPdfDarkModeChanged);
  connect(_pdfViewer, &PdfViewer::cursorPositionChanged, this, &MainWindow::onCursorPositionChanged);
  connect(_pdfViewer, &PdfViewer::pdfLoaded, this, [this]() {
    showPdfPanel();
  });
  connect(_pdfViewer, &PdfViewer::pdfClosed, this, [this]() {
    hidePdfPanel();
  });
  connect(_pdfViewer, &PdfViewer::errorOccurred, this, [this](const QString &message) {
    statusBar()->showMessage(QString("PDF Error: %1").arg(message), 5000);
  });
  
  // Connect screenshot signal to add captured image to main canvas
  connect(_pdfViewer, &PdfViewer::screenshotCaptured, _canvas, &Canvas::addImageFromScreenshot);
  connect(_pdfViewer, &PdfViewer::screenshotCaptured, this, [this]() {
    statusBar()->showMessage("Screenshot captured and added to canvas", 3000);
  });
  
  // Connect drag-drop signals from PDF viewer and canvas
  connect(_pdfViewer, &PdfViewer::pdfFileDropped, this, &MainWindow::onPdfFileDropped);
  connect(_canvas, &Canvas::pdfFileDropped, this, &MainWindow::onPdfFileDropped);
}

void MainWindow::setupPdfToolBar() {
  _pdfToolBar = new QToolBar("PDF Navigation", this);
  _pdfToolBar->setObjectName("pdfToolBar");
  addToolBar(Qt::TopToolBarArea, _pdfToolBar);
  _pdfToolBar->hide(); // Hidden until PDF is loaded

  // Navigation buttons
  _pdfToolBar->addAction("⏮ First", _pdfViewer, &PdfViewer::firstPage);
  _pdfToolBar->addAction("◀ Previous", _pdfViewer, &PdfViewer::previousPage);
  
  // Page indicator
  _pdfPageLabel = new QLabel("Page 0 / 0", this);
  _pdfPageLabel->setMinimumWidth(100);
  _pdfPageLabel->setAlignment(Qt::AlignCenter);
  _pdfToolBar->addWidget(_pdfPageLabel);
  
  _pdfToolBar->addAction("Next ▶", _pdfViewer, &PdfViewer::nextPage);
  _pdfToolBar->addAction("Last ⏭", _pdfViewer, &PdfViewer::lastPage);
  
  _pdfToolBar->addSeparator();
  
  // Zoom controls
  _pdfToolBar->addAction("Zoom In", _pdfViewer, &PdfViewer::zoomIn);
  _pdfToolBar->addAction("Zoom Out", _pdfViewer, &PdfViewer::zoomOut);
  _pdfToolBar->addAction("Reset Zoom", _pdfViewer, &PdfViewer::zoomReset);
  
  _pdfToolBar->addSeparator();
  
  // Dark mode toggle
  _pdfDarkModeAction = _pdfToolBar->addAction("Dark Mode", [this]() {
    _pdfViewer->setDarkMode(!_pdfViewer->darkMode());
  });
  _pdfDarkModeAction->setCheckable(true);
  
  _pdfToolBar->addSeparator();
  
  // Screenshot tool (PDF-specific - not in main toolbar)
  _pdfToolBar->addAction("📷 Screenshot", [this]() {
    _pdfViewer->setScreenshotSelectionMode(true);
  });
  
  _pdfToolBar->addSeparator();
  
  // Undo/Redo
  _pdfToolBar->addAction("Undo", _pdfViewer, &PdfViewer::undo);
  _pdfToolBar->addAction("Redo", _pdfViewer, &PdfViewer::redo);
  
  _pdfToolBar->addSeparator();
  
  // Close PDF
  _pdfToolBar->addAction("Close PDF", this, &MainWindow::onClosePdf);
}

void MainWindow::showPdfPanel() {
  // Split ratio: canvas gets 60%, PDF panel gets 40%
  static constexpr double CANVAS_RATIO = 0.6;
  static constexpr double PDF_RATIO = 0.4;
  
  if (_pdfPanel) {
    _pdfPanel->show();
    // Set splitter sizes to show both canvas and PDF panel
    int totalWidth = _centralSplitter->width();
    int canvasWidth = static_cast<int>(totalWidth * CANVAS_RATIO);
    int pdfWidth = static_cast<int>(totalWidth * PDF_RATIO);
    _centralSplitter->setSizes({canvasWidth, pdfWidth});
  }
  
  // Sync PDF viewer settings with canvas settings
  if (_pdfViewer && _canvas) {
    _pdfViewer->setPenColor(_canvas->getCurrentColor());
    _pdfViewer->setPenWidth(_canvas->getCurrentBrushSize());
    _pdfViewer->setFilledShapes(_canvas->isFilledShapes());
  }
  
  _pdfToolBar->show();
  setWindowTitle("FullScreen Pencil Draw - PDF Annotation Mode");
}

void MainWindow::hidePdfPanel() {
  if (_pdfPanel) {
    _pdfPanel->hide();
    // Give full width back to canvas (use 1:0 ratio)
    _centralSplitter->setSizes({1, 0});
  }
  _pdfToolBar->hide();
  setWindowTitle("FullScreen Pencil Draw - Professional Edition");
}

void MainWindow::onOpenPdf() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Open PDF File", "", "PDF Files (*.pdf);;All Files (*)");
  if (fileName.isEmpty()) {
    return;
  }
  
  if (!_pdfViewer->openPdf(fileName)) {
    statusBar()->showMessage("Failed to open PDF file", 3000);
  }
}

void MainWindow::onClosePdf() {
  if (_pdfViewer) {
    _pdfViewer->closePdf();
  }
  hidePdfPanel();
}

void MainWindow::onPdfPageChanged(int pageIndex, int pageCount) {
  if (_pdfPageLabel) {
    _pdfPageLabel->setText(QString("Page %1 / %2").arg(pageIndex + 1).arg(pageCount));
  }
}

void MainWindow::onPdfZoomChanged(double zoom) {
  if (_toolPanel) {
    _toolPanel->updateZoomDisplay(zoom);
  }
}

void MainWindow::onPdfDarkModeChanged(bool enabled) {
  if (_pdfDarkModeAction) {
    _pdfDarkModeAction->setChecked(enabled);
  }
}

void MainWindow::onPdfFileDropped(const QString &filePath) {
  if (_pdfViewer && _pdfViewer->openPdf(filePath)) {
    statusBar()->showMessage(QString("Opened PDF: %1").arg(QFileInfo(filePath).fileName()), 3000);
  } else {
    statusBar()->showMessage("Failed to open PDF file", 3000);
  }
}

void MainWindow::onExportAnnotatedPdf() {
  if (!_pdfViewer || !_pdfViewer->hasPdf()) {
    statusBar()->showMessage("No PDF loaded to export", 3000);
    return;
  }
  
  QString fileName = QFileDialog::getSaveFileName(
      this, "Export Annotated PDF", "", "PDF Files (*.pdf)");
  if (fileName.isEmpty()) {
    return;
  }
  
  if (_pdfViewer->exportAnnotatedPdf(fileName)) {
    statusBar()->showMessage(QString("Exported annotated PDF to: %1").arg(fileName), 3000);
  } else {
    statusBar()->showMessage("Failed to export annotated PDF", 3000);
  }
}

void MainWindow::swapPanelOrder() {
  if (!_centralSplitter || !_pdfPanel || !_canvas) {
    return;
  }
  
  // Store current sizes to preserve the layout proportions
  QList<int> currentSizes = _centralSplitter->sizes();
  
  // Toggle the panel position flag
  _pdfPanelOnLeft = !_pdfPanelOnLeft;
  
  // Remove both widgets from the splitter first to ensure clean repositioning
  // QSplitter::insertWidget handles this internally, but explicit removal is clearer
  if (_pdfPanelOnLeft) {
    // PDF panel on left (index 0), Canvas on right (index 1)
    _centralSplitter->insertWidget(0, _pdfPanel);
    _centralSplitter->insertWidget(1, _canvas);
  } else {
    // Canvas on left (index 0), PDF panel on right (index 1)
    _centralSplitter->insertWidget(0, _canvas);
    _centralSplitter->insertWidget(1, _pdfPanel);
  }
  
  // Swap the sizes to maintain the visual proportions after swapping
  // The first size now belongs to the widget that was second before, and vice versa
  if (currentSizes.size() == 2) {
    _centralSplitter->setSizes({currentSizes[1], currentSizes[0]});
  }
  
  QString position = _pdfPanelOnLeft ? "left" : "right";
  statusBar()->showMessage(QString("PDF panel moved to %1").arg(position), 2000);
}
#endif
