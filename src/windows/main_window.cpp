// main_window.cpp
#include "main_window.h"
#include "../core/auto_save_manager.h"
#include "../core/layer.h"
#include "../core/recent_files_manager.h"
#include "../core/theme_manager.h"
#include "../tools/tool_manager.h"
#include "../widgets/canvas.h"
#include "../widgets/element_bank_panel.h"
#include "../widgets/layer_panel.h"
#include "../widgets/tool_panel.h"
#include <QApplication>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#ifdef HAVE_QT_PDF
#include "../widgets/page_thumbnail_panel.h"
#include "../widgets/pdf_viewer.h"
#endif

// Helper function for Qt5/Qt6 compatibility: create action with shortcut
static QAction *createAction(QMenu *menu, const QString &text,
                             const QKeySequence &shortcut, QObject *receiver,
                             const char *slot) {
  QAction *action = menu->addAction(text, receiver, slot);
  action->setShortcut(shortcut);
  return action;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)),
      _toolPanel(new ToolPanel(this)), _layerPanel(nullptr),
      _elementBankPanel(new ElementBankPanel(this)), _autoSaveManager(nullptr),
      _statusLabel(nullptr), _measurementLabel(nullptr),
      _recentFilesMenu(nullptr), _snapToGridAction(nullptr),
      _autoSaveAction(nullptr), _rulerAction(nullptr),
      _measurementAction(nullptr)
#ifdef HAVE_QT_PDF
      ,
      _pdfViewer(nullptr), _thumbnailPanel(nullptr), _centralSplitter(nullptr),
      _pdfPanel(nullptr), _pdfToolBar(nullptr), _pdfPageLabel(nullptr),
      _pdfPageSpinBox(nullptr), _pdfZoomCombo(nullptr),
      _pdfDarkModeAction(nullptr), _thumbnailToggleAction(nullptr),
      _pdfPanelOnLeft(false) // PDF panel starts on the right by default
#endif
{
#ifdef HAVE_QT_PDF
  // Split ratio constants for canvas/PDF panel
  static constexpr double PDF_PANEL_SPLIT_RATIO =
      0.4; // PDF panel takes 40% when visible

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

  this->addDockWidget(Qt::LeftDockWidgetArea, _toolPanel);
  this->addDockWidget(Qt::RightDockWidgetArea, _elementBankPanel);
  this->setWindowTitle("FullScreen Pencil Draw - Professional Edition");
  this->resize(1400, 900);

  setupMenuBar();
  setupStatusBar();
  setupLayerPanel();
  setupAutoSave();
  setupConnections();

#ifdef HAVE_QT_PDF
  setupPdfToolBar();

  // Connect thumbnail panel visibility to toggle action (must be after toolbar
  // setup)
  if (_thumbnailPanel && _thumbnailToggleAction) {
    connect(_thumbnailPanel, &PageThumbnailPanel::visibilityChanged,
            _thumbnailToggleAction, &QAction::setChecked);
  }
#endif

  _toolPanel->updateBrushSizeDisplay(_canvas->getCurrentBrushSize());
  _toolPanel->updateColorDisplay(_canvas->getCurrentColor());
  _toolPanel->updateZoomDisplay(_canvas->getCurrentZoom());
  _toolPanel->updateOpacityDisplay(_canvas->getCurrentOpacity());

  // Add global shortcuts for delete (works regardless of focus)
  QShortcut *deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
  connect(deleteShortcut, &QShortcut::activated, _canvas,
          &Canvas::deleteSelectedItems);
  QShortcut *backspaceShortcut =
      new QShortcut(QKeySequence(Qt::Key_Backspace), this);
  connect(backspaceShortcut, &QShortcut::activated, _canvas,
          &Canvas::deleteSelectedItems);
}

MainWindow::~MainWindow() {}

void MainWindow::setupStatusBar() {
  _statusLabel =
      new QLabel("✦ Ready | P:Pen E:Eraser T:Text F:Fill Q:ColorSelect "
                 "L:Line A:Arrow R:Rect C:Circle S:Select H:Pan | G:Grid "
                 "B:Filled | Ctrl+Scroll:Zoom",
                 this);
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
  connect(_toolPanel, &ToolPanel::eraserSelected, _canvas,
          &Canvas::setEraserTool);
  connect(_toolPanel, &ToolPanel::textSelected, _canvas, &Canvas::setTextTool);
  connect(_toolPanel, &ToolPanel::mermaidSelected, _canvas,
          &Canvas::setMermaidTool);
  connect(_toolPanel, &ToolPanel::fillSelected, _canvas, &Canvas::setFillTool);
  connect(_toolPanel, &ToolPanel::colorSelectSelected, _canvas,
          &Canvas::setColorSelectTool);
  connect(_toolPanel, &ToolPanel::arrowSelected, _canvas,
          &Canvas::setArrowTool);
  connect(_toolPanel, &ToolPanel::curvedArrowSelected, _canvas,
          &Canvas::setCurvedArrowTool);
  connect(_toolPanel, &ToolPanel::panSelected, _canvas, &Canvas::setPanTool);
  connect(_toolPanel, &ToolPanel::bezierSelected, _canvas,
          &Canvas::setBezierTool);
  connect(_toolPanel, &ToolPanel::textOnPathSelected, _canvas,
          &Canvas::setTextOnPathTool);
  connect(_toolPanel, &ToolPanel::colorSelected, _canvas, &Canvas::setPenColor);
  connect(_toolPanel, &ToolPanel::opacitySelected, _canvas,
          &Canvas::setOpacity);

#ifdef HAVE_QT_PDF
  // Connect tool selections and settings to PDF viewer as well
  // This allows the same toolbar to control both canvas and PDF viewer
  if (_pdfViewer) {
    // Tool selections - route to PDF viewer so tools work on both renderers
    connect(_toolPanel, &ToolPanel::penSelected, this,
            [this]() { _pdfViewer->setToolType(ToolManager::ToolType::Pen); });
    connect(_toolPanel, &ToolPanel::eraserSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Eraser);
    });
    connect(_toolPanel, &ToolPanel::textSelected, this,
            [this]() { _pdfViewer->setToolType(ToolManager::ToolType::Text); });
    connect(_toolPanel, &ToolPanel::mermaidSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Mermaid);
    });
    connect(_toolPanel, &ToolPanel::fillSelected, this,
            [this]() { _pdfViewer->setToolType(ToolManager::ToolType::Fill); });
    connect(_toolPanel, &ToolPanel::arrowSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Arrow);
    });
    connect(_toolPanel, &ToolPanel::panSelected, this,
            [this]() { _pdfViewer->setToolType(ToolManager::ToolType::Pan); });
    connect(_toolPanel, &ToolPanel::rectangleSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Rectangle);
    });
    connect(_toolPanel, &ToolPanel::circleSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Circle);
    });
    connect(_toolPanel, &ToolPanel::lineSelected, this,
            [this]() { _pdfViewer->setToolType(ToolManager::ToolType::Line); });
    connect(_toolPanel, &ToolPanel::selectionSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Selection);
    });
    connect(_toolPanel, &ToolPanel::bezierSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Bezier);
    });
    connect(_toolPanel, &ToolPanel::textOnPathSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::TextOnPath);
    });

    // Connect color selection to PDF viewer
    connect(_toolPanel, &ToolPanel::colorSelected, _pdfViewer,
            &PdfViewer::setPenColor);
    // Connect brush size changes to PDF viewer
    connect(_canvas, &Canvas::brushSizeChanged, _pdfViewer,
            &PdfViewer::setPenWidth);
    // Connect filled shapes toggle to PDF viewer
    connect(_canvas, &Canvas::filledShapesChanged, _pdfViewer,
            &PdfViewer::setFilledShapes);
  }
