/**
 * @file pdf_viewer.cpp
 * @brief Implementation of PDF viewing widget with annotation support.
 */
#include "pdf_viewer.h"

#ifdef HAVE_QT_PDF

#include "../tools/tool.h"
#include "../tools/tool_manager.h"
#include "item_painting.h"
#include "pdf_search_bar.h"
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPdfSearchModel>
#include <QPdfWriter>
#include <QPointer>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyleOptionGraphicsItem>
#include <QUrl>
#include <QWheelEvent>
#include <cmath>

// --- PdfPageItem Implementation ---

PdfPageItem::PdfPageItem(QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent), inverted_(false) {
  setFlag(QGraphicsItem::ItemIsSelectable, false);
  setFlag(QGraphicsItem::ItemIsMovable, false);
  setZValue(-1000); // Behind all other items
  setTransformationMode(Qt::SmoothTransformation);
}

void PdfPageItem::setPageImage(const QImage &image) {
  originalImage_ = image;
  updatePixmap();
}

void PdfPageItem::setInverted(bool inverted) {
  if (inverted_ != inverted) {
    inverted_ = inverted;
    updatePixmap();
  }
}

void PdfPageItem::updatePixmap() {
  if (originalImage_.isNull()) {
    setPixmap(QPixmap());
    return;
  }

  if (inverted_) {
    QImage inverted = originalImage_.copy();
    inverted.invertPixels(QImage::InvertRgb);
    setPixmap(QPixmap::fromImage(inverted));
  } else {
    setPixmap(QPixmap::fromImage(originalImage_));
  }
}

// --- PdfViewer Implementation ---

PdfViewer::PdfViewer(QWidget *parent)
    : QGraphicsView(parent), document_(std::make_unique<PdfDocument>()),
      overlayManager_(std::make_unique<PdfOverlayManager>()),
      pageItem_(nullptr), scene_(new QGraphicsScene(this)),
      sceneController_(nullptr), toolManager_(nullptr),
      specialTool_(SpecialTool::None), currentPage_(0), renderDpi_(DEFAULT_DPI),
      pageRotation_(0), mode_(Mode::Annotate), darkMode_(false),
      showGrid_(false), fillShapes_(false), currentZoom_(1.0),
      currentPen_(Qt::white, 3), eraserPen_(Qt::black, 10),
      screenshotSelectionRect_(nullptr), searchModel_(nullptr),
      searchBar_(nullptr), currentMatchIndex_(-1), totalMatchCount_(0),
      highlightedMatchPage_(-1), highlightedMatchIndexOnPage_(-1) {

  setupScene();

  // Initialize scene controller (single source of truth)
  sceneController_ = new SceneController(scene_, this);
  overlayManager_->setItemStore(sceneController_->itemStore());

  // Initialize tool manager
  toolManager_ = new ToolManager(this, this);
  toolManager_->setActiveTool(ToolManager::ToolType::Pen);

  // Connect document signals
  connect(document_.get(), &PdfDocument::documentLoaded, this, [this]() {
    overlayManager_->initialize(document_->pageCount());
    goToPage(0);
    emit pdfLoaded();
  });

  connect(document_.get(), &PdfDocument::errorOccurred, this,
          &PdfViewer::errorOccurred);

  // Initialize search model
  searchModel_ = new QPdfSearchModel(this);
  connect(document_.get(), &PdfDocument::documentLoaded, this,
          [this]() { searchModel_->setDocument(document_->document()); });
  // QPdfSearchModel fills in results asynchronously (in timer-driven
  // batches), so react to model updates rather than reading rowCount() once.
  connect(searchModel_, &QAbstractItemModel::rowsInserted, this,
          &PdfViewer::onSearchResultsChanged);
  connect(searchModel_, &QAbstractItemModel::modelReset, this,
          &PdfViewer::onSearchResultsChanged);

  // Initialize search bar
  searchBar_ = new PdfSearchBar(viewport());
  connect(searchBar_, &PdfSearchBar::searchTextChanged, this,
          &PdfViewer::performSearch);
  connect(searchBar_, &PdfSearchBar::findNext, this, &PdfViewer::findNext);
  connect(searchBar_, &PdfSearchBar::findPrevious, this,
          &PdfViewer::findPrevious);
  connect(searchBar_, &PdfSearchBar::closed, this, &PdfViewer::closeSearch);
}

PdfViewer::~PdfViewer() {
  // Child QObjects die in creation order, and the scene was created before
  // the tool manager: tear the tools down first, or an unfinished Bezier /
  // text-on-path gesture would delete its preview items a second time.
  if (toolManager_) {
    if (Tool *tool = toolManager_->activeTool())
      tool->cancelGesture();
    delete toolManager_;
    toolManager_ = nullptr;
  }
}

void PdfViewer::setupScene() {
  setScene(scene_);
  setRenderHint(QPainter::Antialiasing);
  setRenderHint(QPainter::SmoothPixmapTransform);
  setRenderHint(QPainter::TextAntialiasing);
  setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setResizeAnchor(QGraphicsView::AnchorUnderMouse);
  setCacheMode(QGraphicsView::CacheBackground);
  setDragMode(QGraphicsView::NoDrag);
  setMouseTracking(true);
  setAcceptDrops(true);

  // Set initial background color based on dark mode setting
  if (darkMode_) {
    scene_->setBackgroundBrush(QColor(50, 50, 50)); // Dark gray background
  } else {
    scene_->setBackgroundBrush(QColor(240, 240, 240)); // Light gray background
  }

  currentPen_.setCapStyle(Qt::RoundCap);
  currentPen_.setJoinStyle(Qt::RoundJoin);
}

bool PdfViewer::openPdf(const QString &filePath) {
  closePdf();
  return document_->load(filePath);
}

