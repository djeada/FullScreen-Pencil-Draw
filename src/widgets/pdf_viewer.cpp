/**
 * @file pdf_viewer.cpp
 * @brief Implementation of PDF viewing widget with annotation support.
 */
#include "pdf_viewer.h"

#ifdef HAVE_QT_PDF

#include "latex_text_item.h"
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
#include <QPdfWriter>
#include <QPointer>
#include <QScrollBar>
#include <QUrl>
#include <QWheelEvent>
#include <cmath>

// --- PdfPageItem Implementation ---

PdfPageItem::PdfPageItem(QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent), inverted_(false) {
  setFlag(QGraphicsItem::ItemIsSelectable, false);
  setFlag(QGraphicsItem::ItemIsMovable, false);
  setZValue(-1000); // Behind all other items
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
      pageItem_(nullptr), scene_(new QGraphicsScene(this)), currentPage_(0),
      renderDpi_(DEFAULT_DPI), darkMode_(false), showGrid_(false),
      fillShapes_(false), currentZoom_(1.0), currentTool_(Tool::Pen),
      currentPen_(Qt::white, 3), eraserPen_(Qt::black, 10),
      tempShapeItem_(nullptr), currentPath_(nullptr), isPanning_(false),
      screenshotSelectionRect_(nullptr) {

  setupScene();

  // Connect document signals
  connect(document_.get(), &PdfDocument::documentLoaded, this, [this]() {
    overlayManager_->initialize(document_->pageCount());
    goToPage(0);
    emit pdfLoaded();
  });

  connect(document_.get(), &PdfDocument::errorOccurred, this,
          &PdfViewer::errorOccurred);
}

PdfViewer::~PdfViewer() = default;

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
  // Remove page item
  if (pageItem_) {
    scene_->removeItem(pageItem_);
    delete pageItem_;
    pageItem_ = nullptr;
  }

  // Clear overlays and scene
  overlayManager_->clear();
  scene_->clear();

  document_->close();
  currentPage_ = 0;
  currentZoom_ = 1.0;
  resetTransform();

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

  currentPage_ = pageIndex;
  renderCurrentPage();

  // Show only the current page's overlay
  overlayManager_->showPage(currentPage_);

  emit pageChanged(currentPage_, pageCount());
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

  QImage pageImage = document_->renderPage(currentPage_, renderDpi_, false);
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

  // Update scene rect to fit page
  scene_->setSceneRect(0, 0, pageImage.width(), pageImage.height());
}

void PdfViewer::setTool(Tool tool) {
  currentTool_ = tool;
  tempShapeItem_ = nullptr;

  // Reset drag mode
  setDragMode(QGraphicsView::NoDrag);

  // Set appropriate cursor
  switch (tool) {
  case Tool::Selection:
    setCursor(Qt::ArrowCursor);
    setDragMode(QGraphicsView::RubberBandDrag);
    break;
  case Tool::Pan:
    setCursor(Qt::OpenHandCursor);
    break;
  case Tool::Text:
    setCursor(Qt::IBeamCursor);
    break;
  case Tool::Fill:
    setCursor(Qt::PointingHandCursor);
    break;
  case Tool::Eraser:
    setCursor(Qt::CrossCursor);
    break;
  case Tool::ScreenshotSelection:
    setCursor(Qt::CrossCursor);
    break;
  default:
    setCursor(Qt::CrossCursor);
    break;
  }

  scene_->clearSelection();
  isPanning_ = false;
}

void PdfViewer::setPenColor(const QColor &color) { currentPen_.setColor(color); }

