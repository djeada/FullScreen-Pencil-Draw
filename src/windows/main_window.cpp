// main_window.cpp
#include "main_window.h"
#include "../core/app_constants.h"
#include "../core/auto_save_manager.h"
#include "../core/file_dialogs.h"
#include "../core/layer.h"
#include "../core/recent_files_manager.h"
#include "../core/theme_manager.h"
#include "../core/undo_redo_manager.h"
#include "../tools/tool_manager.h"
#include "../widgets/canvas.h"
#include "../widgets/dock_title_bar.h"
#include "../widgets/element_bank_panel.h"
#include "../widgets/layer_panel.h"
#include "../widgets/side_tab_bar.h"
#include "../widgets/tool_panel.h"
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
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

static void installDockTitleBar(QDockWidget *dock) {
  if (!dock) {
    return;
  }
  dock->setTitleBarWidget(new DockTitleBar(dock, dock));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)),
      _toolPanel(new ToolPanel(this)), _layerPanel(nullptr),
      _elementBankPanel(new ElementBankPanel(this)), _autoSaveManager(nullptr),
      _statusLabel(nullptr), _measurementLabel(nullptr),
      _recentFilesMenu(nullptr), _snapToGridAction(nullptr),
      _snapToObjectAction(nullptr), _autoSaveAction(nullptr),
      _rulerAction(nullptr), _measurementAction(nullptr),
      _undoRedoManager(std::make_unique<UndoRedoManager>()),
      _leftSideTabBar(nullptr)
#ifdef HAVE_QT_PDF
      ,
      _pdfViewer(nullptr), _thumbnailPanel(nullptr), _centralSplitter(nullptr),
      _pdfPanel(nullptr), _pdfToolBar(nullptr), _pdfHeaderLabel(nullptr),
      _pdfPageLabel(nullptr), _pdfPageSpinBox(nullptr), _pdfZoomCombo(nullptr),
      _pdfDarkModeAction(nullptr), _thumbnailToggleAction(nullptr),
      _pdfPanelOnLeft(false) // PDF panel starts on the right by default
#endif
{
  _canvas->setUndoRedoManager(_undoRedoManager.get());
  applyHistorySettings();
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
  installDockTitleBar(_toolPanel);
  installDockTitleBar(_elementBankPanel);
  updateWindowTitle();
  this->resize(1400, 900);

  setupMenuBar();
  setupStatusBar();
  setupLayerPanel();
  setupSideTabBars();
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
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          [this]() { applyTheme(); });
  applyTheme();

  // Add global shortcut for Backspace delete (Delete key is owned by the
  // Edit-menu QAction; registering both creates ambiguous-shortcut warnings).
#ifndef Q_OS_MACOS
  // (On macOS QKeySequence::Delete already includes Backspace.)
  QShortcut *backspaceShortcut =
      new QShortcut(QKeySequence(Qt::Key_Backspace), this);
  connect(backspaceShortcut, &QShortcut::activated, this,
          &MainWindow::onEditDelete);
#endif

  // Track dirty state so the exit confirmation only appears when work could
  // actually be lost.
  connect(_canvas, &Canvas::canvasModified, this,
          [this]() { setDocumentDirty(true); });
  // Saving (or loading a document) makes the in-memory state match a file
  // again – otherwise the exit prompt would keep firing after a save.
  connect(_canvas, &Canvas::documentSaved, this, [this]() {
    setDocumentDirty(false);
    // The document is safely on disk: a recovery copy would only produce a
    // stale "restore?" prompt on the next launch.
    if (_autoSaveManager)
      _autoSaveManager->clearAutoSave();
    statusBar()->showMessage(
        tr("Saved %1")
            .arg(QDir::toNativeSeparators(_canvas->currentFilePath())),
        3000);
  });
  // Another document replaced this one (the user already chose to save or
  // discard the old one): it gets a fresh recovery session.
  connect(_canvas, &Canvas::documentLoaded, this, [this]() {
    setDocumentDirty(false);
    if (_autoSaveManager)
      _autoSaveManager->startNewDocument();
  });
  // Recovered work was never saved by the user; keep its snapshot until
  // they save or discard it.
  connect(_canvas, &Canvas::documentRecovered, this,
          [this]() { setDocumentDirty(true); });
  connect(_canvas, &Canvas::saveFailed, this, [this](const QString &message) {
    statusBar()->showMessage(tr("Save failed: %1").arg(message), 8000);
  });
  // Opening a project replaces the drawing; give the user a chance to save.
  _canvas->setDiscardChangesHandler([this]() { return maybeSaveChanges(); });
  // Every edit that lands in the history (delete, cut, fill, group, lock,
  // layer operations, ...) modifies the document, whether or not the code
  // path that recorded it also remembered to emit canvasModified.
  _undoRedoManager->setPushListener(
      [this](const void *owner) { markOwnerDirty(owner); });
}

void MainWindow::setDocumentDirty(bool dirty) {
  _documentDirty = dirty;
  updateWindowTitle();
}

void MainWindow::updateWindowTitle() {
  const QString path = _canvas ? _canvas->currentFilePath() : QString();
  const QString name =
      path.isEmpty() ? tr("Untitled") : QFileInfo(path).fileName();
  const QString mode = _pdfModeTitle ? tr("FullScreen Pencil Draw - PDF "
                                          "Annotation Mode")
                                     : tr("FullScreen Pencil Draw");
  // "[*]" is replaced by Qt with the platform's modified marker.
  setWindowTitle(QStringLiteral("%1[*] \u2014 %2").arg(name, mode));
  setWindowModified(_documentDirty);
}

bool MainWindow::maybeSaveChanges() {
  // Text still being typed only counts once committed (which also marks
  // the document dirty for a brand new item).
  _canvas->finishInlineEditing();
  if (!_documentDirty)
    return true;
  const QMessageBox::StandardButton response = QMessageBox::warning(
      this, tr("Unsaved Changes"),
      tr("The drawing has unsaved changes. Do you want to save them?"),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
      QMessageBox::Save);
  if (response == QMessageBox::Cancel)
    return false;
  if (response == QMessageBox::Save) {
    // Save as a project so nothing (layers, text, groups) is flattened;
    // a cancelled or failed save leaves the document dirty.
    _canvas->saveDocument();
    return !_documentDirty;
  }
  return true;
}