void PdfViewer::closePdf() {
  if (toolManager_) {
    // Discard – never commit – an in-progress gesture: the document it would
    // be drawn into is going away.
    if (Tool *tool = toolManager_->activeTool()) {
      tool->cancelGesture();
    }
    actionGroup_.reset(); // actions for items that are about to go away
    // Reset the tool's state but keep the one the user picked (the tool
    // panel still shows it; forcing Pen made the next PDF draw freehand).
    toolManager_->setActiveTool(toolManager_->activeToolType());
  }
  // Cancel any in-progress special tool gesture; the scene clear below would
  // otherwise delete the rubber-band item out from under the mouse handlers.
  specialTool_ = SpecialTool::None;
  screenshotSelectionRect_ = nullptr;

  // Remove page item
  if (pageItem_) {
    scene_->removeItem(pageItem_);
    delete pageItem_;
    pageItem_ = nullptr;
  }

  // Clear overlays and scene
  overlayManager_->clear();
  if (sceneController_) {
    sceneController_->clearAll();
  } else {
    scene_->clear();
  }

  document_->close();
  currentPage_ = 0;
  currentZoom_ = 1.0;
  pageRotation_ = 0; // never carry a rotation over to the next document
  resetTransform();
  // Drop search state bound to the old document so stale highlights can't
  // flash against the geometry of a newly opened document.
  closeSearch();
  if (undoRedoManager_) {
    // The history is shared with the canvas: drop only the PDF's actions.
    undoRedoManager_->clearOwnedBy(this);
  }

  emit pdfClosed();
}

bool PdfViewer::hasPdf() const {
  return document_->status() == PdfDocument::Status::Ready;
}

int PdfViewer::pageCount() const { return document_->pageCount(); }

void PdfViewer::goToPage(int pageIndex) {
  if (!hasPdf()) {
    return;
  }

  if (pageIndex < 0 || pageIndex >= pageCount()) {
    return;
  }

  if (pageIndex != currentPage_ && !navigatingForHistory_) {
    // Commit an unfinished gesture while currentPage_ still names the page
    // it was drawn on; otherwise it would stay on screen and be filed under
    // the new page. (Not during undo/redo: history is committed up front.)
    commitActiveGesture();
  }

  currentPage_ = pageIndex;
  renderCurrentPage();

  // Show only the current page's overlay
  overlayManager_->showPage(currentPage_);

  emit pageChanged(currentPage_, pageCount());
  updateSearchHighlights();
}

void PdfViewer::nextPage() {
  if (currentPage_ < pageCount() - 1) {
    goToPage(currentPage_ + 1);
  }
}

void PdfViewer::previousPage() {
  if (currentPage_ > 0) {
    goToPage(currentPage_ - 1);
  }
}

void PdfViewer::firstPage() { goToPage(0); }

void PdfViewer::lastPage() { goToPage(pageCount() - 1); }

void PdfViewer::renderCurrentPage() {
  if (!hasPdf()) {
    return;
  }

  const int effectiveDpi = effectiveRenderDpi();
  QImage pageImage = document_->renderPage(currentPage_, effectiveDpi, false);
  if (pageImage.isNull()) {
    emit errorOccurred(tr("Failed to render page %1").arg(currentPage_ + 1));
    return;
  }

  // Create or update page item
  if (!pageItem_) {
    pageItem_ = new PdfPageItem();
    scene_->addItem(pageItem_);
  }

  pageItem_->setPageImage(pageImage);
  pageItem_->setInverted(darkMode_);
  pageItem_->setScale(static_cast<qreal>(renderDpi_) /
                      static_cast<qreal>(effectiveDpi));

  // Keep the scene in base-DPI coordinates so overlay items stay aligned
  // while the page pixmap can be rerendered at higher DPI for zoomed-in views.
  const qreal pageScale = pageItem_->scale();
  scene_->setSceneRect(0, 0, pageImage.width() * pageScale,
                       pageImage.height() * pageScale);
}

void PdfViewer::setToolType(ToolManager::ToolType toolType) {
  specialTool_ = SpecialTool::None;

  // Set via tool manager
  toolManager_->setActiveTool(toolType);
  updateDragMode();

  // Update cursor based on active tool
  Tool *tool = toolManager_->activeTool();
  if (tool) {
    QGraphicsView::setCursor(tool->cursor());
  }

  scene_->clearSelection();
}

void PdfViewer::updateDragMode() {
  // One place decides dragging from mode + special tool + active tool, so
  // switching any of them can't leave a stale mode (e.g. View mode that no
  // longer pans, or a Select tool without rubber-band selection).
  if (mode_ == Mode::View) {
    setDragMode(QGraphicsView::ScrollHandDrag);
  } else if (specialTool_ != SpecialTool::None) {
    setDragMode(QGraphicsView::NoDrag);
  } else if (toolManager_ && toolManager_->activeToolType() ==
                                 ToolManager::ToolType::Selection) {
    setDragMode(QGraphicsView::RubberBandDrag);
  } else {
    setDragMode(QGraphicsView::NoDrag);
  }
  // In View mode clicks must pan, never grab and move annotations.
  setInteractive(mode_ != Mode::View);
}

void PdfViewer::setScreenshotSelectionMode(bool enabled) {
  if (enabled) {
    specialTool_ = SpecialTool::ScreenshotSelection;
    QGraphicsView::setCursor(Qt::CrossCursor);
    updateDragMode();
  } else {
    specialTool_ = SpecialTool::None;
    // Leaving mid-drag (Escape) must not strand the selection rectangle.
    if (screenshotSelectionRect_) {
      if (screenshotSelectionRect_->scene())
        scene_->removeItem(screenshotSelectionRect_);
      delete screenshotSelectionRect_;
      screenshotSelectionRect_ = nullptr;
    }
    // Restore cursor (and rubber-band dragging) from the current tool
    Tool *tool = toolManager_->activeTool();
    if (tool) {
      QGraphicsView::setCursor(tool->cursor());
    }
    updateDragMode();
  }
}

void PdfViewer::setPenColor(const QColor &color) {
  QColor penColor = color;
  penColor.setAlpha(currentOpacity_);
  currentPen_.setColor(penColor);
}

void PdfViewer::setOpacity(int opacity) {
  currentOpacity_ = qBound(0, opacity, 255);
  QColor penColor = currentPen_.color();
  penColor.setAlpha(currentOpacity_);
  currentPen_.setColor(penColor);
}

void PdfViewer::setPenWidth(int width) {
  currentPen_.setWidth(width);
  eraserPen_.setWidth(width * 2);
}

void PdfViewer::setUndoRedoManager(UndoRedoManager *manager) {
  undoRedoManager_ = manager;
  // Release parked undo snapshots when actions are discarded (see Canvas).
  if (manager) {
    QPointer<ItemStore> storeGuard = itemStore();
    manager->addDiscardListener([storeGuard](const ItemId &id) {
      if (storeGuard && !storeGuard->contains(id)) {
        storeGuard->discardSnapshot(id);
      }
    });
  }
}