#endif

  // Shape tools
  connect(_toolPanel, &ToolPanel::rectangleSelected, _canvas,
          [this]() { _canvas->setShape("Rectangle"); });
  connect(_toolPanel, &ToolPanel::circleSelected, _canvas,
          [this]() { _canvas->setShape("Circle"); });
  connect(_toolPanel, &ToolPanel::lineSelected, _canvas,
          [this]() { _canvas->setShape("Line"); });
  connect(_toolPanel, &ToolPanel::selectionSelected, _canvas,
          [this]() { _canvas->setShape("Selection"); });

  // Edit operations
  connect(_toolPanel, &ToolPanel::copyAction, _canvas,
          &Canvas::copySelectedItems);
  connect(_toolPanel, &ToolPanel::cutAction, _canvas,
          &Canvas::cutSelectedItems);
  connect(_toolPanel, &ToolPanel::pasteAction, _canvas, &Canvas::pasteItems);
  connect(_toolPanel, &ToolPanel::duplicateAction, _canvas,
          &Canvas::duplicateSelectedItems);
  connect(_toolPanel, &ToolPanel::deleteAction, _canvas,
          &Canvas::deleteSelectedItems);
  connect(_toolPanel, &ToolPanel::selectAllAction, _canvas, &Canvas::selectAll);

  // Brush controls
  connect(_toolPanel, &ToolPanel::increaseBrushSize, _canvas,
          &Canvas::increaseBrushSize);
  connect(_toolPanel, &ToolPanel::decreaseBrushSize, _canvas,
          &Canvas::decreaseBrushSize);

  // Canvas feedback to ToolPanel
  connect(_canvas, &Canvas::brushSizeChanged, this,
          &MainWindow::onBrushSizeChanged);
  connect(_canvas, &Canvas::colorChanged, this, &MainWindow::onColorChanged);
  connect(_canvas, &Canvas::zoomChanged, this, &MainWindow::onZoomChanged);
  connect(_canvas, &Canvas::opacityChanged, this,
          &MainWindow::onOpacityChanged);
  connect(_canvas, &Canvas::cursorPositionChanged, this,
          &MainWindow::onCursorPositionChanged);

  // Undo/Redo
  connect(_toolPanel, &ToolPanel::undoAction, _canvas, &Canvas::undoLastAction);
  connect(_toolPanel, &ToolPanel::redoAction, _canvas, &Canvas::redoLastAction);

  // Zoom
  connect(_toolPanel, &ToolPanel::zoomInAction, _canvas, &Canvas::zoomIn);
  connect(_toolPanel, &ToolPanel::zoomOutAction, _canvas, &Canvas::zoomOut);
  connect(_toolPanel, &ToolPanel::zoomResetAction, _canvas, &Canvas::zoomReset);
  connect(_toolPanel, &ToolPanel::toggleGridAction, _canvas,
          &Canvas::toggleGrid);
  connect(_toolPanel, &ToolPanel::toggleFilledShapesAction, _canvas,
          &Canvas::toggleFilledShapes);

  // Pressure sensitivity
  connect(_toolPanel, &ToolPanel::pressureSensitivityToggled, _canvas,
          &Canvas::togglePressureSensitivity);

  // Element bank
  connect(_elementBankPanel, &ElementBankPanel::elementSelected, _canvas,
          &Canvas::placeElement);

  // Filled shapes feedback
  connect(_canvas, &Canvas::filledShapesChanged, this,
          &MainWindow::onFilledShapesChanged);

  // Snap to grid feedback
  connect(_canvas, &Canvas::snapToGridChanged, this,
          &MainWindow::onSnapToGridChanged);

  // Ruler and measurement feedback
  connect(_canvas, &Canvas::rulerVisibilityChanged, this,
          &MainWindow::onRulerVisibilityChanged);
  connect(_canvas, &Canvas::measurementToolChanged, this,
          &MainWindow::onMeasurementToolChanged);
  connect(_canvas, &Canvas::measurementUpdated, this,
          &MainWindow::onMeasurementUpdated);

  // File operations
  connect(_toolPanel, &ToolPanel::saveAction, _canvas, &Canvas::saveToFile);
  connect(_toolPanel, &ToolPanel::openAction, _canvas, &Canvas::openFile);
  connect(_toolPanel, &ToolPanel::newCanvasAction, this,
          &MainWindow::onNewCanvas);
  connect(_toolPanel, &ToolPanel::clearCanvas, _canvas, &Canvas::clearCanvas);

  // Recent files
  connect(&RecentFilesManager::instance(),
          &RecentFilesManager::recentFilesChanged, this,
          &MainWindow::onRecentFilesChanged);
}