bool MainWindow::maybeDiscardPdfAnnotations() {
#ifdef HAVE_QT_PDF
  // Text still being typed (or a path being drawn) only counts once
  // committed; otherwise closing the PDF would drop it without asking.
  if (_pdfViewer && _pdfViewer->hasPdf())
    _pdfViewer->commitPendingEdits();
  if (!_pdfDirty || !_pdfViewer || !_pdfViewer->hasPdf())
    return true;
  // Annotations are exported, not saved: label the buttons accordingly.
  QMessageBox box(
      QMessageBox::Warning, tr("Unsaved PDF Annotations"),
      tr("The PDF annotations have not been exported. Export "
         "them now?"),
      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
  box.setDefaultButton(QMessageBox::Save);
  box.button(QMessageBox::Save)->setText(tr("Export..."));
  box.button(QMessageBox::Discard)->setText(tr("Discard Annotations"));
  const auto response = static_cast<QMessageBox::StandardButton>(box.exec());
  if (response == QMessageBox::Cancel)
    return false;
  if (response == QMessageBox::Save) {
    onExportAnnotatedPdf();
    return !_pdfDirty;
  }
#endif
  return true;
}

MainWindow::~MainWindow() {
  if (_layerPanel) {
    _layerPanel->setItemStore(nullptr);
    _layerPanel->setCanvas(nullptr);
  }
  if (_canvas) {
    _canvas->setUndoRedoManager(nullptr);
  }
#ifdef HAVE_QT_PDF
  if (_pdfViewer) {
    _pdfViewer->setUndoRedoManager(nullptr);
  }
#endif
  if (_undoRedoManager) {
    _undoRedoManager->setPushListener(nullptr);
    _undoRedoManager->clear();
  }
}

void MainWindow::setupStatusBar() {
  // A full shortcut list here was cut off at any normal window width; it
  // lives in Help > Keyboard Shortcuts (F1) instead.
  _statusLabel = new QLabel(
      tr("\u2726 Ready  |  F1: keyboard shortcuts  |  Ctrl+Scroll: zoom"),
      this);
  _measurementLabel = new QLabel("", this);
  statusBar()->addWidget(_statusLabel);
  statusBar()->addPermanentWidget(_measurementLabel);
}

void MainWindow::applyTheme() {
  const bool darkTheme = ThemeManager::instance().isDarkTheme();

  statusBar()->setStyleSheet(darkTheme ? R"(
              QStatusBar {
                background-color: #10161d;
                color: #d0c4b7;
                border-top: 1px solid rgba(255, 244, 230, 0.08);
                padding: 8px 14px;
              }
              QStatusBar::item {
                border: none;
              }
              QLabel {
                color: #d0c4b7;
                font-size: 11px;
                font-weight: 600;
              }
            )"
                                       : R"(
              QStatusBar {
                background-color: #f5efe6;
                color: #7a6858;
                border-top: 1px solid #ddcfbc;
                padding: 8px 14px;
              }
              QStatusBar::item {
                border: none;
              }
              QLabel {
                color: #7a6858;
                font-size: 11px;
                font-weight: 600;
              }
            )");

#ifdef HAVE_QT_PDF
  if (_centralSplitter) {
    _centralSplitter->setStyleSheet(darkTheme ? R"(
                QSplitter {
                  background-color: #0d1217;
                }
                QSplitter::handle {
                  background-color: #10161d;
                  width: 10px;
                }
                QSplitter::handle:hover {
                  background-color: rgba(249, 115, 22, 0.35);
                }
                QFrame#pdfPanel {
                  background-color: #10161d;
                  border-left: 1px solid rgba(255, 244, 230, 0.08);
                }
                QWidget#pdfViewerContainer {
                  background-color: #10161d;
                }
              )"
                                              : R"(
                QSplitter {
                  background-color: #efe5d8;
                }
                QSplitter::handle {
                  background-color: #e6d9ca;
                  width: 10px;
                }
                QSplitter::handle:hover {
                  background-color: rgba(234, 88, 12, 0.22);
                }
                QFrame#pdfPanel {
                  background-color: #f5efe6;
                  border-left: 1px solid #ddcfbc;
                }
                QWidget#pdfViewerContainer {
                  background-color: #f5efe6;
                }
              )");
  }

  if (_pdfHeaderLabel) {
    _pdfHeaderLabel->setStyleSheet(
        darkTheme ? "QLabel { background: qlineargradient(x1:0, y1:0, x2:1, "
                    "y2:0, stop:0 #17212b, stop:1 #10161d); color: #fff7ed; "
                    "padding: 12px; font-weight: 700; letter-spacing: 0.8px; "
                    "border-bottom: 1px solid rgba(255, 244, 230, 0.08); }"
                  : "QLabel { background: qlineargradient(x1:0, y1:0, x2:1, "
                    "y2:0, stop:0 #fff9f1, stop:1 #f1e5d5); color: #31261d; "
                    "padding: 12px; font-weight: 700; letter-spacing: 0.8px; "
                    "border-bottom: 1px solid #ddcfbc; }");
  }

  if (_pdfPageSpinBox) {
    _pdfPageSpinBox->setStyleSheet(darkTheme ? R"(
                QSpinBox {
                  background-color: #17212b;
                  color: #fff7ed;
                  border: 1px solid rgba(255, 244, 230, 0.08);
                  border-radius: 10px;
                  padding: 4px 8px;
                  min-height: 28px;
                  font-weight: 600;
                }
                QSpinBox::up-button, QSpinBox::down-button {
                  width: 0px;
                }
              )"
                                             : R"(
                QSpinBox {
                  background-color: #fff9f1;
                  color: #31261d;
                  border: 1px solid #ddcfbc;
                  border-radius: 10px;
                  padding: 4px 8px;
                  min-height: 28px;
                  font-weight: 600;
                }
                QSpinBox::up-button, QSpinBox::down-button {
                  width: 0px;
                }
              )");
  }

  if (_pdfPageLabel) {
    _pdfPageLabel->setStyleSheet(
        darkTheme
            ? "QLabel { color: #d0c4b7; padding: 0 8px; font-weight: 600; }"
            : "QLabel { color: #7a6858; padding: 0 8px; font-weight: 600; }");
  }

  if (_pdfZoomCombo) {
    _pdfZoomCombo->setStyleSheet(darkTheme ? R"(
                QComboBox {
                  background-color: #17212b;
                  color: #fff7ed;
                  border: 1px solid rgba(255, 244, 230, 0.08);
                  border-radius: 10px;
                  padding: 4px 8px;
                  min-height: 28px;
                  font-weight: 600;
                }
                QComboBox::drop-down {
                  border: none;
                  width: 20px;
                }
                QComboBox::down-arrow {
                  image: url(:/ui-icons/arrow_down_dark.png);
                  width: 10px;
                  height: 6px;
                  margin-right: 5px;
                }
                QComboBox QAbstractItemView {
                  background-color: #17212b;
                  color: #fff7ed;
                  selection-background-color: #f97316;
                  selection-color: #fffaf4;
                }
              )"
                                           : R"(
                QComboBox {
                  background-color: #fff9f1;
                  color: #31261d;
                  border: 1px solid #ddcfbc;
                  border-radius: 10px;
                  padding: 4px 8px;
                  min-height: 28px;
                  font-weight: 600;
                }
                QComboBox::drop-down {
                  border: none;
                  width: 20px;
                }
                QComboBox::down-arrow {
                  image: url(:/ui-icons/arrow_down_light.png);
                  width: 10px;
                  height: 6px;
                  margin-right: 5px;
                }
                QComboBox QAbstractItemView {
                  background-color: #fffaf4;
                  color: #31261d;
                  selection-background-color: #f97316;
                  selection-color: #fffaf4;
                }
              )");
  }

  if (_pdfToolBar) {
    _pdfToolBar->setStyleSheet(darkTheme ? R"(
                QToolBar {
                  background-color: #10161d;
                  border-bottom: 1px solid rgba(255, 244, 230, 0.08);
                  padding: 6px 8px;
                  spacing: 4px;
                }
                QToolButton {
                  background-color: #17212b;
                  color: #fff7ed;
                  border: 1px solid rgba(255, 244, 230, 0.08);
                  border-radius: 12px;
                  padding: 6px 8px;
                  min-width: 32px;
                  min-height: 32px;
                  font-size: 15px;
                  font-weight: 600;
                }
                QToolButton:hover {
                  background-color: #1d2934;
                  border: 1px solid rgba(249, 115, 22, 0.35);
                }
                QToolButton:pressed {
                  background-color: rgba(249, 115, 22, 0.14);
                }
                QToolButton:checked {
                  background-color: #f97316;
                  color: #fffaf4;
                  border: 1px solid rgba(255, 244, 230, 0.22);
                }
                /* Qt reserves a narrow slot for the overflow ("more")
                   button; the padded button style made it spill off the
                   window edge. */
                QToolButton#qt_toolbar_ext_button {
                  min-width: 0px;
                  min-height: 0px;
                  padding: 2px;
                  border-radius: 6px;
                }
                QToolButton#pdfModeButton {
                  min-width: 118px;
                }
                QToolBar::separator {
                  background-color: rgba(249, 115, 22, 0.22);
                  width: 1px;
                  margin: 6px 8px;
                }
              )"
                                         : R"(
                QToolBar {
                  background-color: #f5efe6;
                  border-bottom: 1px solid #ddcfbc;
                  padding: 6px 8px;
                  spacing: 4px;
                }
                QToolButton {
                  background-color: #fff9f1;
                  color: #31261d;
                  border: 1px solid #ddcfbc;
                  border-radius: 12px;
                  padding: 6px 8px;
                  min-width: 32px;
                  min-height: 32px;
                  font-size: 15px;
                  font-weight: 600;
                }
                QToolButton:hover {
                  background-color: #fff4e7;
                  border: 1px solid rgba(234, 88, 12, 0.28);
                }
                QToolButton:pressed {
                  background-color: #f6dfca;
                }
                QToolButton:checked {
                  background-color: #f97316;
                  color: #fffaf4;
                  border: 1px solid rgba(117, 59, 19, 0.15);
                }
                /* Qt reserves a narrow slot for the overflow ("more")
                   button; the padded button style made it spill off the
                   window edge. */
                QToolButton#qt_toolbar_ext_button {
                  min-width: 0px;
                  min-height: 0px;
                  padding: 2px;
                  border-radius: 6px;
                }
                QToolButton#pdfModeButton {
                  min-width: 118px;
                }
                QToolBar::separator {
                  background-color: rgba(234, 88, 12, 0.18);
                  width: 1px;
                  margin: 6px 8px;
                }
              )");

    // Re-tint all PDF toolbar action icons for the new theme.
    const QColor tint = darkTheme ? Qt::white : Qt::black;
    const qreal dpr = devicePixelRatioF();
    for (QAction *action : _pdfToolBar->actions()) {
      if (action->isSeparator() || action->icon().isNull())
        continue;
      const int px = qRound(22 * dpr);
      QPixmap pm = action->icon().pixmap(QSize(px, px));
      pm.setDevicePixelRatio(dpr);
      QPainter p(&pm);
      p.setCompositionMode(QPainter::CompositionMode_SourceIn);
      p.fillRect(pm.rect(), tint);
      p.end();
      action->setIcon(QIcon(pm));
    }
  }
#endif
}

bool MainWindow::pdfIsActive() const {
#ifdef HAVE_QT_PDF
  // Edit/zoom commands follow the surface the user works in: the PDF when
  // it has focus or the pointer was last over it.
  return _pdfViewer && _pdfPanel && _pdfPanel->isVisible() &&
         _pdfViewer->hasPdf() &&
         (_pdfViewer->hasFocus() || _activeSurface == ActiveSurface::Pdf);
#else
  return false;
#endif
}