void PdfViewer::setFilledShapes(bool filled) { fillShapes_ = filled; }

void PdfViewer::setDarkMode(bool enabled) {
  if (darkMode_ != enabled) {
    darkMode_ = enabled;
    if (pageItem_) {
      pageItem_->setInverted(darkMode_);
    }
    // Update scene background based on dark mode
    if (darkMode_) {
      scene_->setBackgroundBrush(QColor(50, 50, 50)); // Dark gray background
    } else {
      scene_->setBackgroundBrush(
          QColor(240, 240, 240)); // Light gray background
    }
    viewport()->update();
    emit darkModeChanged(darkMode_);
  }
}

void PdfViewer::setRenderDpi(int dpi) {
  if (renderDpi_ != dpi) {
    renderDpi_ = dpi;
    if (hasPdf()) {
      document_->clearCache();
      renderCurrentPage();
    }
  }
}

void PdfViewer::zoomIn() { applyZoom(ZOOM_FACTOR); }

void PdfViewer::zoomOut() { applyZoom(1.0 / ZOOM_FACTOR); }

void PdfViewer::applyViewTransform() {
  // Rotation lives in the view transform, not in the page bitmap, so the
  // page and its annotations (both in scene coordinates) rotate together
  // and search highlights / export stay in unrotated page space.
  resetTransform();
  rotate(pageRotation_);
  scale(currentZoom_, currentZoom_);
}

void PdfViewer::zoomReset() {
  currentZoom_ = 1.0;
  renderCurrentPage();
  applyViewTransform();
  emit zoomChanged(100.0);
}

void PdfViewer::setZoomPercent(double zoomPercent) {
  const double zoomFactor = zoomPercent / 100.0;
  if (zoomFactor < MIN_ZOOM || zoomFactor > MAX_ZOOM) {
    return;
  }

  currentZoom_ = zoomFactor;
  renderCurrentPage();
  applyViewTransform();
  emit zoomChanged(currentZoom_ * 100.0);
}

void PdfViewer::fitToWidth() {
  if (!hasPdf() || !pageItem_) {
    return;
  }

  QRectF pageRect = pageItem_->sceneBoundingRect();
  if (pageRect.isEmpty()) {
    return;
  }

  // Calculate scale to fit width (a quarter turn swaps the page's sides)
  const bool quarterTurn = pageRotation_ % 180 != 0;
  double viewWidth = viewport()->width() - 20; // Margin
  double scale =
      viewWidth / (quarterTurn ? pageRect.height() : pageRect.width());

  currentZoom_ = scale;
  renderCurrentPage();
  applyViewTransform();
  emit zoomChanged(currentZoom_ * 100.0);
}

void PdfViewer::fitToPage() {
  if (!hasPdf() || !pageItem_) {
    return;
  }

  QRectF pageRect = pageItem_->sceneBoundingRect();
  if (pageRect.isEmpty()) {
    return;
  }

  // Calculate scale to fit entire page
  double viewWidth = viewport()->width() - 20;
  double viewHeight = viewport()->height() - 20;
  const bool quarterTurn = pageRotation_ % 180 != 0;
  double scaleX =
      viewWidth / (quarterTurn ? pageRect.height() : pageRect.width());
  double scaleY =
      viewHeight / (quarterTurn ? pageRect.width() : pageRect.height());
  double scale = qMin(scaleX, scaleY);

  currentZoom_ = scale;
  renderCurrentPage();
  applyViewTransform();
  emit zoomChanged(currentZoom_ * 100.0);
}

void PdfViewer::rotatePageLeft() {
  pageRotation_ = (pageRotation_ - 90 + 360) % 360;
  applyViewTransform();
}

void PdfViewer::rotatePageRight() {
  pageRotation_ = (pageRotation_ + 90) % 360;
  applyViewTransform();
}

void PdfViewer::setMode(Mode mode) {
  if (mode_ != mode) {
    mode_ = mode;

    if (mode_ == Mode::View) {
      // View mode: disable drawing, set cursor to arrow
      QGraphicsView::setCursor(Qt::ArrowCursor);
    } else {
      // Annotate mode: restore tool cursor
      Tool *tool = toolManager_->activeTool();
      if (tool) {
        QGraphicsView::setCursor(tool->cursor());
      }
    }
    updateDragMode();

    emit modeChanged(mode_);
  }
}

void PdfViewer::applyZoom(double factor) {
  double newZoom = currentZoom_ * factor;
  if (newZoom > MAX_ZOOM || newZoom < MIN_ZOOM) {
    return;
  }
  currentZoom_ = newZoom;
  renderCurrentPage();
  scale(factor, factor);
  emit zoomChanged(currentZoom_ * 100.0);
}

int PdfViewer::effectiveRenderDpi() const {
  const double zoomMultiplier = qMax(1.0, currentZoom_);
  const int dpi = static_cast<int>(std::ceil(renderDpi_ * zoomMultiplier));
  return qBound(renderDpi_, dpi, MAX_RENDER_DPI);
}

void PdfViewer::toggleGrid() {
  showGrid_ = !showGrid_;
  viewport()->update();
  scene_->invalidate(scene_->sceneRect(), QGraphicsScene::BackgroundLayer);
}

void PdfViewer::undo() {
  if (undoRedoManager_) {
    undoRedoManager_->undo();
    return;
  }

  if (!hasPdf()) {
    return;
  }

  auto *undoStack = overlayManager_->undoStack(currentPage_);
  auto *redoStack = overlayManager_->redoStack(currentPage_);

  if (undoStack && redoStack && !undoStack->empty()) {
    std::unique_ptr<Action> action = std::move(undoStack->back());
    undoStack->pop_back();
    action->undo();
    redoStack->push_back(std::move(action));
  }
}

void PdfViewer::redo() {
  if (undoRedoManager_) {
    undoRedoManager_->redo();
    return;
  }

  if (!hasPdf()) {
    return;
  }

  auto *undoStack = overlayManager_->undoStack(currentPage_);
  auto *redoStack = overlayManager_->redoStack(currentPage_);

  if (undoStack && redoStack && !redoStack->empty()) {
    std::unique_ptr<Action> action = std::move(redoStack->back());
    redoStack->pop_back();
    action->redo();
    undoStack->push_back(std::move(action));
  }
}