void PdfViewer::setPenWidth(int width) {
  currentPen_.setWidth(width);
  eraserPen_.setWidth(width * 2);
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
      scene_->setBackgroundBrush(QColor(240, 240, 240)); // Light gray background
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

void PdfViewer::zoomReset() {
  resetTransform();
  currentZoom_ = 1.0;
  emit zoomChanged(100.0);
}

void PdfViewer::applyZoom(double factor) {
  double newZoom = currentZoom_ * factor;
  if (newZoom > MAX_ZOOM || newZoom < MIN_ZOOM) {
    return;
  }
  currentZoom_ = newZoom;
  scale(factor, factor);
  emit zoomChanged(currentZoom_ * 100.0);
}

void PdfViewer::toggleGrid() {
  showGrid_ = !showGrid_;
  viewport()->update();
  scene_->invalidate(scene_->sceneRect(), QGraphicsScene::BackgroundLayer);
}

void PdfViewer::undo() {
  if (!hasPdf()) {
    return;
  }

  auto &undoStack = overlayManager_->undoStack(currentPage_);
  auto &redoStack = overlayManager_->redoStack(currentPage_);

  if (!undoStack.empty()) {
    std::unique_ptr<Action> action = std::move(undoStack.back());
    undoStack.pop_back();
    action->undo();
    redoStack.push_back(std::move(action));
  }
}

void PdfViewer::redo() {
  if (!hasPdf()) {
    return;
  }

  auto &undoStack = overlayManager_->undoStack(currentPage_);
  auto &redoStack = overlayManager_->redoStack(currentPage_);

  if (!redoStack.empty()) {
    std::unique_ptr<Action> action = std::move(redoStack.back());
    redoStack.pop_back();
    action->redo();
    undoStack.push_back(std::move(action));
  }
}

bool PdfViewer::canUndo() const {
  return hasPdf() && overlayManager_->canUndo(currentPage_);
}

bool PdfViewer::canRedo() const {
  return hasPdf() && overlayManager_->canRedo(currentPage_);
}

void PdfViewer::addDrawAction(QGraphicsItem *item) {
  if (!hasPdf()) {
    return;
  }

  auto &undoStack = overlayManager_->undoStack(currentPage_);
  undoStack.push_back(std::make_unique<DrawAction>(item, scene_));
  clearRedoStack();
  overlayManager_->addItemToPage(currentPage_, item);
  emit documentModified();
}

void PdfViewer::addDeleteAction(QGraphicsItem *item) {
  if (!hasPdf()) {
    return;
  }

  auto &undoStack = overlayManager_->undoStack(currentPage_);
  undoStack.push_back(std::make_unique<DeleteAction>(item, scene_));
  clearRedoStack();
}

void PdfViewer::clearRedoStack() {
  if (hasPdf()) {
    overlayManager_->redoStack(currentPage_).clear();
  }
}

bool PdfViewer::exportAnnotatedPdf(const QString &filePath) {
  if (!hasPdf()) {
    return false;
  }

  QPdfWriter pdfWriter(filePath);
  pdfWriter.setPageSize(QPageSize::A4);
  pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));
  pdfWriter.setTitle("Annotated PDF Export");
  pdfWriter.setCreator("FullScreen Pencil Draw");
  pdfWriter.setResolution(renderDpi_);

  QPainter painter(&pdfWriter);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.setRenderHint(QPainter::TextAntialiasing);

  int savedPage = currentPage_;

  for (int i = 0; i < pageCount(); ++i) {
    if (i > 0) {
      pdfWriter.newPage();
    }

    // Render PDF page
    QImage pageImage = document_->renderPage(i, renderDpi_, darkMode_);
    if (pageImage.isNull()) {
      continue;
    }

    // Calculate scale to fit page
    QRectF pageRect = painter.viewport();
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

    // Draw overlay items for this page
    if (auto *overlay = overlayManager_->overlay(i)) {
      for (QGraphicsItem *item : overlay->items()) {
        if (item) {
          painter.save();
          painter.setTransform(item->sceneTransform(), true);
          item->paint(&painter, nullptr, nullptr);
          painter.restore();
        }
      }
    }

    painter.restore();
  }

  painter.end();

  // Restore current page
  goToPage(savedPage);

  return true;
}