void MainWindow::onEditDelete() {
#ifdef HAVE_QT_PDF
  if (pdfIsActive()) {
    _pdfViewer->deleteSelectedItems();
    return;
  }
#endif
  _canvas->deleteSelectedItems();
}

void MainWindow::onEditSelectAll() {
#ifdef HAVE_QT_PDF
  if (pdfIsActive()) {
    _pdfViewer->selectAll();
    return;
  }
#endif
  _canvas->selectAll();
}

// Clipboard and duplicate operate on canvas items only; never let them act
// on a canvas selection the user can't see while annotating a PDF.
void MainWindow::onEditCut() {
  if (pdfIsActive()) {
    statusBar()->showMessage(tr("Cut is not available for PDF annotations"),
                             3000);
    return;
  }
  _canvas->cutSelectedItems();
}

void MainWindow::onEditCopy() {
  if (pdfIsActive()) {
    statusBar()->showMessage(tr("Copy is not available for PDF annotations"),
                             3000);
    return;
  }
  _canvas->copySelectedItems();
}

void MainWindow::onEditPaste() {
  if (pdfIsActive()) {
    statusBar()->showMessage(tr("Paste is not available for PDF annotations"),
                             3000);
    return;
  }
  _canvas->pasteItems();
}

void MainWindow::onEditDuplicate() {
  if (pdfIsActive()) {
    statusBar()->showMessage(
        tr("Duplicate is not available for PDF annotations"), 3000);
    return;
  }
  _canvas->duplicateSelectedItems();
}

void MainWindow::onZoomIn() {
#ifdef HAVE_QT_PDF
  if (pdfIsActive()) {
    _pdfViewer->zoomIn();
    return;
  }
#endif
  _canvas->zoomIn();
}

void MainWindow::onZoomOut() {
#ifdef HAVE_QT_PDF
  if (pdfIsActive()) {
    _pdfViewer->zoomOut();
    return;
  }
#endif
  _canvas->zoomOut();
}

void MainWindow::onZoomReset() {
#ifdef HAVE_QT_PDF
  if (pdfIsActive()) {
    _pdfViewer->zoomReset();
    return;
  }
#endif
  _canvas->zoomReset();
}

void MainWindow::commitGesturesBeforeHistory() {
  // Replaying history must not interleave with an unfinished gesture: an
  // open text editor or half-built path would otherwise be committed from
  // inside the replay.
  _canvas->finishInlineEditing();
  _canvas->finishActiveGesture();
#ifdef HAVE_QT_PDF
  if (_pdfViewer && _pdfViewer->hasPdf())
    _pdfViewer->commitActiveGesture();
#endif
}

void MainWindow::markOwnerDirty(const void *owner) {
  // The history is shared between canvas and PDF: mark the surface whose
  // action was replayed (fall back to the one the user works in).
#ifdef HAVE_QT_PDF
  if (owner && owner == _pdfViewer) {
    _pdfDirty = true;
    return;
  }
#endif
  if (owner && owner == _canvas) {
    setDocumentDirty(true);
    return;
  }
  if (_activeSurface == ActiveSurface::Pdf)
    _pdfDirty = true;
  else
    setDocumentDirty(true);
}

void MainWindow::performUndo() {
  if (!_undoRedoManager)
    return;
  commitGesturesBeforeHistory();
  if (_undoRedoManager->canUndo()) {
    const void *owner = _undoRedoManager->undoOwner();
    _undoRedoManager->undo();
    markOwnerDirty(owner);
  }
  // Undone transforms move items without a selection change, so the handles
  // would otherwise keep showing the pre-undo bounds.
  if (_canvas)
    _canvas->updateTransformHandles();
}

void MainWindow::performRedo() {
  if (!_undoRedoManager)
    return;
  commitGesturesBeforeHistory();
  if (_undoRedoManager->canRedo()) {
    const void *owner = _undoRedoManager->redoOwner();
    _undoRedoManager->redo();
    markOwnerDirty(owner);
  }
  if (_canvas)
    _canvas->updateTransformHandles();
}

void MainWindow::setupConnections() {
  // Tool selections
  connect(_toolPanel, &ToolPanel::penSelected, _canvas, &Canvas::setPenTool);
  connect(_toolPanel, &ToolPanel::highlighterSelected, _canvas,
          &Canvas::setHighlighterTool);
  connect(_toolPanel, &ToolPanel::eraserSelected, _canvas,
          &Canvas::setEraserTool);
  connect(_toolPanel, &ToolPanel::pixelEraserSelected, _canvas,
          &Canvas::setPixelEraserTool);
  connect(_canvas, &Canvas::statusMessage, this,
          [this](const QString &msg) { statusBar()->showMessage(msg, 6000); });
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
  connect(_toolPanel, &ToolPanel::wireSelected, _canvas, &Canvas::setWireTool);
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
    connect(_toolPanel, &ToolPanel::highlighterSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Highlighter);
    });
    connect(_toolPanel, &ToolPanel::eraserSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Eraser);
    });
    // PDF annotations are vector objects only: there are no pixels to
    // erase, so the PDF keeps its current tool.
    connect(_toolPanel, &ToolPanel::pixelEraserSelected, this, [this]() {
      if (_pdfPanel && _pdfPanel->isVisible())
        statusBar()->showMessage(
            tr("PDF annotations have no pixels; use the Object Eraser (E) "
               "to remove them."),
            6000);
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
    connect(_toolPanel, &ToolPanel::lassoSelectionSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::LassoSelection);
    });
    connect(_toolPanel, &ToolPanel::bezierSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::Bezier);
    });
    connect(_toolPanel, &ToolPanel::textOnPathSelected, this, [this]() {
      _pdfViewer->setToolType(ToolManager::ToolType::TextOnPath);
    });

    // Canvas-only tools: say so instead of silently leaving the PDF on the
    // previous tool while the panel highlights the new one.
    auto canvasOnly = [this](const QString &tool) {
      if (_pdfPanel && _pdfPanel->isVisible() && _pdfViewer->hasPdf()) {
        statusBar()->showMessage(
            tr("%1 is only available on the canvas").arg(tool), 4000);
      }
    };
    connect(_toolPanel, &ToolPanel::colorSelectSelected, this,
            [canvasOnly]() { canvasOnly(tr("Color Select")); });
    connect(_toolPanel, &ToolPanel::curvedArrowSelected, this,
            [canvasOnly]() { canvasOnly(tr("Curved Arrow")); });
    connect(_toolPanel, &ToolPanel::wireSelected, this,
            [canvasOnly]() { canvasOnly(tr("Wire")); });

    // Connect color selection to PDF viewer
    connect(_toolPanel, &ToolPanel::colorSelected, _pdfViewer,
            &PdfViewer::setPenColor);
    // Connect brush size and opacity changes to PDF viewer
    connect(_canvas, &Canvas::brushSizeChanged, _pdfViewer,
            &PdfViewer::setPenWidth);
    connect(_canvas, &Canvas::opacityChanged, _pdfViewer,
            &PdfViewer::setOpacity);
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
  connect(_toolPanel, &ToolPanel::lassoSelectionSelected, _canvas,
          [this]() { _canvas->setShape("LassoSelection"); });

  // Edit operations
  // (routed through MainWindow so they act on the PDF while annotating it)
  connect(_toolPanel, &ToolPanel::copyAction, this, &MainWindow::onEditCopy);
  connect(_toolPanel, &ToolPanel::cutAction, this, &MainWindow::onEditCut);
  connect(_toolPanel, &ToolPanel::pasteAction, this, &MainWindow::onEditPaste);
  connect(_toolPanel, &ToolPanel::duplicateAction, this,
          &MainWindow::onEditDuplicate);
  connect(_toolPanel, &ToolPanel::deleteAction, this,
          &MainWindow::onEditDelete);
  connect(_toolPanel, &ToolPanel::selectAllAction, this,
          &MainWindow::onEditSelectAll);

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
          [this](const QPointF &pos) {
            _activeSurface = ActiveSurface::Canvas;
            onCursorPositionChanged(pos);
          });

  // Undo/Redo
  connect(_toolPanel, &ToolPanel::undoAction, this, &MainWindow::performUndo);
  connect(_toolPanel, &ToolPanel::redoAction, this, &MainWindow::performRedo);

  // Zoom
  connect(_toolPanel, &ToolPanel::zoomInAction, this, &MainWindow::onZoomIn);
  connect(_toolPanel, &ToolPanel::zoomOutAction, this, &MainWindow::onZoomOut);
  connect(_toolPanel, &ToolPanel::zoomResetAction, this,
          &MainWindow::onZoomReset);
  connect(_toolPanel, &ToolPanel::toggleGridAction, _canvas,
          &Canvas::toggleGrid);
  connect(_toolPanel, &ToolPanel::toggleFilledShapesAction, _canvas,
          &Canvas::toggleFilledShapes);
  connect(_toolPanel, &ToolPanel::fillBrushSelected, _canvas,
          &Canvas::setFillBrush);

  // Pressure sensitivity
  connect(_toolPanel, &ToolPanel::pressureSensitivityToggled, _canvas,
          &Canvas::togglePressureSensitivity);

  // Brush tip
  connect(_toolPanel, &ToolPanel::brushTipSelected, _canvas,
          &Canvas::setBrushTip);

  // Element bank
  connect(_elementBankPanel, &ElementBankPanel::elementSelected, _canvas,
          &Canvas::placeElement);

  // Filled shapes feedback
  connect(_canvas, &Canvas::filledShapesChanged, this,
          &MainWindow::onFilledShapesChanged);
  // The grid can be toggled from the menu, the tool panel or the G key;
  // keep every checkable control in step.
  connect(_canvas, &Canvas::gridVisibilityChanged, this, [this](bool visible) {
    _toolPanel->updateGridDisplay(visible);
    if (_gridAction)
      _gridAction->setChecked(visible);
  });

  // Snap to grid feedback
  connect(_canvas, &Canvas::snapToGridChanged, this,
          &MainWindow::onSnapToGridChanged);

  // Snap to object feedback
  connect(_canvas, &Canvas::snapToObjectChanged, this,
          &MainWindow::onSnapToObjectChanged);

  // Ruler and measurement feedback
  connect(_canvas, &Canvas::rulerVisibilityChanged, this,
          &MainWindow::onRulerVisibilityChanged);
  connect(_canvas, &Canvas::measurementToolChanged, this,
          &MainWindow::onMeasurementToolChanged);
  connect(_canvas, &Canvas::measurementUpdated, this,
          &MainWindow::onMeasurementUpdated);

  // File operations
  connect(_toolPanel, &ToolPanel::saveAction, _canvas, &Canvas::saveDocument);
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
  fileMenu->addAction("Open Image (&Original Size)...", _canvas,
                      SLOT(openSingleImage()));