bool PdfViewer::canUndo() const {
  if (undoRedoManager_) {
    return undoRedoManager_->canUndo();
  }
  return hasPdf() && overlayManager_->canUndo(currentPage_);
}

bool PdfViewer::canRedo() const {
  if (undoRedoManager_) {
    return undoRedoManager_->canRedo();
  }
  return hasPdf() && overlayManager_->canRedo(currentPage_);
}

void PdfViewer::addDrawAction(QGraphicsItem *item) {
  if (!hasPdf()) {
    return;
  }

  ItemStore *store = itemStore();
  ItemId itemId;
  if (store && item) {
    itemId = store->idForItem(item);
    if (!itemId.isValid()) {
      itemId = registerItem(item);
    }
  }

  int pageIndex = currentPage_;
  auto onAdd = [this, pageIndex](QGraphicsItem *added) {
    if (!overlayManager_ || !added)
      return;
    // Undo/redo of an edit on another page: show that page, or the change
    // happens out of sight and looks like undo did nothing.
    if (pageIndex != currentPage_) {
      const QScopedValueRollback<bool> history(navigatingForHistory_, true);
      goToPage(pageIndex);
    }
    if (overlayManager_->overlay(pageIndex)) {
      overlayManager_->addItemToPage(pageIndex, added);
    }
  };
  auto onRemove = [this, pageIndex](QGraphicsItem *removed) {
    if (!overlayManager_ || !removed)
      return;
    if (pageIndex != currentPage_) {
      const QScopedValueRollback<bool> history(navigatingForHistory_, true);
      goToPage(pageIndex);
    }
    overlayManager_->removeItemFromPage(pageIndex, removed);
  };

  if (store && itemId.isValid()) {
    addAction(std::make_unique<DrawAction>(itemId, store, onAdd, onRemove));
  } else {
    qWarning() << "Cannot create DrawAction without ItemStore";
  }
  overlayManager_->addItemToPage(currentPage_, item);
  emit documentModified();
}

void PdfViewer::deleteSelectedItems() {
  if (!hasPdf() || !scene_)
    return;
  const QList<QGraphicsItem *> selected = scene_->selectedItems();
  for (QGraphicsItem *item : selected) {
    if (!item || item->parentItem() || item == pageItem_ ||
        item == screenshotSelectionRect_)
      continue;
    addDeleteAction(item);
    if (sceneController_) {
      sceneController_->removeItem(item, true); // keep for undo
    } else {
      scene_->removeItem(item);
      onItemRemoved(item);
    }
  }
}

void PdfViewer::selectAll() {
  if (!hasPdf() || !overlayManager_)
    return;
  scene_->clearSelection();
  if (PdfPageOverlay *overlay = overlayManager_->overlay(currentPage_)) {
    for (QGraphicsItem *item : overlay->items()) {
      if (item)
        item->setSelected(true);
    }
  }
}

void PdfViewer::addDeleteAction(QGraphicsItem *item) {
  if (!hasPdf()) {
    return;
  }

  ItemStore *store = itemStore();
  ItemId itemId;
  if (store && item) {
    itemId = store->idForItem(item);
    if (!itemId.isValid()) {
      itemId = registerItem(item);
    }
  }

  int pageIndex = -1;
  if (overlayManager_) {
    pageIndex = overlayManager_->findPageForItem(item);
  }
  if (pageIndex < 0) {
    pageIndex = currentPage_;
  }

  auto onAdd = [this, pageIndex](QGraphicsItem *added) {
    if (!overlayManager_ || !added)
      return;
    // Undo/redo of an edit on another page: show that page, or the change
    // happens out of sight and looks like undo did nothing.
    if (pageIndex != currentPage_) {
      const QScopedValueRollback<bool> history(navigatingForHistory_, true);
      goToPage(pageIndex);
    }
    if (overlayManager_->overlay(pageIndex)) {
      overlayManager_->addItemToPage(pageIndex, added);
    }
  };
  auto onRemove = [this, pageIndex](QGraphicsItem *removed) {
    if (!overlayManager_ || !removed)
      return;
    if (pageIndex != currentPage_) {
      const QScopedValueRollback<bool> history(navigatingForHistory_, true);
      goToPage(pageIndex);
    }
    overlayManager_->removeItemFromPage(pageIndex, removed);
  };

  if (store && itemId.isValid()) {
    addAction(std::make_unique<DeleteAction>(itemId, store, onAdd, onRemove));
  } else {
    qWarning() << "Cannot create DeleteAction without ItemStore";
  }
}

void PdfViewer::commitActiveGesture() {
  if (!toolManager_)
    return;
  if (Tool *tool = toolManager_->activeTool()) {
    tool->deactivate();
    tool->activate();
  }
}

void PdfViewer::beginActionGroup() {
  if (!actionGroup_)
    actionGroup_ = std::make_unique<CompositeAction>();
}

void PdfViewer::endActionGroup() {
  if (!actionGroup_)
    return;
  std::unique_ptr<CompositeAction> group = std::move(actionGroup_);
  if (!group->isEmpty())
    addAction(std::move(group));
}

void PdfViewer::addAction(std::unique_ptr<Action> action) {
  if (!hasPdf()) {
    return;
  }
  if (actionGroup_) {
    actionGroup_->addAction(std::move(action));
    emit documentModified();
    return;
  }

  if (undoRedoManager_) {
    undoRedoManager_->push(std::move(action), this);
  } else {
    if (auto *undoStack = overlayManager_->undoStack(currentPage_)) {
      undoStack->push_back(std::move(action));
    }
    clearRedoStack();
  }
  emit documentModified();
}

void PdfViewer::onItemRemoved(QGraphicsItem *item) {
  if (!overlayManager_ || !item)
    return;
  int pageIndex = overlayManager_->findPageForItem(item);
  if (pageIndex >= 0) {
    overlayManager_->removeItemFromPage(pageIndex, item);
  }
}

void PdfViewer::clearRedoStack() {
  if (!undoRedoManager_ && hasPdf()) {
    if (auto *redoStack = overlayManager_->redoStack(currentPage_)) {
      redoStack->clear();
    }
  }
}