void MainWindow::setupMenuBar() {
  QMenuBar *menuBar = this->menuBar();

  // File menu
  QMenu *fileMenu = menuBar->addMenu("&File");

  createAction(fileMenu, "&New", QKeySequence::New, this, SLOT(onNewCanvas()));
  createAction(fileMenu, "&Open...", QKeySequence::Open, _canvas,
               SLOT(openFile()));
  fileMenu->addAction("Open &Project...", _canvas, SLOT(openProject()));

#ifdef HAVE_QT_PDF
  createAction(fileMenu, "Open P&DF...",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), this,
               SLOT(onOpenPdf()));
#endif

  // Recent Files submenu
  _recentFilesMenu = fileMenu->addMenu("Recent Files");
  updateRecentFilesMenu();

  fileMenu->addSeparator();

  createAction(fileMenu, "&Save...", QKeySequence::Save, _canvas,
               SLOT(saveToFile()));
  fileMenu->addAction("Save Pro&ject...", _canvas, SLOT(saveProject()));
  fileMenu->addAction("Export to &PDF...", _canvas, SLOT(exportToPDF()));

#ifdef HAVE_QT_PDF
  fileMenu->addAction("Export &Annotated PDF...", this,
                      SLOT(onExportAnnotatedPdf()));
  fileMenu->addAction("&Close PDF", this, SLOT(onClosePdf()));
#endif

  fileMenu->addSeparator();

  createAction(fileMenu, "E&xit", QKeySequence::Quit, this, SLOT(close()));

  // Edit menu
  QMenu *editMenu = menuBar->addMenu("&Edit");

  createAction(editMenu, "&Undo", QKeySequence::Undo, _canvas,
               SLOT(undoLastAction()));
  createAction(editMenu, "&Redo", QKeySequence::Redo, _canvas,
               SLOT(redoLastAction()));
  editMenu->addSeparator();
  createAction(editMenu, "Cu&t", QKeySequence::Cut, _canvas,
               SLOT(cutSelectedItems()));
  createAction(editMenu, "&Copy", QKeySequence::Copy, _canvas,
               SLOT(copySelectedItems()));
  createAction(editMenu, "&Paste", QKeySequence::Paste, _canvas,
               SLOT(pasteItems()));
  editMenu->addSeparator();
  createAction(editMenu, "Select &All", QKeySequence::SelectAll, _canvas,
               SLOT(selectAll()));
  createAction(editMenu, "&Delete", QKeySequence::Delete, _canvas,
               SLOT(deleteSelectedItems()));
  createAction(editMenu, "D&uplicate", QKeySequence(Qt::CTRL | Qt::Key_D),
               _canvas, SLOT(duplicateSelectedItems()));
  createAction(editMenu, "Extract Color Selection to New Layer",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_J), _canvas,
               SLOT(extractColorSelectionToNewLayer()));

  // View menu
  QMenu *viewMenu = menuBar->addMenu("&View");

  createAction(viewMenu, "Zoom &In", QKeySequence::ZoomIn, _canvas,
               SLOT(zoomIn()));
  createAction(viewMenu, "Zoom &Out", QKeySequence::ZoomOut, _canvas,
               SLOT(zoomOut()));
  createAction(viewMenu, "&Reset Zoom", QKeySequence(Qt::Key_0), _canvas,
               SLOT(zoomReset()));
  viewMenu->addSeparator();

  QAction *gridAction =
      createAction(viewMenu, "Toggle &Grid", QKeySequence(Qt::Key_G), _canvas,
                   SLOT(toggleGrid()));
  gridAction->setCheckable(true);
  gridAction->setChecked(_canvas->isGridVisible());

  _snapToGridAction = createAction(viewMenu, "&Snap to Grid",
                                   QKeySequence(Qt::CTRL | Qt::Key_G), _canvas,
                                   SLOT(toggleSnapToGrid()));
  _snapToGridAction->setCheckable(true);
  _snapToGridAction->setChecked(_canvas->isSnapToGridEnabled());

  QAction *filledAction =
      createAction(viewMenu, "Toggle &Filled Shapes", QKeySequence(Qt::Key_B),
                   _canvas, SLOT(toggleFilledShapes()));
  filledAction->setCheckable(true);
  filledAction->setChecked(_canvas->isFilledShapes());

  viewMenu->addSeparator();

  _rulerAction =
      createAction(viewMenu, "Show &Ruler", QKeySequence(Qt::CTRL | Qt::Key_R),
                   _canvas, SLOT(toggleRuler()));
  _rulerAction->setCheckable(true);
  _rulerAction->setChecked(_canvas->isRulerVisible());

  _measurementAction =
      createAction(viewMenu, "&Measurement Tool", QKeySequence(Qt::Key_M),
                   _canvas, SLOT(toggleMeasurementTool()));
  _measurementAction->setCheckable(true);
  _measurementAction->setChecked(_canvas->isMeasurementToolEnabled());

  viewMenu->addSeparator();

#ifdef HAVE_QT_PDF
  // Panel order option
  createAction(viewMenu, "S&wap Panel Order",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W), this,
               SLOT(swapPanelOrder()));
  viewMenu->addSeparator();