void PdfViewer::drawBackground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawBackground(painter, rect);

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

  if (currentTool_ == Tool::Selection) {
    QGraphicsView::mousePressEvent(event);
    return;
  }

  if (currentTool_ == Tool::Pan) {
    isPanning_ = true;
    lastPanPoint_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    return;
  }

  startPoint_ = sp;

  switch (currentTool_) {
  case Tool::Text:
    createTextItem(sp);
    break;
  case Tool::Fill:
    fillAt(sp);
    break;
  case Tool::Eraser:
    eraseAt(sp);
    break;
  case Tool::Pen: {
    currentPath_ = new QGraphicsPathItem();
    currentPath_->setPen(currentPen_);
    currentPath_->setFlags(QGraphicsItem::ItemIsSelectable |
                           QGraphicsItem::ItemIsMovable);
    QPainterPath p;
    p.moveTo(sp);
    currentPath_->setPath(p);
    scene_->addItem(currentPath_);
    pointBuffer_.clear();
    pointBuffer_.append(sp);
    addDrawAction(currentPath_);
  } break;
  case Tool::Rectangle:
  case Tool::Arrow: {
    auto ri = new QGraphicsRectItem(QRectF(startPoint_, startPoint_));
    ri->setPen(currentPen_);
    if (fillShapes_ && currentTool_ == Tool::Rectangle) {
      ri->setBrush(currentPen_.color());
    }
    ri->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    scene_->addItem(ri);
    tempShapeItem_ = ri;
    if (currentTool_ == Tool::Rectangle) {
      addDrawAction(ri);
    }
  } break;
  case Tool::Circle: {
    auto ei = new QGraphicsEllipseItem(QRectF(startPoint_, startPoint_));
    ei->setPen(currentPen_);
    if (fillShapes_) {
      ei->setBrush(currentPen_.color());
    }
    ei->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    scene_->addItem(ei);
    tempShapeItem_ = ei;
    addDrawAction(ei);
  } break;
  case Tool::Line: {
    auto li = new QGraphicsLineItem(QLineF(startPoint_, startPoint_));
    li->setPen(currentPen_);
    li->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    scene_->addItem(li);
    tempShapeItem_ = li;
    addDrawAction(li);
  } break;
  case Tool::ScreenshotSelection: {
    // Create a dashed rectangle for screenshot selection
    screenshotSelectionRect_ = new QGraphicsRectItem(QRectF(startPoint_, startPoint_));
    QPen selectionPen(Qt::blue, 2, Qt::DashLine);
    screenshotSelectionRect_->setPen(selectionPen);
    screenshotSelectionRect_->setBrush(QBrush(QColor(100, 149, 237, 50))); // Light blue with transparency
    screenshotSelectionRect_->setZValue(1000); // Above everything else
    scene_->addItem(screenshotSelectionRect_);
  } break;
  default:
    QGraphicsView::mousePressEvent(event);
    break;
  }
}

void PdfViewer::mouseMoveEvent(QMouseEvent *event) {
  if (!hasPdf()) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  QPointF cp = mapToScene(event->pos());
  emit cursorPositionChanged(cp);

  if (currentTool_ == Tool::Selection) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  if (currentTool_ == Tool::Pan && isPanning_) {
    QPointF d = event->pos() - lastPanPoint_;
    lastPanPoint_ = event->pos();
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
    return;
  }

  switch (currentTool_) {
  case Tool::Pen:
    if (event->buttons() & Qt::LeftButton) {
      addPoint(cp);
    }
    break;
  case Tool::Eraser:
    if (event->buttons() & Qt::LeftButton) {
      eraseAt(cp);
    }
    break;
  case Tool::Arrow:
  case Tool::Rectangle:
    if (tempShapeItem_) {
      static_cast<QGraphicsRectItem *>(tempShapeItem_)
          ->setRect(QRectF(startPoint_, cp).normalized());
    }
    break;
  case Tool::Circle:
    if (tempShapeItem_) {
      static_cast<QGraphicsEllipseItem *>(tempShapeItem_)
          ->setRect(QRectF(startPoint_, cp).normalized());
    }
    break;
  case Tool::Line:
    if (tempShapeItem_) {
      static_cast<QGraphicsLineItem *>(tempShapeItem_)
          ->setLine(QLineF(startPoint_, cp));
    }
    break;
  case Tool::ScreenshotSelection:
    if (screenshotSelectionRect_ && (event->buttons() & Qt::LeftButton)) {
      screenshotSelectionRect_->setRect(QRectF(startPoint_, cp).normalized());
    }
    break;
  default:
    QGraphicsView::mouseMoveEvent(event);
    break;
  }
}