bool PdfViewer::exportAnnotatedPdf(const QString &filePath) {
  if (!hasPdf()) {
    return false;
  }

  QPdfWriter pdfWriter(filePath);
  const QSizeF firstPagePoints = document_->pageSize(0);
  pdfWriter.setPageSize(firstPagePoints.isEmpty()
                            ? QPageSize(QPageSize::A4)
                            : QPageSize(firstPagePoints, QPageSize::Point));
  pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));
  pdfWriter.setTitle("Annotated PDF Export");
  pdfWriter.setCreator("FullScreen Pencil Draw");
  pdfWriter.setResolution(renderDpi_);

  QPainter painter(&pdfWriter);
  if (!painter.isActive()) {
    // Failed to open the target file (bad path / permissions)
    return false;
  }
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.setRenderHint(QPainter::TextAntialiasing);

  int savedPage = currentPage_;

  for (int i = 0; i < pageCount(); ++i) {
    if (i > 0) {
      // Keep each page's own size/orientation instead of forcing A4.
      const QSizeF pagePoints = document_->pageSize(i);
      if (!pagePoints.isEmpty()) {
        pdfWriter.setPageSize(QPageSize(pagePoints, QPageSize::Point));
      }
      pdfWriter.newPage();
    }

    // Render the page itself; dark mode is only a viewing aid and must not
    // end up inverted in the exported document.
    QImage pageImage = document_->renderPage(i, renderDpi_, false);
    if (pageImage.isNull()) {
      continue;
    }

    // Calculate scale to fit page. Use the writer's current layout: the
    // painter's viewport keeps the first page's size after a page-size
    // change.
    const QRectF pageRect =
        pdfWriter.pageLayout().paintRectPixels(pdfWriter.resolution());
    double scaleX = pageRect.width() / static_cast<double>(pageImage.width());
    double scaleY = pageRect.height() / static_cast<double>(pageImage.height());
    double scale = qMin(scaleX, scaleY);

    // Center content
    double offsetX = (pageRect.width() - pageImage.width() * scale) / 2.0;
    double offsetY = (pageRect.height() - pageImage.height() * scale) / 2.0;

    painter.save();
    painter.translate(offsetX, offsetY);
    painter.scale(scale, scale);

    // Draw PDF background
    painter.drawImage(0, 0, pageImage);

    // Draw overlay items for this page (children included, so grouped
    // items such as arrows are exported too)
    if (auto *overlay = overlayManager_->overlay(i)) {
      const QTransform pageTransform = painter.worldTransform();
      ItemStore *store = itemStore();
      if (store) {
        for (const ItemId &id : overlay->itemIds()) {
          paintItemTree(&painter, store->item(id), pageTransform,
                        /*paintSelection=*/false);
        }
      } else {
        for (QGraphicsItem *item : overlay->items()) {
          paintItemTree(&painter, item, pageTransform,
                        /*paintSelection=*/false);
        }
      }
    }

    painter.restore();
  }

  const bool ok = painter.isActive();
  painter.end();

  // Restore current page
  goToPage(savedPage);

  return ok;
}

void PdfViewer::drawBackground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawBackground(painter, rect);

  // Draw subtle shadow around the PDF page for visual hierarchy
  if (pageItem_ && !pageItem_->pixmap().isNull()) {
    QRectF pageRect = pageItem_->sceneBoundingRect();

    // Draw shadow layers for depth effect
    painter->save();
    painter->setPen(Qt::NoPen);

    // Outer shadow
    QRectF shadowRect1 = pageRect.adjusted(-8, -8, 8, 8);
    painter->setBrush(QColor(0, 0, 0, 20));
    painter->drawRoundedRect(shadowRect1, 4, 4);

    // Middle shadow
    QRectF shadowRect2 = pageRect.adjusted(-4, -4, 4, 4);
    painter->setBrush(QColor(0, 0, 0, 35));
    painter->drawRoundedRect(shadowRect2, 2, 2);

    // Inner shadow
    QRectF shadowRect3 = pageRect.adjusted(-2, -2, 2, 2);
    painter->setBrush(QColor(0, 0, 0, 50));
    painter->drawRoundedRect(shadowRect3, 1, 1);

    painter->restore();
  }

  if (showGrid_) {
    painter->setPen(QPen(QColor(80, 80, 80), 0.5));
    qreal left = int(rect.left()) - (int(rect.left()) % GRID_SIZE);
    qreal top = int(rect.top()) - (int(rect.top()) % GRID_SIZE);
    QVector<QLineF> lines;
    for (qreal x = left; x < rect.right(); x += GRID_SIZE) {
      lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for (qreal y = top; y < rect.bottom(); y += GRID_SIZE) {
      lines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    painter->drawLines(lines);
  }
}

void PdfViewer::mousePressEvent(QMouseEvent *event) {
  if (!hasPdf()) {
    QGraphicsView::mousePressEvent(event);
    return;
  }

  QPointF sp = mapToScene(event->pos());
  emit cursorPositionChanged(sp);

  // In View mode, only allow panning (handled by ScrollHandDrag)
  if (mode_ == Mode::View) {
    QGraphicsView::mousePressEvent(event);
    return;
  }

  // Handle screenshot selection mode (PDF-specific tool)
  if (specialTool_ == SpecialTool::ScreenshotSelection) {
    // Only the left button starts a capture; right-click opens the context
    // menu and must not drop a stray rubber band on the page.
    if (event->button() != Qt::LeftButton) {
      QGraphicsView::mousePressEvent(event);
      return;
    }
    // A previous band can survive a lost release – never leak it.
    if (screenshotSelectionRect_) {
      if (screenshotSelectionRect_->scene())
        scene_->removeItem(screenshotSelectionRect_);
      delete screenshotSelectionRect_;
      screenshotSelectionRect_ = nullptr;
    }
    startPoint_ = sp;
    screenshotSelectionRect_ =
        new QGraphicsRectItem(QRectF(startPoint_, startPoint_));
    QPen selectionPen(Qt::blue, 2, Qt::DashLine);
    screenshotSelectionRect_->setPen(selectionPen);
    screenshotSelectionRect_->setBrush(QBrush(QColor(100, 149, 237, 50)));
    screenshotSelectionRect_->setZValue(1000);
    scene_->addItem(screenshotSelectionRect_);
    return;
  }

  // Check if current tool uses rubber band selection (let QGraphicsView handle
  // it)
  Tool *tool = toolManager_->activeTool();
  if (tool && tool->usesRubberBandSelection()) {
    QGraphicsView::mousePressEvent(event);
    // Remember where the (possibly just selected) items start, so a drag
    // becomes one undoable move on release.
    moveStartPositions_.clear();
    if (event->button() == Qt::LeftButton) {
      if (ItemStore *store = itemStore()) {
        for (QGraphicsItem *item : scene_->selectedItems()) {
          if (!item || item->parentItem() || item == pageItem_)
            continue;
          const ItemId id = store->idForItem(item);
          if (id.isValid())
            moveStartPositions_.insert(id, item->pos());
        }
      }
    }
    return;
  }

  // Delegate to the current tool
  if (tool) {
    tool->mousePressEvent(event, sp);
  }
}

void PdfViewer::mouseMoveEvent(QMouseEvent *event) {
  if (!hasPdf()) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  QPointF cp = mapToScene(event->pos());
  emit cursorPositionChanged(cp);

  // In View mode, only allow panning
  if (mode_ == Mode::View) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  // Handle screenshot selection mode
  if (specialTool_ == SpecialTool::ScreenshotSelection) {
    if (screenshotSelectionRect_ && (event->buttons() & Qt::LeftButton)) {
      screenshotSelectionRect_->setRect(QRectF(startPoint_, cp).normalized());
    }
    return;
  }

  // Check if current tool uses rubber band selection
  Tool *tool = toolManager_->activeTool();
  if (tool && tool->usesRubberBandSelection()) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  // Delegate to the current tool
  if (tool) {
    tool->mouseMoveEvent(event, cp);
  }
}

void PdfViewer::mouseReleaseEvent(QMouseEvent *event) {
  if (!hasPdf()) {
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }

  QPointF ep = mapToScene(event->pos());

  // In View mode, only allow panning
  if (mode_ == Mode::View) {
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }

  // Handle screenshot selection mode
  if (specialTool_ == SpecialTool::ScreenshotSelection &&
      screenshotSelectionRect_) {
    // Only the button that started the capture ends it.
    if (event->button() != Qt::LeftButton) {
      return;
    }
    QRectF selectionRect = screenshotSelectionRect_->rect();
    scene_->removeItem(screenshotSelectionRect_);
    delete screenshotSelectionRect_;
    screenshotSelectionRect_ = nullptr;

    if (selectionRect.width() > 5 && selectionRect.height() > 5) {
      captureScreenshot(selectionRect);
      // One capture per activation; otherwise every later drag would keep
      // pasting screenshots into the canvas.
      setScreenshotSelectionMode(false);
    }
    return;
  }

  // Check if current tool uses rubber band selection
  Tool *tool = toolManager_->activeTool();
  if (tool && tool->usesRubberBandSelection()) {
    QGraphicsView::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton && !moveStartPositions_.isEmpty()) {
      ItemStore *store = itemStore();
      auto composite = std::make_unique<CompositeAction>();
      for (auto it = moveStartPositions_.cbegin();
           it != moveStartPositions_.cend(); ++it) {
        QGraphicsItem *item = store ? store->item(it.key()) : nullptr;
        if (item && item->pos() != it.value()) {
          composite->addAction(std::make_unique<MoveAction>(
              it.key(), store, it.value(), item->pos()));
        }
      }
      moveStartPositions_.clear();
      if (!composite->isEmpty())
        addAction(std::move(composite));
    }
    return;
  }

  // Delegate to the current tool
  if (tool) {
    tool->mouseReleaseEvent(event, ep);
  }
}