#endif

  // Theme submenu
  QMenu *themeMenu = viewMenu->addMenu("&Theme");
  QAction *toggleThemeAction =
      createAction(themeMenu, "Toggle &Dark/Light Theme",
                   QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), this,
                   SLOT(onToggleTheme()));

  // Panels
  viewMenu->addSeparator();
  viewMenu->addAction(_elementBankPanel->toggleViewAction());

  // Edit menu - add lock/unlock after other edit items
  editMenu->addSeparator();
  createAction(editMenu, "&Lock Selected", QKeySequence(Qt::CTRL | Qt::Key_L),
               _canvas, SLOT(lockSelectedItems()));
  createAction(editMenu, "&Unlock All",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L), _canvas,
               SLOT(unlockSelectedItems()));

  // Edit menu - add group/ungroup
  editMenu->addSeparator();
  createAction(editMenu, "&Group", QKeySequence(Qt::CTRL | Qt::Key_G), _canvas,
               SLOT(groupSelectedItems()));
  createAction(editMenu, "U&ngroup",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_U), _canvas,
               SLOT(ungroupSelectedItems()));

  // Edit menu - scaling
  editMenu->addSeparator();
  createAction(editMenu, "Scale &Selected Items...",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), _canvas,
               SLOT(scaleSelectedItems()));
  createAction(editMenu, "Scale Active &Layer...",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), _canvas,
               SLOT(scaleActiveLayer()));

  // Edit menu - perspective transform
  createAction(editMenu, "&Perspective Transform...",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), _canvas,
               SLOT(perspectiveTransformSelectedItems()));

  // Edit menu - canvas resize
  editMenu->addSeparator();
  editMenu->addAction("Resize Canvas...", _canvas, SLOT(resizeCanvas()));

  // Tools menu
  QMenu *toolsMenu = menuBar->addMenu("&Tools");

  _autoSaveAction = toolsMenu->addAction("Enable &Auto-Save");
  connect(_autoSaveAction, &QAction::triggered, this, [this]() {
    if (_autoSaveManager) {
      _autoSaveManager->setEnabled(!_autoSaveManager->isEnabled());
      _autoSaveAction->setChecked(_autoSaveManager->isEnabled());
    }
  });
  _autoSaveAction->setCheckable(true);
  _autoSaveAction->setChecked(true); // Enabled by default

  toolsMenu->addSeparator();

  QAction *colorToleranceAction =
      toolsMenu->addAction("Color Select &Tolerance...");
  connect(colorToleranceAction, &QAction::triggered, _canvas,
          &Canvas::setColorSelectTolerance);

  QAction *contiguousColorSelectAction =
      toolsMenu->addAction("Color Select &Contiguous");
  contiguousColorSelectAction->setCheckable(true);
  contiguousColorSelectAction->setChecked(_canvas->isColorSelectContiguous());
  connect(contiguousColorSelectAction, &QAction::triggered, this,
          [this, contiguousColorSelectAction]() {
            _canvas->toggleColorSelectContiguous();
            contiguousColorSelectAction->setChecked(
                _canvas->isColorSelectContiguous());
          });

  QAction *clearColorSelectionAction =
      toolsMenu->addAction("Clear Color Selection");
  connect(clearColorSelectionAction, &QAction::triggered, _canvas,
          &Canvas::clearColorSelection);

  // Filters menu
  QMenu *filtersMenu = menuBar->addMenu("F&ilters");
  filtersMenu->addAction("&Blur", _canvas, SLOT(applyBlurToSelection()));
  filtersMenu->addAction("&Sharpen", _canvas, SLOT(applySharpenToSelection()));

  // Help menu
  QMenu *helpMenu = menuBar->addMenu("&Help");

  helpMenu->addAction("&About", this, [this]() {
    QMessageBox::about(
        this, "About FullScreen Pencil Draw",
        QString("<h3>FullScreen Pencil Draw</h3>"
                "<p>Version 0.1</p>"
                "<p>A vector and raster graphics editor.</p>"
                "<p>Copyright © 2020 Adam Djellouli</p>"
                "<p>Licensed under the MIT License.</p>"
                "<hr>"
                "<p><b>Third-Party Libraries:</b></p>"
                "<p>This application uses the <a href='https://www.qt.io/'>Qt "
                "framework</a> "
                "version %1 under the "
                "<a href='https://www.gnu.org/licenses/lgpl-3.0.html'>GNU LGPL "
                "v3</a>.</p>"
                "<p>Qt source code is available at "
                "<a href='https://code.qt.io/'>code.qt.io</a>.</p>")
            .arg(qVersion()));
  });

  helpMenu->addAction("About &Qt", qApp, &QApplication::aboutQt);
}

void MainWindow::updateRecentFilesMenu() {
  if (!_recentFilesMenu)
    return;

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

void MainWindow::onRecentFilesChanged() { updateRecentFilesMenu(); }

void MainWindow::openRecentFile() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString filePath = action->data().toString();
    _canvas->openRecentFile(filePath);
  }
}

void MainWindow::onToggleTheme() { ThemeManager::instance().toggleTheme(); }

void MainWindow::setupLayerPanel() {
  if (_canvas && _canvas->layerManager()) {
    _layerPanel = new LayerPanel(_canvas->layerManager(), this);
    _layerPanel->setCanvas(_canvas);
    _layerPanel->setItemStore(_canvas->itemStore());
    addDockWidget(Qt::RightDockWidgetArea, _layerPanel);

    // Add panel visibility actions to View menu now that both panels exist
    QMenuBar *menuBar = this->menuBar();
    QMenu *viewMenu = nullptr;
    for (QAction *action : menuBar->actions()) {
      if (action->text().contains("View")) {
        viewMenu = action->menu();
        break;
      }
    }
    if (viewMenu) {
      viewMenu->addSeparator();

      QAction *showToolsAction = _toolPanel->toggleViewAction();
      showToolsAction->setText("Show &Tools Panel");
      showToolsAction->setShortcut(QKeySequence(Qt::Key_F5));
      viewMenu->addAction(showToolsAction);

      QAction *showLayersAction = _layerPanel->toggleViewAction();
      showLayersAction->setText("Show &Layers Panel");
      showLayersAction->setShortcut(QKeySequence(Qt::Key_F6));
      viewMenu->addAction(showLayersAction);
    }
  }
}