#ifdef HAVE_QT_PDF
  createAction(fileMenu, "Open P&DF...",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), this,
               SLOT(onOpenPdf()));
#endif

  // Recent Files submenu
  _recentFilesMenu = fileMenu->addMenu("Recent Files");
  updateRecentFilesMenu();

  fileMenu->addSeparator();

  // Save / Save As write the editable native project; exports write other
  // formats and never mark the document as saved.
  createAction(fileMenu, "&Save", QKeySequence::Save, _canvas,
               SLOT(saveDocument()));
  createAction(fileMenu, "Save Project &As...", QKeySequence::SaveAs, _canvas,
               SLOT(saveProject()));
  createAction(fileMenu, "&Export (Image, SVG, PDF)...",
               QKeySequence(Qt::CTRL | Qt::Key_E), _canvas, SLOT(saveToFile()));
  fileMenu->addAction("Export to &PDF...", _canvas, SLOT(exportToPDF()));
  fileMenu->addAction("Export Single &Element...", _canvas,
                      SLOT(exportSingleElementToPNG()));

#ifdef HAVE_QT_PDF
  _exportAnnotatedPdfAction = fileMenu->addAction(
      "Export &Annotated PDF...", this, SLOT(onExportAnnotatedPdf()));
  _closePdfAction = fileMenu->addAction("&Close PDF", this, SLOT(onClosePdf()));
  // Only meaningful while a PDF is open (see show/hidePdfPanel).
  _exportAnnotatedPdfAction->setEnabled(false);
  _closePdfAction->setEnabled(false);