void PdfViewer::mouseReleaseEvent(QMouseEvent *event) {
  if (!hasPdf()) {
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }

  QPointF ep = mapToScene(event->pos());

  if (currentTool_ == Tool::Selection) {
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }

  if (currentTool_ == Tool::Pan) {
    isPanning_ = false;
    setCursor(Qt::OpenHandCursor);
    return;
  }

  if (currentTool_ == Tool::Arrow && tempShapeItem_) {
    scene_->removeItem(tempShapeItem_);
    delete tempShapeItem_;
    tempShapeItem_ = nullptr;
    drawArrow(startPoint_, ep);
    return;
  }

  if (currentTool_ == Tool::ScreenshotSelection && screenshotSelectionRect_) {
    QRectF selectionRect = screenshotSelectionRect_->rect();
    // Remove the selection rectangle from the scene
    scene_->removeItem(screenshotSelectionRect_);
    delete screenshotSelectionRect_;
    screenshotSelectionRect_ = nullptr;
    
    // Capture the screenshot if the selection has a valid size
    if (selectionRect.width() > 5 && selectionRect.height() > 5) {
      captureScreenshot(selectionRect);
    }
    return;
  }

  if (currentTool_ != Tool::Pen && currentTool_ != Tool::Eraser &&
      tempShapeItem_) {
    tempShapeItem_ = nullptr;
  } else if (currentTool_ == Tool::Pen) {
    currentPath_ = nullptr;
    pointBuffer_.clear();
  }
}

void PdfViewer::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    event->angleDelta().y() > 0 ? zoomIn() : zoomOut();
    event->accept();
  } else {
    QGraphicsView::wheelEvent(event);
  }
}

void PdfViewer::addPoint(const QPointF &point) {
  if (!currentPath_) {
    return;
  }

  pointBuffer_.append(point);

  // Use Catmull-Rom spline interpolation for smooth curves
  if (pointBuffer_.size() >= 4) {
    QPainterPath path = currentPath_->path();

    const QPointF &p0 = pointBuffer_[pointBuffer_.size() - 4];
    const QPointF &p1 = pointBuffer_[pointBuffer_.size() - 3];
    const QPointF &p2 = pointBuffer_[pointBuffer_.size() - 2];
    const QPointF &p3 = pointBuffer_[pointBuffer_.size() - 1];

    for (int i = 1; i <= 5; ++i) {
      qreal t = i / 5.0;
      qreal t2 = t * t;
      qreal t3 = t2 * t;

      qreal x = 0.5 * ((2 * p1.x()) + (-p0.x() + p2.x()) * t +
                       (2 * p0.x() - 5 * p1.x() + 4 * p2.x() - p3.x()) * t2 +
                       (-p0.x() + 3 * p1.x() - 3 * p2.x() + p3.x()) * t3);

      qreal y = 0.5 * ((2 * p1.y()) + (-p0.y() + p2.y()) * t +
                       (2 * p0.y() - 5 * p1.y() + 4 * p2.y() - p3.y()) * t2 +
                       (-p0.y() + 3 * p1.y() - 3 * p2.y() + p3.y()) * t3);

      path.lineTo(x, y);
    }

    currentPath_->setPath(path);
  } else if (pointBuffer_.size() >= 2) {
    QPainterPath path = currentPath_->path();
    path.lineTo(point);
    currentPath_->setPath(path);
  }
}