void MainWindow::onBrushSizeChanged(int size) {
  _toolPanel->updateBrushSizeDisplay(size);
}
void MainWindow::onColorChanged(const QColor &color) {
  _toolPanel->updateColorDisplay(color);
}
void MainWindow::onZoomChanged(double zoom) {
  _toolPanel->updateZoomDisplay(zoom);
}
void MainWindow::onOpacityChanged(int opacity) {
  _toolPanel->updateOpacityDisplay(opacity);
}
void MainWindow::onFilledShapesChanged(bool filled) {
  _toolPanel->updateFilledShapesDisplay(filled);
}
void MainWindow::onCursorPositionChanged(const QPointF &pos) {
  _toolPanel->updatePositionDisplay(pos);
}

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

  connect(_autoSaveManager, &AutoSaveManager::autoSavePerformed, this,
          &MainWindow::onAutoSavePerformed);

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
  int width = QInputDialog::getInt(this, "New Canvas", "Width:", 1920, 100,
                                   10000, 100, &ok);
  if (!ok)
    return;
  int height = QInputDialog::getInt(this, "New Canvas", "Height:", 1080, 100,
                                    10000, 100, &ok);
  if (!ok)
    return;
  QColor bgColor = QColorDialog::getColor(Qt::black, this, "Background Color");
  if (!bgColor.isValid())
    return;
  _canvas->newCanvas(width, height, bgColor);
  // Refresh layer panel after new canvas
  if (_layerPanel) {
    _layerPanel->refreshLayerList();
  }
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
  } else if (event->matches(QKeySequence::Open)) {
    _canvas->openFile();
  } else if (event->matches(QKeySequence::New)) {
    onNewCanvas();
  } else if (event->matches(QKeySequence::SelectAll)) {
    _canvas->selectAll();
  } else if (event->key() == Qt::Key_J &&
             (event->modifiers() & Qt::ControlModifier) &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _canvas->extractColorSelectionToNewLayer();
  } else if (event->key() == Qt::Key_Escape) {
    this->close();
  } else if (event->key() == Qt::Key_Delete ||
             event->key() == Qt::Key_Backspace) {
    _canvas->deleteSelectedItems();
  }
  // Group/Ungroup shortcuts
  else if (event->key() == Qt::Key_G &&
           (event->modifiers() & Qt::ControlModifier)) {
    _canvas->groupSelectedItems();
  } else if (event->key() == Qt::Key_U &&
             (event->modifiers() & Qt::ControlModifier) &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _canvas->ungroupSelectedItems();
  }
  // Tool shortcuts
  else if (event->key() == Qt::Key_P) {
    _toolPanel->onActionPen();
  } else if (event->key() == Qt::Key_E) {
    _toolPanel->onActionEraser();
  } else if (event->key() == Qt::Key_T &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _toolPanel->onActionTextOnPath();
  } else if (event->key() == Qt::Key_T) {
    _toolPanel->onActionText();
  } else if (event->key() == Qt::Key_M) {
    _toolPanel->onActionMermaid();
  } else if (event->key() == Qt::Key_F) {
    _toolPanel->onActionFill();
  } else if (event->key() == Qt::Key_Q &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionColorSelect();
  } else if (event->key() == Qt::Key_L) {
    _toolPanel->onActionLine();
  } else if (event->key() == Qt::Key_A &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionArrow();
  } else if (event->key() == Qt::Key_R) {
    _toolPanel->onActionRectangle();
  } else if (event->key() == Qt::Key_C &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionCircle();
  } else if (event->key() == Qt::Key_S &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionSelection();
  } else if (event->key() == Qt::Key_H) {
    _toolPanel->onActionPan();
  } else if (event->key() == Qt::Key_G) {
    _canvas->toggleGrid();
  } else if (event->key() == Qt::Key_B &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _toolPanel->onActionBezier();
  } else if (event->key() == Qt::Key_B) {
    _canvas->toggleFilledShapes();
  } else if (event->key() == Qt::Key_D &&
             (event->modifiers() & Qt::ControlModifier)) {
    _canvas->duplicateSelectedItems();
  }
#ifdef HAVE_QT_PDF
  // PDF navigation shortcuts (only work when PDF panel is visible)
  else if (event->key() == Qt::Key_PageDown && _pdfPanel &&
           _pdfPanel->isVisible() && _pdfViewer) {
    _pdfViewer->nextPage();
  } else if (event->key() == Qt::Key_PageUp && _pdfPanel &&
             _pdfPanel->isVisible() && _pdfViewer) {
    _pdfViewer->previousPage();
  } else if (event->key() == Qt::Key_Home && _pdfPanel &&
             _pdfPanel->isVisible() && _pdfViewer) {
    _pdfViewer->firstPage();
  } else if (event->key() == Qt::Key_End && _pdfPanel &&
             _pdfPanel->isVisible() && _pdfViewer) {
    _pdfViewer->lastPage();
  }
#endif
  // Brush size
  else if (event->key() == Qt::Key_BracketRight) {
    _canvas->increaseBrushSize();
  } else if (event->key() == Qt::Key_BracketLeft) {
    _canvas->decreaseBrushSize();
  }
  // Zoom
  else if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
    _canvas->zoomIn();
  } else if (event->key() == Qt::Key_Minus) {
    _canvas->zoomOut();
  } else if (event->key() == Qt::Key_0) {
    _canvas->zoomReset();
  } else {
    QMainWindow::keyPressEvent(event);
  }
}