#endif

  fileMenu->addSeparator();

  createAction(fileMenu, "E&xit", QKeySequence::Quit, this, SLOT(close()));

  // Edit menu
  QMenu *editMenu = menuBar->addMenu("&Edit");

  QAction *undoEditAction = editMenu->addAction("&Undo");
  undoEditAction->setShortcut(QKeySequence::Undo);
  connect(undoEditAction, &QAction::triggered, this, &MainWindow::performUndo);

  QAction *redoEditAction = editMenu->addAction("&Redo");
  // QKeySequence::Redo is only Ctrl+Shift+Z on Linux/macOS; the UI advertises
  // Ctrl+Y everywhere, so bind it explicitly as well.
  QList<QKeySequence> redoShortcuts =
      QKeySequence::keyBindings(QKeySequence::Redo);
  if (!redoShortcuts.contains(QKeySequence(Qt::CTRL | Qt::Key_Y)))
    redoShortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Y));
  redoEditAction->setShortcuts(redoShortcuts);
  connect(redoEditAction, &QAction::triggered, this, &MainWindow::performRedo);
  editMenu->addSeparator();
  createAction(editMenu, "Cu&t", QKeySequence::Cut, this, SLOT(onEditCut()));
  createAction(editMenu, "&Copy", QKeySequence::Copy, this, SLOT(onEditCopy()));
  createAction(editMenu, "&Paste", QKeySequence::Paste, this,
               SLOT(onEditPaste()));
  editMenu->addSeparator();
  createAction(editMenu, "Select &All", QKeySequence::SelectAll, this,
               SLOT(onEditSelectAll()));
  createAction(editMenu, "&Delete", QKeySequence::Delete, this,
               SLOT(onEditDelete()));
  createAction(editMenu, "D&uplicate", QKeySequence(Qt::CTRL | Qt::Key_D), this,
               SLOT(onEditDuplicate()));
  createAction(editMenu, "Extract Color Selection to New Layer",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_J), _canvas,
               SLOT(extractColorSelectionToNewLayer()));

  // View menu
  QMenu *viewMenu = menuBar->addMenu("&View");

  createAction(viewMenu, "Zoom &In", QKeySequence::ZoomIn, this,
               SLOT(onZoomIn()));
  createAction(viewMenu, "Zoom &Out", QKeySequence::ZoomOut, this,
               SLOT(onZoomOut()));
  createAction(viewMenu, "&Reset Zoom", QKeySequence(Qt::Key_0), this,
               SLOT(onZoomReset()));
  viewMenu->addSeparator();

  _gridAction = createAction(viewMenu, "Toggle &Grid", QKeySequence(Qt::Key_G),
                             _canvas, SLOT(toggleGrid()));
  _gridAction->setCheckable(true);
  _gridAction->setChecked(_canvas->isGridVisible());

  _snapToGridAction = createAction(viewMenu, "&Snap to Grid",
                                   QKeySequence(Qt::SHIFT | Qt::Key_G), _canvas,
                                   SLOT(toggleSnapToGrid()));
  _snapToGridAction->setCheckable(true);
  _snapToGridAction->setChecked(_canvas->isSnapToGridEnabled());

  _snapToObjectAction =
      createAction(viewMenu, "Snap to &Objects",
                   QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G), _canvas,
                   SLOT(toggleSnapToObject()));
  _snapToObjectAction->setCheckable(true);
  _snapToObjectAction->setChecked(_canvas->isSnapToObjectEnabled());

  _filledAction =
      createAction(viewMenu, "Toggle &Filled Shapes", QKeySequence(Qt::Key_B),
                   _canvas, SLOT(toggleFilledShapes()));
  _filledAction->setCheckable(true);
  _filledAction->setChecked(_canvas->isFilledShapes());

  viewMenu->addSeparator();

  _rulerAction =
      createAction(viewMenu, "Show &Ruler", QKeySequence(Qt::CTRL | Qt::Key_R),
                   _canvas, SLOT(toggleRuler()));
  _rulerAction->setCheckable(true);
  _rulerAction->setChecked(_canvas->isRulerVisible());

  _measurementAction = createAction(viewMenu, "&Measurement Tool",
                                    QKeySequence(Qt::ALT | Qt::Key_M), _canvas,
                                    SLOT(toggleMeasurementTool()));
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
  QAction *showElementsAction = _elementBankPanel->toggleViewAction();
  showElementsAction->setText("Show &Element Library");
  showElementsAction->setShortcut(QKeySequence(Qt::Key_F7));
  viewMenu->addAction(showElementsAction);

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

  // Edit menu - scaling (elements)
  editMenu->addSeparator();
  // Ctrl+Shift+S is Save As on every platform, so resizing uses Ctrl+Alt+S.
  createAction(editMenu, "Resize Selected &Elements...",
               QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S), _canvas,
               SLOT(scaleSelectedItems()));
  createAction(editMenu, "Resize Active &Layer...",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), _canvas,
               SLOT(scaleActiveLayer()));

  // Edit menu - perspective transform
  createAction(editMenu, "&Perspective Transform...",
               QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), _canvas,
               SLOT(perspectiveTransformSelectedItems()));

  // Edit menu - rotation and alignment
  editMenu->addSeparator();
  createAction(editMenu, "Rotate Selected &Items...",
               QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_R), _canvas,
               SLOT(rotateSelectedItems()));
  editMenu->addAction("Align Items...", _canvas, SLOT(alignSelectedItems()));

  // Edit menu - canvas resize
  editMenu->addSeparator();
  createAction(editMenu, "Resize &Canvas...",
               QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C), _canvas,
               SLOT(resizeCanvas()));

  // Layer menu: vector and raster layers live side by side.
  QMenu *layerMenu = menuBar->addMenu(tr("&Layer"));
  QAction *newVectorLayer =
      layerMenu->addAction(tr("New &Vector Layer"), this, [this]() {
        if (LayerManager *layers = _canvas->layerManager()) {
          layers->createLayer(tr("Layer %1").arg(layers->layerCount() + 1));
          layers->setActiveLayer(layers->layerCount() - 1);
          if (_layerPanel)
            _layerPanel->refreshLayerList();
        }
      });
  newVectorLayer->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
  layerMenu->addAction(tr("New &Raster (Pixel) Layer"), this, [this]() {
    if (_layerPanel)
      _layerPanel->onAddRasterLayer();
  });

  // Tools menu
  QMenu *toolsMenu = menuBar->addMenu("&Tools");

  // The two erasers do different things and must both be discoverable. The
  // "\t" text only displays the key: E / Shift+E are handled in
  // keyPressEvent so they never steal letters from text editing.
  QMenu *eraserMenu = toolsMenu->addMenu(tr("&Erasers"));
  QAction *objectEraserAction =
      eraserMenu->addAction(tr("&Object Eraser (delete whole objects)\tE"),
                            _toolPanel, &ToolPanel::onActionEraser);
  objectEraserAction->setStatusTip(
      tr("Delete every editable object the eraser touches"));
  QAction *pixelEraserAction =
      eraserMenu->addAction(tr("&Pixel Eraser (erase raster pixels)\tShift+E"),
                            _toolPanel, &ToolPanel::onActionPixelEraser);
  pixelEraserAction->setStatusTip(
      tr("Make pixels of raster layers, images and brush strokes transparent"));
  toolsMenu->addSeparator();

  _autoSaveAction = toolsMenu->addAction("Enable &Auto-Save");
  connect(_autoSaveAction, &QAction::triggered, this, [this]() {
    if (_autoSaveManager) {
      _autoSaveManager->setEnabled(!_autoSaveManager->isEnabled());
      _autoSaveAction->setChecked(_autoSaveManager->isEnabled());
    }
  });
  _autoSaveAction->setCheckable(true);
  _autoSaveAction->setChecked(true); // synced in setupAutoSave()
  toolsMenu->addAction(tr("Auto-Save &Interval..."), this, [this]() {
    if (!_autoSaveManager)
      return;
    bool ok = false;
    const int minutes = QInputDialog::getInt(
        this, tr("Auto-Save Interval"),
        tr("Save a recovery copy of unsaved work every (minutes):"),
        _autoSaveManager->intervalMinutes(), 1, 60, 1, &ok);
    if (ok) {
      _autoSaveManager->setIntervalMinutes(minutes);
      statusBar()->showMessage(
          tr("Auto-save every %n minute(s)", nullptr, minutes), 3000);
    }
  });

  toolsMenu->addAction(tr("&History Settings..."), this,
                       &MainWindow::onHistorySettings);

  toolsMenu->addSeparator();

  toolsMenu->addAction(tr("Pixel Eraser &Strength..."), this, [this]() {
    bool ok = false;
    const int value = QInputDialog::getInt(
        this, tr("Pixel Eraser Strength"), tr("Alpha removed per pass (%):"),
        _canvas->pixelEraserStrength(), 1, 100, 5, &ok);
    if (ok)
      _canvas->setPixelEraserStrength(value);
  });
  toolsMenu->addAction(tr("Pixel Eraser &Hardness..."), this, [this]() {
    bool ok = false;
    const int value =
        QInputDialog::getInt(this, tr("Pixel Eraser Hardness"),
                             tr("Edge hardness (%, 100 = hard edge):"),
                             _canvas->pixelEraserHardness(), 0, 100, 5, &ok);
    if (ok)
      _canvas->setPixelEraserHardness(value);
  });

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
  filtersMenu->addAction("&Blur...", _canvas, SLOT(applyBlurToSelection()));
  filtersMenu->addAction("&Sharpen...", _canvas,
                         SLOT(applySharpenToSelection()));
  filtersMenu->addAction("Scan &Document...", _canvas,
                         SLOT(applyScanDocumentToSelection()));
  filtersMenu->addAction("Color Curves / &Levels...", _canvas,
                         SLOT(applyColorCurvesToSelection()));

  // Help menu
  QMenu *helpMenu = menuBar->addMenu("&Help");

  QAction *shortcutsAction =
      helpMenu->addAction(tr("&Keyboard Shortcuts"), this, [this]() {
        QMessageBox box(this);
        box.setWindowTitle(tr("Keyboard Shortcuts"));
        box.setTextFormat(Qt::RichText);
        box.setText(tr(
            "<table cellspacing='4'>"
            "<tr><td colspan='2'><b>Tools</b></td></tr>"
            "<tr><td>P / I</td><td>Pen / Highlighter</td></tr>"
            "<tr><td>E / Shift+E</td><td>Object Eraser / Pixel Eraser</td></tr>"
            "<tr><td>T / Shift+T</td><td>Text / Text on Path</td></tr>"
            "<tr><td>M</td><td>Mermaid diagram</td></tr>"
            "<tr><td>F / Q</td><td>Fill / Select by Color</td></tr>"
            "<tr><td>L / A / Shift+A</td><td>Line / Arrow / Curved "
            "Arrow</td></tr>"
            "<tr><td>R / C</td><td>Rectangle / Circle</td></tr>"
            "<tr><td>Shift+B / W</td><td>Bezier / Wire</td></tr>"
            "<tr><td>S or V / Shift+S</td><td>Select / Lasso</td></tr>"
            "<tr><td>H / K</td><td>Pan / Color</td></tr>"
            "<tr><td>[ / ]</td><td>Brush size</td></tr>"
            "<tr><td>Enter / Esc</td><td>Finish / cancel a path</td></tr>"
            "<tr><td colspan='2'><b>View</b></td></tr>"
            "<tr><td>G / Shift+G</td><td>Grid / Snap to grid</td></tr>"
            "<tr><td>B</td><td>Filled shapes</td></tr>"
            "<tr><td>Ctrl+Scroll, + / - / 0</td><td>Zoom</td></tr>"
            "<tr><td colspan='2'><b>Edit and file</b></td></tr>"
            "<tr><td>Ctrl+Z / Ctrl+Y</td><td>Undo / Redo</td></tr>"
            "<tr><td>Ctrl+C / X / V / D</td><td>Copy / Cut / Paste / "
            "Duplicate</td></tr>"
            "<tr><td>Ctrl+G / Ctrl+Shift+U</td><td>Group / Ungroup</td></tr>"
            "<tr><td>Ctrl+L / Ctrl+Shift+L</td><td>Lock / Unlock all</td></tr>"
            "<tr><td>Ctrl+S / Ctrl+Shift+S</td><td>Save / Save As</td></tr>"
            "<tr><td>Ctrl+E</td><td>Export</td></tr>"
            "<tr><td>Ctrl+Shift+N</td><td>New layer</td></tr>"
            "</table>"));
        box.exec();
      });
  shortcutsAction->setShortcut(QKeySequence(Qt::Key_F1));
  helpMenu->addSeparator();
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
    installDockTitleBar(_layerPanel);

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