void PdfViewer::mouseDoubleClickEvent(QMouseEvent *event) {
  if (!hasPdf()) {
    QGraphicsView::mouseDoubleClickEvent(event);
    return;
  }

  if (mode_ == Mode::View || specialTool_ != SpecialTool::None) {
    QGraphicsView::mouseDoubleClickEvent(event);
    return;
  }

  // Forward to the active tool so it can implement double-click gestures
  // (e.g. finalizing a Bezier / text-on-path).
  Tool *tool = toolManager_->activeTool();
  if (tool && !tool->usesDoubleClick() && !tool->usesRubberBandSelection()) {
    // A fast second click arrives as a double-click; handle it as a new
    // press or quick successive strokes/shapes would be dropped.
    mousePressEvent(event);
    return;
  }
  if (tool) {
    tool->mouseDoubleClickEvent(event, mapToScene(event->pos()));
  }
}

void PdfViewer::keyPressEvent(QKeyEvent *event) {
  if (!event) {
    return;
  }
  // Enter commits and Escape discards an in-progress path gesture; when no
  // gesture is running the keys keep their window-level meaning.
  Tool *tool = toolManager_ ? toolManager_->activeTool() : nullptr;
  if (tool && tool->hasActiveGesture()) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
      tool->finishGesture();
      event->accept();
      return;
    }
    if (event->key() == Qt::Key_Escape) {
      tool->cancelGesture();
      event->accept();
      return;
    }
  }
  // Page navigation. QGraphicsView would consume these keys for scrolling,
  // so the window-level shortcuts never saw them once the viewer had focus.
  // (Not while an inline text editor has focus: it needs Home/End.)
  if (event->modifiers() == Qt::NoModifier && !scene_->focusItem()) {
    switch (event->key()) {
    case Qt::Key_PageDown:
      nextPage();
      event->accept();
      return;
    case Qt::Key_PageUp:
      previousPage();
      event->accept();
      return;
    case Qt::Key_Home:
      firstPage();
      event->accept();
      return;
    case Qt::Key_End:
      lastPage();
      event->accept();
      return;
    default:
      break;
    }
  }
  if (event->key() == Qt::Key_Escape && specialTool_ != SpecialTool::None) {
    // Leave screenshot-selection (or other special) mode.
    setScreenshotSelectionMode(false);
    event->accept();
    return;
  }
  QGraphicsView::keyPressEvent(event);
}

void PdfViewer::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    event->angleDelta().y() > 0 ? zoomIn() : zoomOut();
    event->accept();
  } else {
    QGraphicsView::wheelEvent(event);
  }
}

// Helper function to check if a URL points to a PDF file
static bool isPdfFile(const QUrl &url) {
  if (url.isLocalFile()) {
    QString extension = QFileInfo(url.toLocalFile()).suffix().toLower();
    return extension == "pdf";
  }
  return false;
}

// Helper function to check if mime data contains a PDF file
static bool containsPdfFile(const QMimeData *mimeData) {
  if (mimeData->hasUrls()) {
    for (const QUrl &url : mimeData->urls()) {
      if (isPdfFile(url)) {
        return true;
      }
    }
  }
  return false;
}