#ifdef HAVE_QT_PDF
void MainWindow::setupPdfViewer() {
  // Create PDF panel with its own layout (includes thumbnail rail + viewer)
  _pdfPanel = new QFrame(this);
  _pdfPanel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  _pdfPanel->setMinimumWidth(200);

  QHBoxLayout *pdfHLayout = new QHBoxLayout(_pdfPanel);
  pdfHLayout->setContentsMargins(0, 0, 0, 0);
  pdfHLayout->setSpacing(0);

  // Create PDF viewer widget first (needed for thumbnail panel)
  _pdfViewer = new PdfViewer(_pdfPanel);

  // Create thumbnail panel (collapsible, on the left of PDF viewer)
  _thumbnailPanel = new PageThumbnailPanel(_pdfViewer, _pdfPanel);
  _thumbnailPanel->hide(); // Initially hidden, shown when PDF loads
  pdfHLayout->addWidget(_thumbnailPanel);

  // Create viewer container with header
  QWidget *viewerContainer = new QWidget(_pdfPanel);
  QVBoxLayout *viewerLayout = new QVBoxLayout(viewerContainer);
  viewerLayout->setContentsMargins(0, 0, 0, 0);
  viewerLayout->setSpacing(0);

  // Create header label for PDF panel
  QLabel *pdfHeader = new QLabel("PDF Viewer", viewerContainer);
  pdfHeader->setAlignment(Qt::AlignCenter);
  pdfHeader->setStyleSheet("QLabel { background-color: #2a2a30; color: white; "
                           "padding: 8px; font-weight: bold; }");
  viewerLayout->addWidget(pdfHeader);

  viewerLayout->addWidget(_pdfViewer, 1);
  pdfHLayout->addWidget(viewerContainer, 1);

  // Add PDF panel to splitter (initially hidden)
  _centralSplitter->addWidget(_pdfPanel);
  _pdfPanel->hide();

  // Connect thumbnail panel page selection
  connect(_thumbnailPanel, &PageThumbnailPanel::pageSelected, this,
          &MainWindow::onThumbnailPageSelected);

  // Connect PDF viewer signals
  connect(_pdfViewer, &PdfViewer::pageChanged, this,
          &MainWindow::onPdfPageChanged);
  connect(_pdfViewer, &PdfViewer::zoomChanged, this,
          &MainWindow::onPdfZoomChanged);
  connect(_pdfViewer, &PdfViewer::darkModeChanged, this,
          &MainWindow::onPdfDarkModeChanged);
  connect(_pdfViewer, &PdfViewer::cursorPositionChanged, this,
          &MainWindow::onCursorPositionChanged);
  connect(_pdfViewer, &PdfViewer::pdfLoaded, this,
          [this]() { showPdfPanel(); });
  connect(_pdfViewer, &PdfViewer::pdfClosed, this,
          [this]() { hidePdfPanel(); });
  connect(_pdfViewer, &PdfViewer::errorOccurred, this,
          [this](const QString &message) {
            statusBar()->showMessage(QString("PDF Error: %1").arg(message),
                                     5000);
          });

  // Connect screenshot signal to add captured image to main canvas
  connect(_pdfViewer, &PdfViewer::screenshotCaptured, _canvas,
          &Canvas::addImageFromScreenshot);
  connect(_pdfViewer, &PdfViewer::screenshotCaptured, this, [this]() {
    statusBar()->showMessage("Screenshot captured and added to canvas", 3000);
  });

  // Connect drag-drop signals from PDF viewer and canvas
  connect(_pdfViewer, &PdfViewer::pdfFileDropped, this,
          &MainWindow::onPdfFileDropped);
  connect(_canvas, &Canvas::pdfFileDropped, this,
          &MainWindow::onPdfFileDropped);
}