void MainWindow::setupSideTabBars() {
  _leftSideTabBar = new SideTabBar("Left Panel Tabs", this);
  addToolBar(Qt::LeftToolBarArea, _leftSideTabBar);

  _leftSideTabBar->trackDockWidget(_toolPanel);
  _leftSideTabBar->trackDockWidget(_elementBankPanel);
  if (_layerPanel) {
    _leftSideTabBar->trackDockWidget(_layerPanel);
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
  if (_filledAction)
    _filledAction->setChecked(filled);
}
void MainWindow::onCursorPositionChanged(const QPointF &pos) {
  _toolPanel->updatePositionDisplay(pos);
}

void MainWindow::onSnapToGridChanged(bool enabled) {
  if (_snapToGridAction) {
    _snapToGridAction->setChecked(enabled);
  }
}

void MainWindow::onSnapToObjectChanged(bool enabled) {
  if (_snapToObjectAction) {
    _snapToObjectAction->setChecked(enabled);
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
  // The menu is built first; show the saved setting, not "on" regardless.
  if (_autoSaveAction)
    _autoSaveAction->setChecked(_autoSaveManager->isEnabled());

  connect(_autoSaveManager, &AutoSaveManager::autoSavePerformed, this,
          &MainWindow::onAutoSavePerformed);

  // Only auto-save when there is something the user could lose.
  _autoSaveManager->setShouldSaveCheck([this]() { return _documentDirty; });

  connect(_autoSaveManager, &AutoSaveManager::autoSaveFailed, this,
          [this](const QString &message) {
            statusBar()->showMessage(
                tr("Auto-save failed (the previous recovery copy is kept): "
                   "%1")
                    .arg(message),
                8000);
          });

  // Offer work left behind by sessions that did not close normally.
  if (_autoSaveManager->restoreAutoSave()) {
    // The recovered work was never saved by the user.
    setDocumentDirty(true);
  }

  // Update the menu action state
  if (_autoSaveAction) {
    _autoSaveAction->setChecked(_autoSaveManager->isEnabled());
  }
}

void MainWindow::applyHistorySettings() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  HistoryPolicy policy;
  policy.maxSteps = static_cast<std::size_t>(qMax(
      0,
      settings.value("history/maxSteps", int(UndoRedoManager::kDefaultMaxSteps))
          .toInt()));
  const int budgetMb =
      qMax(0, settings
                  .value("history/memoryBudgetMB",
                         int(UndoRedoManager::kDefaultMemoryBudgetBytes >> 20))
                  .toInt());
  policy.memoryBudgetBytes = static_cast<std::size_t>(budgetMb) << 20;
  if (_undoRedoManager)
    _undoRedoManager->setPolicy(policy);
}

void MainWindow::onHistorySettings() {
  if (!_undoRedoManager)
    return;
  const HistoryPolicy current = _undoRedoManager->policy();
  QDialog dialog(this);
  dialog.setWindowTitle(tr("History Settings"));
  auto *form = new QFormLayout(&dialog);
  auto *steps = new QSpinBox(&dialog);
  steps->setRange(0, 100000);
  steps->setSpecialValueText(tr("Unlimited"));
  steps->setValue(static_cast<int>(current.maxSteps));
  auto *budget = new QSpinBox(&dialog);
  budget->setRange(0, 65536);
  budget->setSuffix(tr(" MiB"));
  budget->setSpecialValueText(tr("Unlimited"));
  budget->setValue(static_cast<int>(current.memoryBudgetBytes >> 20));
  form->addRow(tr("Undo steps to keep:"), steps);
  form->addRow(tr("Memory for undo history:"), budget);
  auto *usage = new QLabel(
      tr("Currently used: %1 steps, %2 MiB. The oldest steps are dropped "
         "first when either limit is reached; history is not stored in "
         "saved files.")
          .arg(_undoRedoManager->undoCount() + _undoRedoManager->redoCount())
          .arg(QString::number(double(_undoRedoManager->memoryUsage()) /
                                   (1024.0 * 1024.0),
                               'f', 1)),
      &dialog);
  usage->setWordWrap(true);
  form->addRow(usage);
  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  form->addRow(buttons);
  if (dialog.exec() != QDialog::Accepted)
    return;
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  settings.setValue("history/maxSteps", steps->value());
  settings.setValue("history/memoryBudgetMB", budget->value());
  applyHistorySettings();
}

void MainWindow::onAutoSavePerformed(const QString &path) {
  statusBar()->showMessage(QString("Auto-saved to: %1").arg(path), 3000);
}

void MainWindow::onNewCanvas() {
  if (!maybeSaveChanges())
    return;
  // One dialog for size and background (it used to be three prompts in a
  // row); remembers the last choice.
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  QDialog dialog(this);
  dialog.setWindowTitle(tr("New Canvas"));
  auto *widthBox = new QSpinBox(&dialog);
  auto *heightBox = new QSpinBox(&dialog);
  for (QSpinBox *box : {widthBox, heightBox}) {
    box->setRange(1, 20000); // same order as Resize Canvas, bounded memory
    box->setSingleStep(100);
    box->setSuffix(tr(" px"));
  }
  widthBox->setValue(settings.value("newCanvas/width", 1920).toInt());
  heightBox->setValue(settings.value("newCanvas/height", 1080).toInt());
  QColor bgColor(settings
                     .value("newCanvas/background",
                            _canvas->backgroundColor().name(QColor::HexArgb))
                     .toString());
  if (!bgColor.isValid())
    bgColor = _canvas->backgroundColor();
  auto *colorButton = new QPushButton(&dialog);
  auto showColor = [colorButton](const QColor &c) {
    QPixmap swatch(28, 16);
    swatch.fill(c);
    colorButton->setIcon(QIcon(swatch));
    colorButton->setText(c.name(QColor::HexRgb).toUpper());
  };
  showColor(bgColor);
  connect(colorButton, &QPushButton::clicked, &dialog, [&]() {
    const QColor chosen =
        QColorDialog::getColor(bgColor, &dialog, tr("Background Color"));
    if (chosen.isValid()) {
      bgColor = chosen;
      showColor(bgColor);
    }
  });
  auto *form = new QFormLayout;
  form->addRow(tr("Width:"), widthBox);
  form->addRow(tr("Height:"), heightBox);
  form->addRow(tr("Background:"), colorButton);
  auto *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  auto *layout = new QVBoxLayout(&dialog);
  layout->addLayout(form);
  layout->addWidget(buttons);
  if (dialog.exec() != QDialog::Accepted)
    return;
  settings.setValue("newCanvas/width", widthBox->value());
  settings.setValue("newCanvas/height", heightBox->value());
  settings.setValue("newCanvas/background", bgColor.name(QColor::HexArgb));
  _canvas->newCanvas(widthBox->value(), heightBox->value(), bgColor);
  // A brand new empty canvas holds nothing worth warning about on exit.
  setDocumentDirty(false);
  if (_autoSaveManager)
    _autoSaveManager->startNewDocument();
  // Refresh layer panel after new canvas
  if (_layerPanel) {
    _layerPanel->refreshLayerList();
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  // Only interrupt the user when unsaved work could actually be lost.
  if (!maybeSaveChanges() || !maybeDiscardPdfAnnotations()) {
    event->ignore();
    return;
  }
  // Clean exit: the user saved or chose to discard, nothing to recover.
  if (_autoSaveManager)
    _autoSaveManager->clearAutoSave();
  event->accept();
  QMainWindow::closeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);
  if (_docksSized)
    return;
  _docksSized = true;
  // By default the element library got a single row of cards while the
  // layer list below it stood mostly empty; give the library room for a
  // few rows. Done on the first show (not from the constructor): dock sizes
  // only apply once the window is laid out, which is later when a recovery
  // prompt is shown first.
  QTimer::singleShot(0, this, [this]() {
    if (_elementBankPanel && _layerPanel && _elementBankPanel->isVisible() &&
        _layerPanel->isVisible())
      resizeDocks({_elementBankPanel, _layerPanel}, {340, 560}, Qt::Vertical);
  });
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  // Single-letter tool keys must not also fire for Ctrl+<key> combinations.
  const bool ctrl = event->modifiers() & Qt::ControlModifier;
  // Standard shortcuts
  if (event->matches(QKeySequence::Copy)) {
    onEditCopy();
  } else if (event->matches(QKeySequence::Cut)) {
    onEditCut();
  } else if (event->matches(QKeySequence::Paste)) {
    onEditPaste();
  } else if (event->matches(QKeySequence::Undo)) {
    performUndo();
  } else if (event->matches(QKeySequence::Redo)) {
    performRedo();
  } else if (event->matches(QKeySequence::SaveAs)) {
    _canvas->saveProject();
  } else if (event->matches(QKeySequence::Save)) {
    _canvas->saveDocument();
  } else if (event->matches(QKeySequence::Open)) {
    _canvas->openFile();
  } else if (event->matches(QKeySequence::New)) {
    onNewCanvas();
  } else if (event->matches(QKeySequence::SelectAll)) {
    onEditSelectAll();
  } else if (event->key() == Qt::Key_J &&
             (event->modifiers() & Qt::ControlModifier) &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _canvas->extractColorSelectionToNewLayer();
  } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    // Enter commits an in-progress path gesture even when focus sits on a
    // panel widget rather than the canvas.
    if (!_canvas->finishActiveGesture()) {
      QMainWindow::keyPressEvent(event);
      return;
    }
    event->accept();
  } else if (event->key() == Qt::Key_Escape) {
    // Esc cancels the current interaction (path gesture, selection, overlays,
    // search); quitting is done via the Exit action (Ctrl+Q) instead.
    if (_canvas->cancelActiveGesture()) {
      event->accept();
      return;
    }
#ifdef HAVE_QT_PDF
    if (_pdfPanel && _pdfPanel->isVisible() && _pdfViewer) {
      _pdfViewer->closeSearch();
    }
#endif
    _canvas->deselectAll();
    event->accept();
  } else if (event->key() == Qt::Key_Delete ||
             event->key() == Qt::Key_Backspace) {
    onEditDelete();
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
#ifdef HAVE_QT_PDF
  // PDF search (Ctrl+F when PDF panel is visible); must precede the plain
  // tool keys below.
  else if (event->key() == Qt::Key_F && ctrl && _pdfPanel &&
           _pdfPanel->isVisible() && _pdfViewer) {
    _pdfViewer->openSearch();
  }
#endif
  // Tool shortcuts
  else if (event->key() == Qt::Key_P && !ctrl) {
    _toolPanel->onActionPen();
  } else if (event->key() == Qt::Key_I && !ctrl) {
    _toolPanel->onActionHighlighter();
  } else if (event->key() == Qt::Key_E && !ctrl &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _toolPanel->onActionPixelEraser();
  } else if (event->key() == Qt::Key_E && !ctrl) {
    _toolPanel->onActionEraser();
  } else if (event->key() == Qt::Key_T &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _toolPanel->onActionTextOnPath();
  } else if (event->key() == Qt::Key_T && !ctrl) {
    _toolPanel->onActionText();
  } else if (event->key() == Qt::Key_M && !ctrl) {
    _toolPanel->onActionMermaid();
  } else if (event->key() == Qt::Key_F && !ctrl) {
    _toolPanel->onActionFill();
  } else if (event->key() == Qt::Key_Q &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionColorSelect();
  } else if (event->key() == Qt::Key_L && !ctrl) {
    _toolPanel->onActionLine();
  } else if (event->key() == Qt::Key_A && !ctrl &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _toolPanel->onActionCurvedArrow();
  } else if (event->key() == Qt::Key_A && !ctrl) {
    _toolPanel->onActionArrow();
  } else if (event->key() == Qt::Key_R && !ctrl) {
    _toolPanel->onActionRectangle();
  } else if (event->key() == Qt::Key_C &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionCircle();
  } else if (event->key() == Qt::Key_S &&
             !(event->modifiers() & Qt::ControlModifier) &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _toolPanel->onActionLassoSelection();
  } else if ((event->key() == Qt::Key_S || event->key() == Qt::Key_V) &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionSelection();
  } else if (event->key() == Qt::Key_W &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionWire();
  } else if (event->key() == Qt::Key_K &&
             !(event->modifiers() & Qt::ControlModifier)) {
    _toolPanel->onActionColor();
  } else if (event->key() == Qt::Key_H && !ctrl) {
    _toolPanel->onActionPan();
  } else if (event->key() == Qt::Key_G && !ctrl) {
    _canvas->toggleGrid();
  } else if (event->key() == Qt::Key_B &&
             (event->modifiers() & Qt::ShiftModifier)) {
    _toolPanel->onActionBezier();
  } else if (event->key() == Qt::Key_B && !ctrl) {
    _canvas->toggleFilledShapes();
  } else if (event->key() == Qt::Key_D &&
             (event->modifiers() & Qt::ControlModifier)) {
    onEditDuplicate();
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
    onZoomIn();
  } else if (event->key() == Qt::Key_Minus) {
    onZoomOut();
  } else if (event->key() == Qt::Key_0) {
    onZoomReset();
  } else {
    QMainWindow::keyPressEvent(event);
  }
}

#ifdef HAVE_QT_PDF
void MainWindow::setupPdfViewer() {
  // Create PDF panel with its own layout (includes thumbnail rail + viewer)
  _pdfPanel = new QFrame(this);
  _pdfPanel->setObjectName("pdfPanel");
  _pdfPanel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  _pdfPanel->setMinimumWidth(200);
  _centralSplitter->setObjectName("workspaceSplitter");
  _centralSplitter->setHandleWidth(10);
  _canvas->setObjectName("canvasSurface");

  QHBoxLayout *pdfHLayout = new QHBoxLayout(_pdfPanel);
  pdfHLayout->setContentsMargins(0, 0, 0, 0);
  pdfHLayout->setSpacing(0);

  // Create PDF viewer widget first (needed for thumbnail panel)
  _pdfViewer = new PdfViewer(_pdfPanel);
  _pdfViewer->setObjectName("pdfViewerSurface");
  _pdfViewer->setUndoRedoManager(_undoRedoManager.get());

  // Create thumbnail panel (collapsible, on the left of PDF viewer)
  _thumbnailPanel = new PageThumbnailPanel(_pdfViewer, _pdfPanel);
  _thumbnailPanel->hide(); // Initially hidden, shown when PDF loads
  pdfHLayout->addWidget(_thumbnailPanel);

  // Create viewer container with header
  QWidget *viewerContainer = new QWidget(_pdfPanel);
  viewerContainer->setObjectName("pdfViewerContainer");
  QVBoxLayout *viewerLayout = new QVBoxLayout(viewerContainer);
  viewerLayout->setContentsMargins(0, 0, 0, 0);
  viewerLayout->setSpacing(0);

  // Create header label for PDF panel
  _pdfHeaderLabel = new QLabel("PDF Viewer", viewerContainer);
  _pdfHeaderLabel->setAlignment(Qt::AlignCenter);
  viewerLayout->addWidget(_pdfHeaderLabel);

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
          [this](const QPointF &pos) {
            _activeSurface = ActiveSurface::Pdf;
            onCursorPositionChanged(pos);
          });
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
  connect(_pdfViewer, &PdfViewer::documentModified, this,
          [this]() { _pdfDirty = true; });
  connect(_canvas, &Canvas::pdfFileDropped, this,
          &MainWindow::onPdfFileDropped);
}