void PdfViewer::dragEnterEvent(QDragEnterEvent *event) {
  // Accept the drag if it contains PDF files
  if (containsPdfFile(event->mimeData())) {
    dragAccepted_ = true;
    event->acceptProposedAction();
    return;
  }
  dragAccepted_ = false;
  // Let the base class handle other drag events
  QGraphicsView::dragEnterEvent(event);
}

void PdfViewer::dragMoveEvent(QDragMoveEvent *event) {
  // Accept the drag move if it contains PDF files
  if (dragAccepted_ && containsPdfFile(event->mimeData())) {
    event->acceptProposedAction();
    return;
  }
  QGraphicsView::dragMoveEvent(event);
}

void PdfViewer::dragLeaveEvent(QDragLeaveEvent *event) {
  if (dragAccepted_) {
    dragAccepted_ = false;
    event->accept();
    return;
  }
  QGraphicsView::dragLeaveEvent(event);
}

void PdfViewer::dropEvent(QDropEvent *event) {
  // Handle the dropped PDF files
  const QMimeData *mimeData = event->mimeData();

  if (mimeData->hasUrls()) {
    dragAccepted_ = false;
    for (const QUrl &url : mimeData->urls()) {
      if (isPdfFile(url)) {
        emit pdfFileDropped(url.toLocalFile());
        event->acceptProposedAction();
        return;
      }
    }
  }

  QGraphicsView::dropEvent(event);
}

void PdfViewer::captureScreenshot(const QRectF &rect) {
  if (!hasPdf() || rect.isEmpty()) {
    return;
  }

  // Scale the screenshot to match the effective render DPI of the page.
  // The page pixmap is rendered at effectiveRenderDpi() and displayed in the
  // scene with a scale factor of (renderDpi_ / effectiveRenderDpi()). If we
  // create the screenshot at just the scene-coordinate size (base DPI), we
  // lose the extra resolution when the view is zoomed in or resized with
  // fit-to modes.  By scaling up the target image, scene_->render()
  // composites the high-res pixmap closer to its native resolution,
  // preserving quality.
  const int effectiveDpi = effectiveRenderDpi();
  const double dpiScale =
      static_cast<double>(effectiveDpi) / static_cast<double>(renderDpi_);
  // Also honour the device-pixel ratio so HiDPI screens don't lose detail.
  // We use qMax rather than multiplying because the underlying page pixmap
  // only contains dpiScale worth of resolution; exceeding that would just
  // produce interpolated (upscaled) pixels without adding real detail.
  const double dpr = qMax(1.0, static_cast<double>(devicePixelRatioF()));
  const double scaleFactor = qMax(dpiScale, dpr);

  QSize imageSize(
      qMax(1, static_cast<int>(std::ceil(rect.width() * scaleFactor))),
      qMax(1, static_cast<int>(std::ceil(rect.height() * scaleFactor))));

  // Create an image to render the selected area
  QImage screenshot(imageSize, QImage::Format_ARGB32);
  // Fill with white background (standard PDF background color)
  // instead of transparent to ensure proper visibility
  screenshot.fill(Qt::white);

  QPainter painter(&screenshot);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  // Render the scene area (including PDF page and any annotations)
  // The target rect covers the full (scaled) image so that the scene content
  // in `rect` is painted at the higher resolution.
  scene_->render(&painter, screenshot.rect(), rect);

  painter.end();

  // Emit the signal with the captured image
  emit screenshotCaptured(screenshot);
}

// --- Search Implementation ---

void PdfViewer::openSearch() {
  if (!hasPdf()) {
    return;
  }
  searchBar_->activate();
}

void PdfViewer::closeSearch() {
  {
    // deactivate() emits closed(), which is connected back to this slot.
    const QSignalBlocker blocker(searchBar_);
    searchBar_->deactivate();
  }
  searchModel_->setSearchString(QString());
  currentMatchIndex_ = -1;
  totalMatchCount_ = 0;
  currentPageHighlights_.clear();
  highlightedMatchPage_ = -1;
  highlightedMatchIndexOnPage_ = -1;
  viewport()->update();
  scene_->invalidate(scene_->sceneRect(), QGraphicsScene::ForegroundLayer);
}

void PdfViewer::performSearch(const QString &text) {
  if (text.isEmpty()) {
    searchModel_->setSearchString(QString());
    currentMatchIndex_ = -1;
    totalMatchCount_ = 0;
    currentPageHighlights_.clear();
    highlightedMatchPage_ = -1;
    highlightedMatchIndexOnPage_ = -1;
    searchBar_->setMatchInfo(0, 0);
    viewport()->update();
    scene_->invalidate(scene_->sceneRect(), QGraphicsScene::ForegroundLayer);
    return;
  }

  // Results arrive asynchronously via onSearchResultsChanged().
  searchStartPage_ = currentPage_;
  searchUserNavigated_ = false;
  currentMatchIndex_ = -1;
  totalMatchCount_ = 0;
  currentPageHighlights_.clear();
  highlightedMatchPage_ = -1;
  highlightedMatchIndexOnPage_ = -1;
  searchModel_->setSearchString(text);
  onSearchResultsChanged();
}

void PdfViewer::onSearchResultsChanged() {
  if (!searchModel_ || searchModel_->searchString().isEmpty())
    return;
  totalMatchCount_ = searchModel_->rowCount(QModelIndex());
  if (totalMatchCount_ <= 0) {
    currentMatchIndex_ = -1;
    currentPageHighlights_.clear();
    highlightedMatchPage_ = -1;
    highlightedMatchIndexOnPage_ = -1;
    searchBar_->setMatchInfo(0, 0);
    viewport()->update();
    scene_->invalidate(scene_->sceneRect(), QGraphicsScene::ForegroundLayer);
    return;
  }
  if (!searchUserNavigated_) {
    // Results arrive in batches: until the user steps through matches, keep
    // targeting the first match on or after the page the search started on
    // (an early batch may only contain earlier pages).
    int preferred = 0;
    for (int i = 0; i < totalMatchCount_; ++i) {
      if (searchModel_->resultAtIndex(i).page() >= searchStartPage_) {
        preferred = i;
        break;
      }
    }
    if (preferred != currentMatchIndex_) {
      currentMatchIndex_ = preferred;
      navigateToMatch(currentMatchIndex_);
      return;
    }
  }
  // More results for the current target: refresh count and highlights.
  searchBar_->setMatchInfo(currentMatchIndex_ + 1, totalMatchCount_);
  updateSearchHighlights();
  viewport()->update();
  scene_->invalidate(scene_->sceneRect(), QGraphicsScene::ForegroundLayer);
}