void MainWindow::setupPdfToolBar() {
  _pdfToolBar = new QToolBar("PDF Navigation", this);
  _pdfToolBar->setObjectName("pdfToolBar");
  _pdfToolBar->setIconSize(QSize(24, 24));
  _pdfToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  addToolBar(Qt::TopToolBarArea, _pdfToolBar);
  _pdfToolBar->hide(); // Hidden until PDF is loaded

  // === CLUSTER 1: Document Operations ===
  // Thumbnail panel toggle
  _thumbnailToggleAction = _pdfToolBar->addAction("☰", [this]() {
    if (_thumbnailPanel) {
      _thumbnailPanel->toggleVisibility();
    }
  });
  _thumbnailToggleAction->setToolTip("Toggle Page Thumbnails");
  _thumbnailToggleAction->setCheckable(true);
  _thumbnailToggleAction->setChecked(true);

  _pdfToolBar->addSeparator();

  QAction *openPdfAction =
      _pdfToolBar->addAction("📂", this, &MainWindow::onOpenPdf);
  openPdfAction->setToolTip("Open PDF (Ctrl+Shift+O)");

  QAction *exportAction =
      _pdfToolBar->addAction("💾", this, &MainWindow::onExportAnnotatedPdf);
  exportAction->setToolTip("Export Annotated PDF");

  QAction *closeAction =
      _pdfToolBar->addAction("✕", this, &MainWindow::onClosePdf);
  closeAction->setToolTip("Close PDF");

  _pdfToolBar->addSeparator();

  // === CLUSTER 2: Navigation ===
  QAction *firstAction =
      _pdfToolBar->addAction("⏮", _pdfViewer, &PdfViewer::firstPage);
  firstAction->setToolTip("First Page (Home)");

  QAction *prevAction =
      _pdfToolBar->addAction("◀", _pdfViewer, &PdfViewer::previousPage);
  prevAction->setToolTip("Previous Page (Page Up)");

  // Editable page number spinbox
  _pdfPageSpinBox = new QSpinBox(this);
  _pdfPageSpinBox->setMinimum(1);
  _pdfPageSpinBox->setMaximum(1);
  _pdfPageSpinBox->setValue(1);
  _pdfPageSpinBox->setToolTip("Go to page (type number and press Enter)");
  _pdfPageSpinBox->setFixedWidth(60);
  _pdfPageSpinBox->setAlignment(Qt::AlignCenter);
  _pdfPageSpinBox->setStyleSheet(R"(
    QSpinBox {
      background-color: #2a2a30;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.1);
      border-radius: 4px;
      padding: 4px 8px;
      min-height: 28px;
    }
    QSpinBox::up-button, QSpinBox::down-button {
      width: 0px;
    }
  )");
  connect(_pdfPageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &MainWindow::onPdfPageSpinBoxChanged);
  _pdfToolBar->addWidget(_pdfPageSpinBox);

  // Page count label
  _pdfPageLabel = new QLabel(" / 0", this);
  _pdfPageLabel->setStyleSheet("QLabel { color: #a0a0a8; padding: 0 8px; }");
  _pdfToolBar->addWidget(_pdfPageLabel);

  QAction *nextAction =
      _pdfToolBar->addAction("▶", _pdfViewer, &PdfViewer::nextPage);
  nextAction->setToolTip("Next Page (Page Down)");

  QAction *lastAction =
      _pdfToolBar->addAction("⏭", _pdfViewer, &PdfViewer::lastPage);
  lastAction->setToolTip("Last Page (End)");

  _pdfToolBar->addSeparator();

  // === CLUSTER 3: View/Edit Controls ===
  // Zoom controls with dropdown
  QAction *zoomOutAction =
      _pdfToolBar->addAction("−", _pdfViewer, &PdfViewer::zoomOut);
  zoomOutAction->setToolTip("Zoom Out (-)");

  _pdfZoomCombo = new QComboBox(this);
  _pdfZoomCombo->addItems(
      {"50%", "75%", "100%", "125%", "150%", "200%", "300%"});
  _pdfZoomCombo->setCurrentIndex(2); // 100%
  _pdfZoomCombo->setEditable(true);
  _pdfZoomCombo->setFixedWidth(75);
  _pdfZoomCombo->setToolTip("Zoom level (select or type)");
  _pdfZoomCombo->setStyleSheet(R"(
    QComboBox {
      background-color: #2a2a30;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.1);
      border-radius: 4px;
      padding: 4px 8px;
      min-height: 28px;
    }
    QComboBox::drop-down {
      border: none;
      width: 20px;
    }
    QComboBox::down-arrow {
      image: none;
      border-left: 4px solid transparent;
      border-right: 4px solid transparent;
      border-top: 5px solid #a0a0a8;
      margin-right: 5px;
    }
    QComboBox QAbstractItemView {
      background-color: #2a2a30;
      color: #f8f8fc;
      selection-background-color: #3b82f6;
    }
  )");
  connect(_pdfZoomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onPdfZoomComboChanged);
  connect(_pdfZoomCombo->lineEdit(), &QLineEdit::returnPressed, this, [this]() {
    QString text = _pdfZoomCombo->currentText().remove('%');
    bool ok;
    int zoom = text.toInt(&ok);
    if (ok && zoom >= 10 && zoom <= 500 && _pdfViewer) {
      double currentZoom = _pdfViewer->zoomLevel();
      double factor = zoom / currentZoom;
      // Apply zoom by scaling
      _pdfViewer->resetTransform();
      _pdfViewer->scale(zoom / 100.0, zoom / 100.0);
    }
  });
  _pdfToolBar->addWidget(_pdfZoomCombo);

  QAction *zoomInAction =
      _pdfToolBar->addAction("+", _pdfViewer, &PdfViewer::zoomIn);
  zoomInAction->setToolTip("Zoom In (+)");

  _pdfToolBar->addSeparator();

  // Fit options
  QAction *fitWidthAction = _pdfToolBar->addAction("↔", this, [this]() {
    if (_pdfViewer && _pdfViewer->hasPdf()) {
      _pdfViewer->fitToWidth();
    }
  });
  fitWidthAction->setToolTip("Fit to Width");

  QAction *fitPageAction = _pdfToolBar->addAction("⬜", this, [this]() {
    if (_pdfViewer && _pdfViewer->hasPdf()) {
      _pdfViewer->fitToPage();
    }
  });
  fitPageAction->setToolTip("Fit to Page");

  _pdfToolBar->addSeparator();

  // Rotation
  QAction *rotateLeftAction = _pdfToolBar->addAction("↺", this, [this]() {
    if (_pdfViewer && _pdfViewer->hasPdf()) {
      _pdfViewer->rotatePageLeft();
    }
  });
  rotateLeftAction->setToolTip("Rotate Left (90° CCW)");

  QAction *rotateRightAction = _pdfToolBar->addAction("↻", this, [this]() {
    if (_pdfViewer && _pdfViewer->hasPdf()) {
      _pdfViewer->rotatePageRight();
    }
  });
  rotateRightAction->setToolTip("Rotate Right (90° CW)");

  _pdfToolBar->addSeparator();

  // Dark mode toggle
  _pdfDarkModeAction = _pdfToolBar->addAction(
      "🌙", [this]() { _pdfViewer->setDarkMode(!_pdfViewer->darkMode()); });
  _pdfDarkModeAction->setToolTip("Toggle Dark Mode");
  _pdfDarkModeAction->setCheckable(true);

  _pdfToolBar->addSeparator();

  // Mode toggle: View vs Annotate
  QAction *modeAction = _pdfToolBar->addAction("✏️ Annotate", [this]() {
    if (_pdfViewer) {
      if (_pdfViewer->isAnnotateMode()) {
        _pdfViewer->setMode(PdfViewer::Mode::View);
      } else {
        _pdfViewer->setMode(PdfViewer::Mode::Annotate);
      }
    }
  });
  modeAction->setToolTip("Toggle View/Annotate mode");
  modeAction->setCheckable(true);
  modeAction->setChecked(true); // Start in Annotate mode

  // Connect mode change to update button
  connect(_pdfViewer, &PdfViewer::modeChanged, this,
          [modeAction](PdfViewer::Mode mode) {
            if (mode == PdfViewer::Mode::Annotate) {
              modeAction->setText("✏️ Annotate");
              modeAction->setChecked(true);
            } else {
              modeAction->setText("👁 View");
              modeAction->setChecked(false);
            }
          });

  _pdfToolBar->addSeparator();

  // Screenshot tool
  QAction *screenshotAction = _pdfToolBar->addAction(
      "📷", [this]() { _pdfViewer->setScreenshotSelectionMode(true); });
  screenshotAction->setToolTip("Screenshot Selection");

  _pdfToolBar->addSeparator();

  // Undo/Redo
  QAction *undoAction =
      _pdfToolBar->addAction("↶", _pdfViewer, &PdfViewer::undo);
  undoAction->setToolTip("Undo (Ctrl+Z)");

  QAction *redoAction =
      _pdfToolBar->addAction("↷", _pdfViewer, &PdfViewer::redo);
  redoAction->setToolTip("Redo (Ctrl+Y)");

  // Style the toolbar
  _pdfToolBar->setStyleSheet(R"(
    QToolBar {
      background-color: #1e1e22;
      border-bottom: 1px solid rgba(255, 255, 255, 0.06);
      padding: 4px 8px;
      spacing: 4px;
    }
    QToolButton {
      background-color: transparent;
      color: #e0e0e6;
      border: none;
      border-radius: 6px;
      padding: 8px 12px;
      min-width: 32px;
      min-height: 32px;
      font-size: 16px;
    }
    QToolButton:hover {
      background-color: rgba(255, 255, 255, 0.08);
    }
    QToolButton:pressed {
      background-color: rgba(255, 255, 255, 0.12);
    }
    QToolButton:checked {
      background-color: #3b82f6;
      color: white;
    }
    QToolBar::separator {
      background-color: rgba(255, 255, 255, 0.1);
      width: 1px;
      margin: 6px 8px;
    }
  )");
}