void MainWindow::setupPdfToolBar() {
  _pdfToolBar = new QToolBar("PDF Navigation", this);
  _pdfToolBar->setObjectName("pdfToolBar");
  _pdfToolBar->setIconSize(QSize(22, 22));
  _pdfToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  addToolBar(Qt::TopToolBarArea, _pdfToolBar);
  _pdfToolBar->hide(); // Hidden until PDF is loaded

  // Helper: load an SVG icon and tint it for the current theme.
  const bool dark = ThemeManager::instance().isDarkTheme();
  const QColor tint = dark ? Qt::white : Qt::black;
  auto makeIcon = [&](const QString &path) -> QIcon {
    const int sz = 22;
    const qreal dpr = devicePixelRatioF();
    const int px = qRound(sz * dpr);
    QIcon raw(path);
    QPixmap pm = raw.pixmap(QSize(px, px));
    pm.setDevicePixelRatio(dpr);
    QPainter p(&pm);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pm.rect(), tint);
    p.end();
    return QIcon(pm);
  };

  // === CLUSTER 1: Document Operations ===
  _thumbnailToggleAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_thumbnails.svg"), "", [this]() {
        if (_thumbnailPanel) {
          _thumbnailPanel->toggleVisibility();
        }
      });
  _thumbnailToggleAction->setToolTip("Toggle Page Thumbnails");
  _thumbnailToggleAction->setCheckable(true);
  // Start in sync with the panel (hidden until a PDF is loaded) instead of
  // showing a checked button over a hidden panel.
  _thumbnailToggleAction->setChecked(_thumbnailPanel &&
                                     _thumbnailPanel->isPanelVisible());

  _pdfToolBar->addSeparator();

  QAction *openPdfAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_open.svg"), "", this, &MainWindow::onOpenPdf);
  openPdfAction->setToolTip("Open PDF (Ctrl+Shift+O)");

  QAction *exportAction =
      _pdfToolBar->addAction(makeIcon(":/ui-icons/pdf_save.svg"), "", this,
                             &MainWindow::onExportAnnotatedPdf);
  exportAction->setToolTip("Export Annotated PDF");

  QAction *closeAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_close.svg"), "", this, &MainWindow::onClosePdf);
  closeAction->setToolTip("Close PDF");

  _pdfToolBar->addSeparator();

  // === CLUSTER 2: Navigation ===
  QAction *firstAction =
      _pdfToolBar->addAction(makeIcon(":/ui-icons/pdf_first_page.svg"), "",
                             _pdfViewer, &PdfViewer::firstPage);
  firstAction->setToolTip("First Page (Home)");

  QAction *prevAction =
      _pdfToolBar->addAction(makeIcon(":/ui-icons/pdf_prev_page.svg"), "",
                             _pdfViewer, &PdfViewer::previousPage);
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

  _pdfPageLabel = new QLabel(" / 0", this);
  _pdfPageLabel->setStyleSheet("QLabel { color: #a0a0a8; padding: 0 8px; }");
  _pdfToolBar->addWidget(_pdfPageLabel);

  QAction *nextAction =
      _pdfToolBar->addAction(makeIcon(":/ui-icons/pdf_next_page.svg"), "",
                             _pdfViewer, &PdfViewer::nextPage);
  nextAction->setToolTip("Next Page (Page Down)");

  QAction *lastAction =
      _pdfToolBar->addAction(makeIcon(":/ui-icons/pdf_last_page.svg"), "",
                             _pdfViewer, &PdfViewer::lastPage);
  lastAction->setToolTip("Last Page (End)");

  _pdfToolBar->addSeparator();

  // === CLUSTER 3: View/Edit Controls ===
  QAction *zoomOutAction =
      _pdfToolBar->addAction(makeIcon(":/ui-icons/pdf_zoom_out.svg"), "",
                             _pdfViewer, &PdfViewer::zoomOut);
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
      image: url(:/ui-icons/arrow_down_dark.png);
      width: 10px;
      height: 6px;
      margin-right: 5px;
    }
    QComboBox QAbstractItemView {
      background-color: #2a2a30;
      color: #f8f8fc;
      selection-background-color: #3b82f6;
    }
  )");
  // activated (not currentIndexChanged): re-picking the preset that is
  // still "current" after Ctrl+wheel / fit zooming must apply it again.
  connect(_pdfZoomCombo, QOverload<int>::of(&QComboBox::activated), this,
          &MainWindow::onPdfZoomComboChanged);
  // Typed values are applied on Enter; don't pile them up as new presets.
  _pdfZoomCombo->setInsertPolicy(QComboBox::NoInsert);
  connect(_pdfZoomCombo->lineEdit(), &QLineEdit::returnPressed, this, [this]() {
    QString text = _pdfZoomCombo->currentText().remove('%');
    bool ok;
    int zoom = text.toInt(&ok);
    if (ok && zoom >= 10 && zoom <= 500 && _pdfViewer) {
      _pdfViewer->setZoomPercent(zoom);
    }
  });
  _pdfToolBar->addWidget(_pdfZoomCombo);

  QAction *zoomInAction =
      _pdfToolBar->addAction(makeIcon(":/ui-icons/pdf_zoom_in.svg"), "",
                             _pdfViewer, &PdfViewer::zoomIn);
  zoomInAction->setToolTip("Zoom In (+)");

  _pdfToolBar->addSeparator();

  // Fit options
  QAction *fitWidthAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_fit_width.svg"), "", this, [this]() {
        if (_pdfViewer && _pdfViewer->hasPdf()) {
          _pdfViewer->fitToWidth();
        }
      });
  fitWidthAction->setToolTip("Fit to Width");

  QAction *fitPageAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_fit_page.svg"), "", this, [this]() {
        if (_pdfViewer && _pdfViewer->hasPdf()) {
          _pdfViewer->fitToPage();
        }
      });
  fitPageAction->setToolTip("Fit to Page");

  _pdfToolBar->addSeparator();

  // Rotation
  QAction *rotateLeftAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_rotate_left.svg"), "", this, [this]() {
        if (_pdfViewer && _pdfViewer->hasPdf()) {
          _pdfViewer->rotatePageLeft();
        }
      });
  rotateLeftAction->setToolTip("Rotate Left (90° CCW)");

  QAction *rotateRightAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_rotate_right.svg"), "", this, [this]() {
        if (_pdfViewer && _pdfViewer->hasPdf()) {
          _pdfViewer->rotatePageRight();
        }
      });
  rotateRightAction->setToolTip("Rotate Right (90° CW)");

  _pdfToolBar->addSeparator();

  // Dark mode toggle
  _pdfDarkModeAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_dark_mode.svg"), "",
      [this]() { _pdfViewer->setDarkMode(!_pdfViewer->darkMode()); });
  _pdfDarkModeAction->setToolTip("Toggle Dark Mode");
  _pdfDarkModeAction->setCheckable(true);

  _pdfToolBar->addSeparator();

  // Mode toggle: View vs Annotate
  _pdfModeAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_annotate.svg"), "Annotate", [this]() {
        if (_pdfViewer) {
          if (_pdfViewer->isAnnotateMode()) {
            _pdfViewer->setMode(PdfViewer::Mode::View);
          } else {
            _pdfViewer->setMode(PdfViewer::Mode::Annotate);
          }
        }
      });
  _pdfModeAction->setToolTip("Toggle View/Annotate mode");
  _pdfModeAction->setCheckable(true);
  _pdfModeAction->setChecked(true);
  // The mode switch is the most important control on this bar: put it
  // first (after the page-list toggle) so a narrow window pushes the
  // rarely used buttons into the overflow menu, not this one.
  if (_pdfToolBar->actions().size() > 1) {
    QAction *first = _pdfToolBar->actions().at(1);
    _pdfToolBar->removeAction(_pdfModeAction);
    _pdfToolBar->insertAction(first, _pdfModeAction);
  }
  // Show label beside icon for this button.
  if (auto *btn = qobject_cast<QToolButton *>(
          _pdfToolBar->widgetForAction(_pdfModeAction))) {
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // The style sheet's min-width would override setMinimumWidth(); name
    // the button so the sheet can give its label room ("An...te").
    btn->setObjectName(QStringLiteral("pdfModeButton"));
  }

  // Connect mode change to update button
  connect(_pdfViewer, &PdfViewer::modeChanged, this,
          [this](PdfViewer::Mode mode) {
            if (mode == PdfViewer::Mode::Annotate) {
              _pdfModeAction->setText("Annotate");
              _pdfModeAction->setChecked(true);
            } else {
              _pdfModeAction->setText("View");
              _pdfModeAction->setChecked(false);
            }
          });

  _pdfToolBar->addSeparator();

  // Screenshot tool
  QAction *screenshotAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_screenshot.svg"), "",
      [this]() { _pdfViewer->setScreenshotSelectionMode(true); });
  screenshotAction->setToolTip("Screenshot Selection");

  _pdfToolBar->addSeparator();

  // Undo/Redo
  QAction *undoAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_undo.svg"), "", this, &MainWindow::performUndo);
  undoAction->setToolTip("Undo (Ctrl+Z)");

  QAction *redoAction = _pdfToolBar->addAction(
      makeIcon(":/ui-icons/pdf_redo.svg"), "", this, &MainWindow::performRedo);
  redoAction->setToolTip("Redo (Ctrl+Y)");

  // Style the toolbar
  applyTheme();
}