void PdfViewer::eraseAt(const QPointF &point) {
  QList<QGraphicsItem *> items = scene_->items(point);
  for (QGraphicsItem *item : items) {
    // Don't erase the PDF page item
    if (item == pageItem_) {
      continue;
    }
    addDeleteAction(item);
    scene_->removeItem(item);
    overlayManager_->removeItemFromPage(currentPage_, item);
    delete item;
    break; // Only erase one item at a time
  }
}

void PdfViewer::createTextItem(const QPointF &pos) {
  auto *textItem = new LatexTextItem();
  textItem->setFont(QFont("Arial", qMax(12, currentPen_.width() * 3)));
  textItem->setTextColor(currentPen_.color());
  textItem->setPos(pos);

  scene_->addItem(textItem);

  // Connect to handle when editing is finished
  // Use QPointer to safely track the textItem in case it gets deleted before signal fires
  connect(textItem, &LatexTextItem::editingFinished, this, [this, textItem = QPointer<LatexTextItem>(textItem)]() {
    // Check if textItem is still valid (not deleted)
    if (!textItem) {
      return;
    }
    if (textItem->text().trimmed().isEmpty()) {
      scene_->removeItem(textItem);
      textItem->deleteLater();
    } else {
      addDrawAction(textItem);
    }
  });

  textItem->startEditing();
}

void PdfViewer::fillAt(const QPointF &point) {
  QBrush newBrush(currentPen_.color());
  for (QGraphicsItem *item : scene_->items(point)) {
    if (item == pageItem_) {
      continue;
    }
    
    QBrush oldBrush;
    bool canFill = false;
    
    if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) {
      oldBrush = r->brush();
      r->setBrush(newBrush);
      canFill = true;
    } else if (auto e = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      oldBrush = e->brush();
      e->setBrush(newBrush);
      canFill = true;
    }
    
    if (canFill) {
      auto &undoStack = overlayManager_->undoStack(currentPage_);
      undoStack.push_back(
          std::make_unique<FillAction>(item, oldBrush, newBrush));
      clearRedoStack();
      emit documentModified();
      return;
    }
  }
}

void PdfViewer::drawArrow(const QPointF &start, const QPointF &end) {
  auto li = new QGraphicsLineItem(QLineF(start, end));
  li->setPen(currentPen_);
  li->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
  scene_->addItem(li);

  double angle = std::atan2(-(end.y() - start.y()), end.x() - start.x());
  double sz = currentPen_.width() * 4;
  QPolygonF ah;
  ah << end
     << end + QPointF(std::sin(angle + M_PI / 3) * sz,
                      std::cos(angle + M_PI / 3) * sz)
     << end + QPointF(std::sin(angle + M_PI - M_PI / 3) * sz,
                      std::cos(angle + M_PI - M_PI / 3) * sz);
  auto ahi = new QGraphicsPolygonItem(ah);
  ahi->setPen(currentPen_);
  ahi->setBrush(currentPen_.color());
  ahi->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
  scene_->addItem(ahi);

  addDrawAction(li);
  addDrawAction(ahi);
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

  // Create an image to render the selected area
  QImage screenshot(rect.size().toSize(), QImage::Format_ARGB32);
  // Fill with white background (standard PDF background color)
  // instead of transparent to ensure proper visibility
  screenshot.fill(Qt::white);

  QPainter painter(&screenshot);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  // Render the scene area (including PDF page and any annotations)
  // Let scene_->render() handle the transformation from source rect to target
  scene_->render(&painter, screenshot.rect(), rect);

  painter.end();

  // Emit the signal with the captured image
  emit screenshotCaptured(screenshot);
}

#endif // HAVE_QT_PDF