void MainWindow::onPdfPageSpinBoxChanged(int page) {
  if (_pdfViewer && _pdfViewer->hasPdf()) {
    _pdfViewer->goToPage(page - 1); // Convert 1-based to 0-based
  }
}

void MainWindow::onPdfZoomComboChanged(int index) {
  static const int zoomLevels[] = {50, 75, 100, 125, 150, 200, 300};
  if (index >= 0 && index < 7 && _pdfViewer) {
    int targetZoom = zoomLevels[index];
    _pdfViewer->zoomReset();
    if (targetZoom != 100) {
      _pdfViewer->scale(targetZoom / 100.0, targetZoom / 100.0);
    }
  }
}

void MainWindow::updatePdfZoomCombo(double zoomPercent) {
  if (_pdfZoomCombo) {
    _pdfZoomCombo->blockSignals(true);
    _pdfZoomCombo->setCurrentText(QString("%1%").arg(qRound(zoomPercent)));
    _pdfZoomCombo->blockSignals(false);
  }
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
  if (_pdfPageSpinBox) {
    _pdfPageSpinBox->blockSignals(true);
    _pdfPageSpinBox->setMaximum(pageCount);
    _pdfPageSpinBox->setValue(pageIndex + 1);
    _pdfPageSpinBox->blockSignals(false);
  }
  if (_pdfPageLabel) {
    _pdfPageLabel->setText(QString(" / %1").arg(pageCount));
  }
}

void MainWindow::onPdfZoomChanged(double zoom) {
  if (_toolPanel) {
    _toolPanel->updateZoomDisplay(zoom);
  }
  updatePdfZoomCombo(zoom);
}

void MainWindow::onPdfDarkModeChanged(bool enabled) {
  if (_pdfDarkModeAction) {
    _pdfDarkModeAction->setChecked(enabled);
  }
}

void MainWindow::onPdfFileDropped(const QString &filePath) {
  if (_pdfViewer && _pdfViewer->openPdf(filePath)) {
    statusBar()->showMessage(
        QString("Opened PDF: %1").arg(QFileInfo(filePath).fileName()), 3000);
  } else {
    statusBar()->showMessage("Failed to open PDF file", 3000);
  }
}

void MainWindow::onExportAnnotatedPdf() {
  if (!_pdfViewer || !_pdfViewer->hasPdf()) {
    statusBar()->showMessage("No PDF loaded to export", 3000);
    return;
  }

  QString fileName = QFileDialog::getSaveFileName(this, "Export Annotated PDF",
                                                  "", "PDF Files (*.pdf)");
  if (fileName.isEmpty()) {
    return;
  }

  if (_pdfViewer->exportAnnotatedPdf(fileName)) {
    statusBar()->showMessage(
        QString("Exported annotated PDF to: %1").arg(fileName), 3000);
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
  // QSplitter::insertWidget handles this internally, but explicit removal is
  // clearer
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
  // The first size now belongs to the widget that was second before, and vice
  // versa
  if (currentSizes.size() == 2) {
    _centralSplitter->setSizes({currentSizes[1], currentSizes[0]});
  }

  QString position = _pdfPanelOnLeft ? "left" : "right";
  statusBar()->showMessage(QString("PDF panel moved to %1").arg(position),
                           2000);
}

void MainWindow::onThumbnailPageSelected(int pageIndex) {
  if (_pdfViewer && _pdfViewer->hasPdf()) {
    _pdfViewer->goToPage(pageIndex);
  }
}
#endif