void PdfViewer::findNext() {
  if (totalMatchCount_ <= 0) {
    return;
  }
  searchUserNavigated_ = true;
  currentMatchIndex_ = (currentMatchIndex_ + 1) % totalMatchCount_;
  navigateToMatch(currentMatchIndex_);
}

void PdfViewer::findPrevious() {
  if (totalMatchCount_ <= 0) {
    return;
  }
  searchUserNavigated_ = true;
  currentMatchIndex_ =
      (currentMatchIndex_ - 1 + totalMatchCount_) % totalMatchCount_;
  navigateToMatch(currentMatchIndex_);
}

void PdfViewer::navigateToMatch(int index) {
  if (index < 0 || index >= totalMatchCount_) {
    return;
  }

  QPdfLink link = searchModel_->resultAtIndex(index);
  int matchPage = link.page();

  // Navigate to the page if different
  if (matchPage != currentPage_) {
    // goToPage will call updateSearchHighlights
    goToPage(matchPage);
  } else {
    updateSearchHighlights();
  }

  // Track which match is the "active" one for distinct highlighting
  highlightedMatchPage_ = matchPage;
  // Determine the index-on-page for this match
  QList<QPdfLink> pageResults = searchModel_->resultsOnPage(matchPage);
  highlightedMatchIndexOnPage_ = -1;
  for (int i = 0; i < pageResults.size(); ++i) {
    if (pageResults[i].rectangles() == link.rectangles()) {
      highlightedMatchIndexOnPage_ = i;
      break;
    }
  }

  searchBar_->setMatchInfo(index + 1, totalMatchCount_);

  // Scroll to make the match visible: convert PDF-point rects to scene coords
  QList<QRectF> rects = link.rectangles();
  if (!rects.isEmpty()) {
    QSizeF pageSizePt = document_->pageSize(matchPage);
    if (!pageSizePt.isEmpty()) {
      QRectF sceneRect = scene_->sceneRect();
      double sx = sceneRect.width() / pageSizePt.width();
      double sy = sceneRect.height() / pageSizePt.height();
      QRectF firstRect = rects.first();
      QRectF mapped(firstRect.x() * sx, firstRect.y() * sy,
                    firstRect.width() * sx, firstRect.height() * sy);
      // Add generous padding so the match isn't right at the edge
      mapped.adjust(-30, -30, 30, 30);
      ensureVisible(mapped, 50, 50);
    }
  }

  viewport()->update();
  scene_->invalidate(scene_->sceneRect(), QGraphicsScene::ForegroundLayer);
}

void PdfViewer::updateSearchHighlights() {
  currentPageHighlights_.clear();
  if (!hasPdf() || !searchModel_ || searchModel_->searchString().isEmpty()) {
    return;
  }

  QList<QPdfLink> pageResults = searchModel_->resultsOnPage(currentPage_);
  QSizeF pageSizePt = document_->pageSize(currentPage_);
  if (pageSizePt.isEmpty()) {
    return;
  }

  QRectF sceneRect = scene_->sceneRect();
  double sx = sceneRect.width() / pageSizePt.width();
  double sy = sceneRect.height() / pageSizePt.height();

  for (const QPdfLink &link : pageResults) {
    for (const QRectF &r : link.rectangles()) {
      currentPageHighlights_.append(
          QRectF(r.x() * sx, r.y() * sy, r.width() * sx, r.height() * sy));
    }
  }
}

void PdfViewer::drawForeground(QPainter *painter, const QRectF &rect) {
  Q_UNUSED(rect);
  if (currentPageHighlights_.isEmpty()) {
    return;
  }

  painter->save();

  // Determine active match rectangles for distinct highlighting
  QList<QRectF> activeRects;
  if (highlightedMatchPage_ == currentPage_ &&
      highlightedMatchIndexOnPage_ >= 0) {
    QList<QPdfLink> pageResults = searchModel_->resultsOnPage(currentPage_);
    if (highlightedMatchIndexOnPage_ < pageResults.size()) {
      QPdfLink activeLink = pageResults[highlightedMatchIndexOnPage_];
      QSizeF pageSizePt = document_->pageSize(currentPage_);
      if (!pageSizePt.isEmpty()) {
        QRectF sr = scene_->sceneRect();
        double sx = sr.width() / pageSizePt.width();
        double sy = sr.height() / pageSizePt.height();
        for (const QRectF &r : activeLink.rectangles()) {
          activeRects.append(
              QRectF(r.x() * sx, r.y() * sy, r.width() * sx, r.height() * sy));
        }
      }
    }
  }

  // Draw all highlights (yellow, semi-transparent)
  for (const QRectF &r : currentPageHighlights_) {
    bool isActive = false;
    for (const QRectF &ar : activeRects) {
      if (qAbs(ar.x() - r.x()) < 1 && qAbs(ar.y() - r.y()) < 1) {
        isActive = true;
        break;
      }
    }

    if (isActive) {
      // Active match: orange highlight with border
      painter->setPen(QPen(QColor(245, 130, 30), 2));
      painter->setBrush(QColor(245, 160, 50, 100));
    } else {
      // Other matches: yellow highlight
      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(255, 230, 0, 80));
    }
    painter->drawRoundedRect(r, 2, 2);
  }

  painter->restore();
}

void PdfViewer::resizeEvent(QResizeEvent *event) {
  QGraphicsView::resizeEvent(event);
  // Reposition the search bar when the viewer is resized
  // (activate() would also steal focus and re-select the typed text).
  if (searchBar_ && searchBar_->isVisible()) {
    searchBar_->positionInParent();
  }
}

void PdfViewer::scrollContentsBy(int dx, int dy) {
  QGraphicsView::scrollContentsBy(dx, dy);
  // Scrolling the viewport also moves its child widgets; keep the search
  // bar pinned to the top-right corner instead of scrolling it away.
  if (searchBar_ && searchBar_->isVisible()) {
    searchBar_->positionInParent();
  }
}

#endif // HAVE_QT_PDF