void MainWindow::onPdfPageSpinBoxChanged(int page) {
  if (_pdfViewer && _pdfViewer->hasPdf()) {
    _pdfViewer->goToPage(page - 1); // Convert 1-based to 0-based
  }
}

void MainWindow::onPdfZoomComboChanged(int index) {
  if (!_pdfViewer || index < 0)
    return;
  bool ok = false;
  const int zoom = _pdfZoomCombo->itemText(index).remove('%').toInt(&ok);
  if (ok)
    _pdfViewer->setZoomPercent(zoom);
}

void MainWindow::updatePdfZoomCombo(double zoomPercent) {
  if (_pdfZoomCombo) {
    _pdfZoomCombo->blockSignals(true);
    _pdfZoomCombo->setCurrentText(QString("%1%").arg(qRound(zoomPercent)));
    _pdfZoomCombo->blockSignals(false);
  }
}

void MainWindow::showPdfPanel() {
  // Split ratio: the PDF panel also hosts the page-thumbnail strip, so it
  // needs the larger share for the page itself to stay readable.
  static constexpr double CANVAS_RATIO = 0.35;
  static constexpr double PDF_RATIO = 0.65;

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
    // The canvas defaults to white ink on black; on a light PDF page that
    // ink would be invisible, so fall back to a colour that shows up.
    QColor penColor = _canvas->getCurrentColor();
    if (!_pdfViewer->darkMode() && penColor.lightnessF() > 0.9) {
      penColor = QColor(220, 38, 38);
    }
    _pdfViewer->setOpacity(_canvas->getCurrentOpacity());
    _pdfViewer->setPenColor(penColor);
    _pdfViewer->setPenWidth(_canvas->getCurrentBrushSize());
    _pdfViewer->setFilledShapes(_canvas->isFilledShapes());
  }

  _pdfToolBar->show();
  _pdfModeTitle = true;
  updateWindowTitle();
  if (_exportAnnotatedPdfAction)
    _exportAnnotatedPdfAction->setEnabled(true);
  if (_closePdfAction)
    _closePdfAction->setEnabled(true);
}

void MainWindow::hidePdfPanel() {
  if (_pdfPanel) {
    _pdfPanel->hide();
    // Give full width back to canvas (use 1:0 ratio)
    _centralSplitter->setSizes({1, 0});
  }
  _pdfToolBar->hide();
  _pdfModeTitle = false;
  updateWindowTitle();
  if (_exportAnnotatedPdfAction)
    _exportAnnotatedPdfAction->setEnabled(false);
  if (_closePdfAction)
    _closePdfAction->setEnabled(false);
}

void MainWindow::onOpenPdf() {
  QString fileName = FileDialogs::getOpen(this, "Open PDF File", "",
                                          "PDF Files (*.pdf);;All Files (*)");
  if (fileName.isEmpty()) {
    return;
  }
  if (!maybeDiscardPdfAnnotations()) {
    return;
  }

  if (_pdfViewer->openPdf(fileName)) {
    _pdfDirty = false;
  } else {
    statusBar()->showMessage("Failed to open PDF file", 3000);
  }
}

void MainWindow::onClosePdf() {
  if (!maybeDiscardPdfAnnotations()) {
    return;
  }
  if (_pdfViewer) {
    _pdfViewer->closePdf();
  }
  _pdfDirty = false;
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
  if (!maybeDiscardPdfAnnotations()) {
    return;
  }
  if (_pdfViewer && _pdfViewer->openPdf(filePath)) {
    _pdfDirty = false;
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
  _pdfViewer->commitPendingEdits(); // include what is still being typed

  QString fileName = FileDialogs::getSave(this, "Export Annotated PDF", "",
                                          "PDF Files (*.pdf)");
  if (fileName.isEmpty()) {
    return;
  }

  if (_pdfViewer->exportAnnotatedPdf(fileName)) {
    _pdfDirty = false;
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
