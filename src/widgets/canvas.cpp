/**
 * @file canvas.cpp
 * @brief Implementation of the main drawing canvas widget.
 */
#include "canvas.h"
#include "image_size_dialog.h"
#include "latex_text_item.h"
#include "transform_handle_item.h"
#include "../core/recent_files_manager.h"
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPdfWriter>
#include <QPointer>
#include <QScrollBar>
#ifdef HAVE_QT_SVG
#include <QSvgGenerator>
#endif
#include <QUrl>
#include <QWheelEvent>
#include <cmath>

// Supported image file extensions for drag-and-drop
static const QSet<QString> SUPPORTED_IMAGE_EXTENSIONS = {"png", "jpg", "jpeg",
                                                          "bmp", "gif"};

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)),
      layerManager_(nullptr), tempShapeItem_(nullptr), currentShape_(Pen),
      currentPen_(Qt::white, 3), eraserPen_(Qt::black, 10), currentPath_(nullptr),
      backgroundColor_(Qt::black), eraserPreview_(nullptr),
      backgroundImage_(nullptr), isPanning_(false) {

  this->setScene(scene_);
  this->setRenderHint(QPainter::Antialiasing);
  this->setRenderHint(QPainter::SmoothPixmapTransform);
  this->setRenderHint(QPainter::TextAntialiasing);
  this->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  this->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
  this->setCacheMode(QGraphicsView::CacheBackground);
  scene_->setSceneRect(0, 0, 3000, 2000);

  scene_->setBackgroundBrush(backgroundColor_);
  eraserPen_.setColor(backgroundColor_);

  // Initialize layer manager
  layerManager_ = new LayerManager(scene_, this);

  eraserPreview_ =
      scene_->addEllipse(0, 0, eraserPen_.width(), eraserPen_.width(),
                         QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview_->setZValue(1000);
  eraserPreview_->hide();

  this->setMouseTracking(true);
  scene_->setItemIndexMethod(QGraphicsScene::BspTreeIndex);

  // Enable drag and drop
  this->setAcceptDrops(true);

  currentPen_.setCapStyle(Qt::RoundCap);
  currentPen_.setJoinStyle(Qt::RoundJoin);
  
  // Connect to scene selection changes for transform handles
  connect(scene_, &QGraphicsScene::selectionChanged, this, &Canvas::updateTransformHandles);
}

Canvas::~Canvas() {
  clearTransformHandles();
  undoStack_.clear();
  redoStack_.clear();
}

int Canvas::getCurrentBrushSize() const { return currentPen_.width(); }
QColor Canvas::getCurrentColor() const { return currentPen_.color(); }
double Canvas::getCurrentZoom() const { return currentZoom_ * 100.0; }
int Canvas::getCurrentOpacity() const { return currentOpacity_; }
bool Canvas::isGridVisible() const { return showGrid_; }
bool Canvas::isFilledShapes() const { return fillShapes_; }
bool Canvas::isSnapToGridEnabled() const { return snapToGrid_; }
bool Canvas::isRulerVisible() const { return showRuler_; }
bool Canvas::isMeasurementToolEnabled() const { return measurementToolEnabled_; }

// Action management methods
void Canvas::addDrawAction(QGraphicsItem *item) {
  undoStack_.push_back(std::make_unique<DrawAction>(item, scene_));
  clearRedoStack();
  // Add item to active layer
  if (layerManager_) {
    layerManager_->addItemToActiveLayer(item);
  }
  emit canvasModified();
}

void Canvas::addDeleteAction(QGraphicsItem *item) {
  undoStack_.push_back(std::make_unique<DeleteAction>(item, scene_));
  clearRedoStack();
}

void Canvas::addAction(std::unique_ptr<Action> action) {
  undoStack_.push_back(std::move(action));
  clearRedoStack();
}

void Canvas::clearRedoStack() { redoStack_.clear(); }

void Canvas::drawBackground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawBackground(painter, rect);
  if (showGrid_) {
    painter->setPen(QPen(QColor(60, 60, 60), 0.5));
    qreal left = int(rect.left()) - (int(rect.left()) % GRID_SIZE);
    qreal top = int(rect.top()) - (int(rect.top()) % GRID_SIZE);
    QVector<QLineF> lines;
    for (qreal x = left; x < rect.right(); x += GRID_SIZE)
      lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    for (qreal y = top; y < rect.bottom(); y += GRID_SIZE)
      lines.append(QLineF(rect.left(), y, rect.right(), y));
    painter->drawLines(lines);
  }
}

void Canvas::setShape(const QString &shapeType) {
  if (shapeType == "Line") { currentShape_ = Line; setCursor(Qt::CrossCursor); }
  else if (shapeType == "Rectangle") { currentShape_ = Rectangle; setCursor(Qt::CrossCursor); }
  else if (shapeType == "Circle") { currentShape_ = Circle; setCursor(Qt::CrossCursor); }
  else if (shapeType == "Selection") { currentShape_ = Selection; setCursor(Qt::ArrowCursor); this->setDragMode(QGraphicsView::RubberBandDrag); }
  tempShapeItem_ = nullptr;
  if (currentShape_ != Eraser) {
    hideEraserPreview();
    for (auto item : scene_->items()) {
      if (item != eraserPreview_ && item != backgroundImage_ && item->type() != TransformHandleItem::Type) {
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
      }
    }
  }
  if (currentShape_ != Selection) {
    scene_->clearSelection();
    this->setDragMode(QGraphicsView::NoDrag);
    clearTransformHandles();
  }
  isPanning_ = false;
}

void Canvas::setPenTool() {
  currentShape_ = Pen; tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene_->clearSelection();
  setCursor(Qt::CrossCursor); isPanning_ = false;
}

void Canvas::setEraserTool() {
  currentShape_ = Eraser; tempShapeItem_ = nullptr;
  eraserPen_.setColor(backgroundColor_);
  this->setDragMode(QGraphicsView::NoDrag);
  scene_->clearSelection();
  for (auto item : scene_->items()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      item->setFlag(QGraphicsItem::ItemIsSelectable, false);
      item->setFlag(QGraphicsItem::ItemIsMovable, false);
    }
  }
  setCursor(Qt::BlankCursor); isPanning_ = false;
}

void Canvas::setTextTool() {
  currentShape_ = Text; tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene_->clearSelection();
  setCursor(Qt::IBeamCursor); isPanning_ = false;
}

void Canvas::setFillTool() {
  currentShape_ = Fill; tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene_->clearSelection();
  setCursor(Qt::PointingHandCursor); isPanning_ = false;
}

void Canvas::setArrowTool() {
  currentShape_ = Arrow; tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene_->clearSelection();
  setCursor(Qt::CrossCursor); isPanning_ = false;
}

void Canvas::setPanTool() {
  currentShape_ = Pan; tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene_->clearSelection();
  setCursor(Qt::OpenHandCursor); isPanning_ = false;
}

void Canvas::setPenColor(const QColor &color) {
  QColor newColor = color;
  newColor.setAlpha(currentOpacity_);
  currentPen_.setColor(newColor);
  emit colorChanged(color);
}

void Canvas::setOpacity(int opacity) {
  currentOpacity_ = qBound(0, opacity, 255);
  QColor color = currentPen_.color();
  color.setAlpha(currentOpacity_);
  currentPen_.setColor(color);
  emit opacityChanged(currentOpacity_);
}

void Canvas::increaseBrushSize() {
  if (currentPen_.width() < MAX_BRUSH_SIZE) {
    int newSize = currentPen_.width() + BRUSH_SIZE_STEP;
    currentPen_.setWidth(newSize);
    eraserPen_.setWidth(eraserPen_.width() + BRUSH_SIZE_STEP);
    if (currentShape_ == Eraser && eraserPreview_)
      eraserPreview_->setRect(eraserPreview_->rect().x(), eraserPreview_->rect().y(), eraserPen_.width(), eraserPen_.width());
    emit brushSizeChanged(newSize);
  }
}

void Canvas::decreaseBrushSize() {
  if (currentPen_.width() > MIN_BRUSH_SIZE) {
    int newSize = qMax(currentPen_.width() - BRUSH_SIZE_STEP, MIN_BRUSH_SIZE);
    currentPen_.setWidth(newSize);
    eraserPen_.setWidth(qMax(eraserPen_.width() - BRUSH_SIZE_STEP, MIN_BRUSH_SIZE));
    if (currentShape_ == Eraser && eraserPreview_)
      eraserPreview_->setRect(eraserPreview_->rect().x(), eraserPreview_->rect().y(), eraserPen_.width(), eraserPen_.width());
    emit brushSizeChanged(newSize);
  }
}

void Canvas::clearCanvas() {
  // Ask for confirmation if there are drawable items on the canvas
  // (excluding system items like eraser preview and background image)
  int drawableItemCount = 0;
  for (auto item : scene_->items()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      drawableItemCount++;
    }
  }
  
  if (drawableItemCount > 0) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Canvas",
        "Are you sure you want to clear the canvas? This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) {
      return;
    }
  }
  
  scene_->clear();
  undoStack_.clear();
  redoStack_.clear();
  backgroundImage_ = nullptr;
  scene_->setBackgroundBrush(backgroundColor_);
  eraserPreview_ = scene_->addEllipse(0, 0, eraserPen_.width(), eraserPen_.width(), QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview_->setZValue(1000);
  eraserPreview_->hide();
}

void Canvas::newCanvas(int width, int height, const QColor &bgColor) {
  clearCanvas();
  backgroundColor_ = bgColor;
  eraserPen_.setColor(backgroundColor_);
  scene_->setSceneRect(0, 0, width, height);
  scene_->setBackgroundBrush(backgroundColor_);
  resetTransform();
  currentZoom_ = 1.0;
  emit zoomChanged(100.0);
}

void Canvas::undoLastAction() {
  if (!undoStack_.empty()) {
    std::unique_ptr<Action> lastAction = std::move(undoStack_.back());
    undoStack_.pop_back();
    lastAction->undo();
    redoStack_.push_back(std::move(lastAction));
  }
}

void Canvas::redoLastAction() {
  if (!redoStack_.empty()) {
    std::unique_ptr<Action> nextAction = std::move(redoStack_.back());
    redoStack_.pop_back();
    nextAction->redo();
    undoStack_.push_back(std::move(nextAction));
  }
}

void Canvas::zoomIn() { applyZoom(ZOOM_FACTOR); }
void Canvas::zoomOut() { applyZoom(1.0 / ZOOM_FACTOR); }
void Canvas::zoomReset() { resetTransform(); currentZoom_ = 1.0; emit zoomChanged(100.0); }

void Canvas::applyZoom(double factor) {
  double newZoom = currentZoom_ * factor;
  if (newZoom > MAX_ZOOM || newZoom < MIN_ZOOM) return;
  currentZoom_ = newZoom;
  scale(factor, factor);
  emit zoomChanged(currentZoom_ * 100.0);
}

void Canvas::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    event->angleDelta().y() > 0 ? zoomIn() : zoomOut();
    event->accept();
  } else QGraphicsView::wheelEvent(event);
}

void Canvas::toggleGrid() {
  showGrid_ = !showGrid_;
  viewport()->update();
  scene_->invalidate(scene_->sceneRect(), QGraphicsScene::BackgroundLayer);
}

void Canvas::toggleFilledShapes() {
  fillShapes_ = !fillShapes_;
  emit filledShapesChanged(fillShapes_);
}

void Canvas::toggleSnapToGrid() {
  snapToGrid_ = !snapToGrid_;
  emit snapToGridChanged(snapToGrid_);
}

void Canvas::toggleRuler() {
  showRuler_ = !showRuler_;
  viewport()->update();
  emit rulerVisibilityChanged(showRuler_);
}

void Canvas::toggleMeasurementTool() {
  measurementToolEnabled_ = !measurementToolEnabled_;
  emit measurementToolChanged(measurementToolEnabled_);
}

void Canvas::lockSelectedItems() {
  for (QGraphicsItem *item : scene_->selectedItems()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      item->setFlag(QGraphicsItem::ItemIsMovable, false);
      item->setFlag(QGraphicsItem::ItemIsSelectable, false);
      // Store lock state in data
      item->setData(0, "locked");
    }
  }
  scene_->clearSelection();
}

void Canvas::unlockSelectedItems() {
  // Unlock all locked items (since they can't be selected when locked)
  for (QGraphicsItem *item : scene_->items()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      if (item->data(0).toString() == "locked") {
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setData(0, QVariant());
      }
    }
  }
}

QString Canvas::calculateDistance(const QPointF &p1, const QPointF &p2) const {
  qreal dx = p2.x() - p1.x();
  qreal dy = p2.y() - p1.y();
  qreal distance = std::sqrt(dx * dx + dy * dy);
  return QString("%1 px").arg(QString::number(distance, 'f', 1));
}

void Canvas::drawForeground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawForeground(painter, rect);
  if (showRuler_) {
    drawRuler(painter, rect);
  }
}

void Canvas::drawRuler(QPainter *painter, const QRectF &rect) {
  // Get viewport rect in scene coordinates
  QRectF viewRect = mapToScene(viewport()->rect()).boundingRect();
  
  // Save painter state
  painter->save();
  
  // Draw horizontal ruler background
  painter->fillRect(QRectF(viewRect.left(), viewRect.top(), viewRect.width(), RULER_SIZE), QColor(50, 50, 50, 200));
  
  // Draw vertical ruler background
  painter->fillRect(QRectF(viewRect.left(), viewRect.top(), RULER_SIZE, viewRect.height()), QColor(50, 50, 50, 200));
  
  // Setup pen and font for ruler markings
  painter->setPen(QPen(Qt::white, 1));
  QFont rulerFont;
  rulerFont.setPixelSize(9);
  painter->setFont(rulerFont);
  
  // Calculate tick spacing based on zoom
  int majorTickSpacing = GRID_SIZE * 5; // Major tick every 100px at 100% zoom
  int minorTickSpacing = GRID_SIZE; // Minor tick every 20px
  
  // Draw horizontal ruler ticks
  qreal startX = std::floor(viewRect.left() / minorTickSpacing) * minorTickSpacing;
  for (qreal x = startX; x < viewRect.right(); x += minorTickSpacing) {
    bool isMajor = (static_cast<int>(x) % majorTickSpacing) == 0;
    qreal tickHeight = isMajor ? RULER_SIZE * 0.6 : RULER_SIZE * 0.3;
    painter->drawLine(QPointF(x, viewRect.top() + RULER_SIZE - tickHeight), 
                      QPointF(x, viewRect.top() + RULER_SIZE));
    if (isMajor) {
      painter->drawText(QPointF(x + 2, viewRect.top() + RULER_SIZE * 0.5), QString::number(static_cast<int>(x)));
    }
  }
  
  // Draw vertical ruler ticks
  qreal startY = std::floor(viewRect.top() / minorTickSpacing) * minorTickSpacing;
  for (qreal y = startY; y < viewRect.bottom(); y += minorTickSpacing) {
    bool isMajor = (static_cast<int>(y) % majorTickSpacing) == 0;
    qreal tickWidth = isMajor ? RULER_SIZE * 0.6 : RULER_SIZE * 0.3;
    painter->drawLine(QPointF(viewRect.left() + RULER_SIZE - tickWidth, y), 
                      QPointF(viewRect.left() + RULER_SIZE, y));
    if (isMajor) {
      painter->save();
      painter->translate(viewRect.left() + RULER_SIZE * 0.4, y + 2);
      painter->rotate(90);
      painter->drawText(0, 0, QString::number(static_cast<int>(y)));
      painter->restore();
    }
  }
  
  // Draw corner square
  painter->fillRect(QRectF(viewRect.left(), viewRect.top(), RULER_SIZE, RULER_SIZE), QColor(70, 70, 70, 200));
  
  painter->restore();
}

QPointF Canvas::snapToGridPoint(const QPointF &point) const {
  if (!snapToGrid_) return point;
  
  qreal x = qRound(point.x() / GRID_SIZE) * GRID_SIZE;
  qreal y = qRound(point.y() / GRID_SIZE) * GRID_SIZE;
  return QPointF(x, y);
}

QPointF Canvas::calculateSmartDuplicateOffset() const {
  // Calculate smart offset based on selected items and available space
  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty()) return QPointF(GRID_SIZE, GRID_SIZE);
  
  // Get bounding rect of selected items
  QRectF boundingRect;
  for (QGraphicsItem *item : selected) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      if (boundingRect.isEmpty()) {
        boundingRect = item->sceneBoundingRect();
      } else {
        boundingRect = boundingRect.united(item->sceneBoundingRect());
      }
    }
  }
  
  // Use grid-aligned offset that's at least as large as the item + some padding
  qreal gridSize = static_cast<qreal>(GRID_SIZE);
  qreal offsetX = std::max(gridSize, std::ceil(boundingRect.width() / gridSize) * gridSize + gridSize);
  qreal offsetY = gridSize;
  
  // If the offset would go beyond scene width, move down instead
  if (boundingRect.right() + offsetX > scene_->sceneRect().right()) {
    offsetX = gridSize;
    offsetY = std::max(gridSize, std::ceil(boundingRect.height() / gridSize) * gridSize + gridSize);
  }
  
  return QPointF(offsetX, offsetY);
}

void Canvas::selectAll() {
  for (auto item : scene_->items())
    if (item != eraserPreview_ && item != backgroundImage_) item->setSelected(true);
}

void Canvas::deleteSelectedItems() {
  for (QGraphicsItem *item : scene_->selectedItems()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      addDeleteAction(item);
      scene_->removeItem(item);
    }
  }
}

void Canvas::duplicateSelectedItems() {
  QList<QGraphicsItem *> newItems;
  QPointF offset = calculateSmartDuplicateOffset();
  
  for (QGraphicsItem *item : scene_->selectedItems()) {
    if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) {
      auto n = new QGraphicsRectItem(r->rect()); n->setPen(r->pen()); n->setBrush(r->brush());
      QPointF newPos = snapToGrid_ ? snapToGridPoint(r->pos() + offset) : r->pos() + offset;
      n->setPos(newPos); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene_->addItem(n); newItems.append(n); addDrawAction(n);
    } else if (auto e = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      if (item == eraserPreview_) continue;
      auto n = new QGraphicsEllipseItem(e->rect()); n->setPen(e->pen()); n->setBrush(e->brush());
      QPointF newPos = snapToGrid_ ? snapToGridPoint(e->pos() + offset) : e->pos() + offset;
      n->setPos(newPos); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene_->addItem(n); newItems.append(n); addDrawAction(n);
    } else if (auto l = dynamic_cast<QGraphicsLineItem *>(item)) {
      auto n = new QGraphicsLineItem(l->line()); n->setPen(l->pen());
      QPointF newPos = snapToGrid_ ? snapToGridPoint(l->pos() + offset) : l->pos() + offset;
      n->setPos(newPos); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene_->addItem(n); newItems.append(n); addDrawAction(n);
    } else if (auto p = dynamic_cast<QGraphicsPathItem *>(item)) {
      auto n = new QGraphicsPathItem(p->path()); n->setPen(p->pen());
      QPointF newPos = snapToGrid_ ? snapToGridPoint(p->pos() + offset) : p->pos() + offset;
      n->setPos(newPos); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene_->addItem(n); newItems.append(n); addDrawAction(n);
    } else if (auto lt = dynamic_cast<LatexTextItem *>(item)) {
      auto n = new LatexTextItem(); n->setText(lt->text()); n->setFont(lt->font()); n->setTextColor(lt->textColor());
      QPointF newPos = snapToGrid_ ? snapToGridPoint(lt->pos() + offset) : lt->pos() + offset;
      n->setPos(newPos);
      scene_->addItem(n); newItems.append(n); addDrawAction(n);
    } else if (auto t = dynamic_cast<QGraphicsTextItem *>(item)) {
      auto n = new QGraphicsTextItem(t->toPlainText()); n->setFont(t->font()); n->setDefaultTextColor(t->defaultTextColor());
      QPointF newPos = snapToGrid_ ? snapToGridPoint(t->pos() + offset) : t->pos() + offset;
      n->setPos(newPos); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene_->addItem(n); newItems.append(n); addDrawAction(n);
    }
  }
  scene_->clearSelection();
  for (auto i : newItems) i->setSelected(true);
}

void Canvas::saveToFile() {
  QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp);;PDF (*.pdf)");
  if (fileName.isEmpty()) return;
  
  // Check if saving as PDF - pass the filename to avoid double dialog
  if (fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
    exportToPDFWithFilename(fileName);
    return;
  }
  
  bool ev = eraserPreview_ && eraserPreview_->isVisible();
  if (eraserPreview_) eraserPreview_->hide();
  scene_->clearSelection();
  QRectF sr = scene_->itemsBoundingRect();
  if (sr.isEmpty()) sr = scene_->sceneRect();
  sr.adjust(-10, -10, 10, 10);
  QImage img(sr.size().toSize(), QImage::Format_ARGB32);
  img.fill(backgroundColor_);
  QPainter p(&img); p.setRenderHint(QPainter::Antialiasing); p.setRenderHint(QPainter::TextAntialiasing);
  scene_->render(&p, QRectF(), sr); p.end();
  img.save(fileName);
  if (ev && eraserPreview_) eraserPreview_->show();
  
  // Add to recent files
  RecentFilesManager::instance().addRecentFile(fileName);
}

void Canvas::openFile() {
  QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif);;All (*)");
  if (fileName.isEmpty()) return;
  QPixmap pm(fileName);
  if (pm.isNull()) return;
  if (backgroundImage_) { scene_->removeItem(backgroundImage_); delete backgroundImage_; }
  backgroundImage_ = scene_->addPixmap(pm);
  backgroundImage_->setZValue(-1000);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsSelectable, false);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsMovable, false);
  scene_->setSceneRect(0, 0, qMax(scene_->sceneRect().width(), (qreal)pm.width()), qMax(scene_->sceneRect().height(), (qreal)pm.height()));
  
  // Add to recent files
  RecentFilesManager::instance().addRecentFile(fileName);
}

void Canvas::openRecentFile(const QString &filePath) {
  if (filePath.isEmpty()) return;
  QPixmap pm(filePath);
  if (pm.isNull()) {
    QMessageBox::warning(this, "Error", QString("Could not open file: %1").arg(filePath));
    return;
  }
  if (backgroundImage_) { scene_->removeItem(backgroundImage_); delete backgroundImage_; }
  backgroundImage_ = scene_->addPixmap(pm);
  backgroundImage_->setZValue(-1000);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsSelectable, false);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsMovable, false);
  scene_->setSceneRect(0, 0, qMax(scene_->sceneRect().width(), (qreal)pm.width()), qMax(scene_->sceneRect().height(), (qreal)pm.height()));
  
  // Update recent files
  RecentFilesManager::instance().addRecentFile(filePath);
}

void Canvas::exportToPDF() {
  QString fileName = QFileDialog::getSaveFileName(this, "Export to PDF", "", "PDF (*.pdf)");
  if (fileName.isEmpty()) return;
  exportToPDFWithFilename(fileName);
}

void Canvas::exportToPDFWithFilename(const QString &fileName) {
  bool ev = eraserPreview_ && eraserPreview_->isVisible();
  if (eraserPreview_) eraserPreview_->hide();
  scene_->clearSelection();
  
  QRectF sr = scene_->itemsBoundingRect();
  if (sr.isEmpty()) sr = scene_->sceneRect();
  sr.adjust(-10, -10, 10, 10);
  
  // Create PDF writer with A4 page size as default
  QPdfWriter pdfWriter(fileName);
  
  // Use A4 page size for reasonable dimensions
  pdfWriter.setPageSize(QPageSize::A4);
  pdfWriter.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);
  pdfWriter.setTitle("FullScreen Pencil Draw Export");
  pdfWriter.setCreator("FullScreen Pencil Draw");
  
  // Calculate resolution to maintain quality
  int dpi = 300;
  pdfWriter.setResolution(dpi);
  
  QPainter painter(&pdfWriter);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  
  // Fill background
  painter.fillRect(painter.viewport(), backgroundColor_);
  
  // Calculate scale factor to fit the scene onto the page while maintaining aspect ratio
  QRectF pageRect = painter.viewport();
  double scaleX = pageRect.width() / sr.width();
  double scaleY = pageRect.height() / sr.height();
  double scale = qMin(scaleX, scaleY);
  
  // Center the content on the page
  double offsetX = (pageRect.width() - sr.width() * scale) / 2.0;
  double offsetY = (pageRect.height() - sr.height() * scale) / 2.0;
  
  painter.translate(offsetX, offsetY);
  painter.scale(scale, scale);
  painter.translate(-sr.topLeft());
  
  scene_->render(&painter, QRectF(), sr);
  painter.end();
  
  if (ev && eraserPreview_) eraserPreview_->show();
  
  // Add to recent files
  RecentFilesManager::instance().addRecentFile(fileName);
}

void Canvas::createTextItem(const QPointF &pos) {
  // Create text item with inline editing
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
    // If the text is empty after editing, remove the item
    if (textItem->text().trimmed().isEmpty()) {
      scene_->removeItem(textItem);
      textItem->deleteLater();
    } else {
      // Add to undo stack only when there's actual content
      addDrawAction(textItem);
    }
  });

  // Start inline editing immediately
  textItem->startEditing();
}

void Canvas::fillAt(const QPointF &point) {
  QBrush newBrush(currentPen_.color());
  for (QGraphicsItem *item : scene_->items(point)) {
    if (item == eraserPreview_ || item == backgroundImage_) continue;
    if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) {
      QBrush oldBrush = r->brush();
      r->setBrush(newBrush);
      addAction(std::make_unique<FillAction>(item, oldBrush, newBrush));
      return;
    }
    if (auto e = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      QBrush oldBrush = e->brush();
      e->setBrush(newBrush);
      addAction(std::make_unique<FillAction>(item, oldBrush, newBrush));
      return;
    }
    if (auto p = dynamic_cast<QGraphicsPolygonItem *>(item)) {
      QBrush oldBrush = p->brush();
      p->setBrush(newBrush);
      addAction(std::make_unique<FillAction>(item, oldBrush, newBrush));
      return;
    }
  }
}

void Canvas::drawArrow(const QPointF &start, const QPointF &end) {
  auto li = new QGraphicsLineItem(QLineF(start, end));
  li->setPen(currentPen_);
  li->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
  scene_->addItem(li);
  double angle = std::atan2(-(end.y() - start.y()), end.x() - start.x());
  double sz = currentPen_.width() * 4;
  QPolygonF ah; ah << end << end + QPointF(std::sin(angle + M_PI/3)*sz, std::cos(angle + M_PI/3)*sz)
                    << end + QPointF(std::sin(angle + M_PI - M_PI/3)*sz, std::cos(angle + M_PI - M_PI/3)*sz);
  auto ahi = new QGraphicsPolygonItem(ah);
  ahi->setPen(currentPen_); ahi->setBrush(currentPen_.color());
  ahi->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
  scene_->addItem(ahi);
  addDrawAction(li);
  addDrawAction(ahi);
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  QPointF sp = mapToScene(event->pos());
  // Apply snap-to-grid for shape tools
  if (snapToGrid_ && (currentShape_ == Rectangle || currentShape_ == Circle || 
                       currentShape_ == Line || currentShape_ == Arrow)) {
    sp = snapToGridPoint(sp);
  }
  emit cursorPositionChanged(sp);
  if (currentShape_ == Selection) { QGraphicsView::mousePressEvent(event); return; }
  if (currentShape_ == Pan) { isPanning_ = true; lastPanPoint_ = event->pos(); setCursor(Qt::ClosedHandCursor); return; }
  startPoint_ = sp;
  switch (currentShape_) {
  case Text: createTextItem(snapToGrid_ ? snapToGridPoint(sp) : sp); break;
  case Fill: fillAt(sp); break;
  case Eraser: eraseAt(sp); break;
  case Pen: {
    currentPath_ = new QGraphicsPathItem();
    currentPath_->setPen(currentPen_);
    currentPath_->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    QPainterPath p; p.moveTo(sp); currentPath_->setPath(p);
    scene_->addItem(currentPath_);
    pointBuffer_.clear(); pointBuffer_.append(sp);
    addDrawAction(currentPath_);
  } break;
  case Arrow: case Rectangle: {
    auto ri = new QGraphicsRectItem(QRectF(startPoint_, startPoint_));
    ri->setPen(currentPen_);
    // Only fill rectangles, not the preview rect used for Arrow tool
    if (fillShapes_ && currentShape_ == Rectangle) ri->setBrush(currentPen_.color());
    ri->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    scene_->addItem(ri); tempShapeItem_ = ri;
    if (currentShape_ == Rectangle) { addDrawAction(ri); }
  } break;
  case Circle: {
    auto ei = new QGraphicsEllipseItem(QRectF(startPoint_, startPoint_));
    ei->setPen(currentPen_);
    if (fillShapes_) ei->setBrush(currentPen_.color());
    ei->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    scene_->addItem(ei); tempShapeItem_ = ei;
    addDrawAction(ei);
  } break;
  case Line: {
    auto li = new QGraphicsLineItem(QLineF(startPoint_, startPoint_));
    li->setPen(currentPen_); li->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    scene_->addItem(li); tempShapeItem_ = li;
    addDrawAction(li);
  } break;
  default: QGraphicsView::mousePressEvent(event); break;
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  QPointF cp = mapToScene(event->pos());
  // Apply snap-to-grid for shape tools during drawing
  if (snapToGrid_ && (currentShape_ == Rectangle || currentShape_ == Circle || 
                       currentShape_ == Line || currentShape_ == Arrow)) {
    cp = snapToGridPoint(cp);
  }
  emit cursorPositionChanged(cp);
  
  // Emit measurement when drawing shapes (only when we have a valid start point)
  if (measurementToolEnabled_ && (event->buttons() & Qt::LeftButton) && tempShapeItem_) {
    if (currentShape_ == Rectangle || currentShape_ == Circle || 
        currentShape_ == Line || currentShape_ == Arrow) {
      emit measurementUpdated(calculateDistance(startPoint_, cp));
    }
  }
  
  if (currentShape_ == Selection) { QGraphicsView::mouseMoveEvent(event); return; }
  if (currentShape_ == Pan && isPanning_) {
    QPointF d = event->pos() - lastPanPoint_; lastPanPoint_ = event->pos();
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
    return;
  }
  switch (currentShape_) {
  case Pen: if (event->buttons() & Qt::LeftButton) addPoint(cp); break;
  case Eraser: if (event->buttons() & Qt::LeftButton) eraseAt(cp); updateEraserPreview(cp); break;
  case Arrow: case Rectangle: if (tempShapeItem_) static_cast<QGraphicsRectItem*>(tempShapeItem_)->setRect(QRectF(startPoint_, cp).normalized()); break;
  case Circle: if (tempShapeItem_) static_cast<QGraphicsEllipseItem*>(tempShapeItem_)->setRect(QRectF(startPoint_, cp).normalized()); break;
  case Line: if (tempShapeItem_) static_cast<QGraphicsLineItem*>(tempShapeItem_)->setLine(QLineF(startPoint_, cp)); break;
  default: QGraphicsView::mouseMoveEvent(event); break;
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  QPointF ep = mapToScene(event->pos());
  // Apply snap-to-grid for shape tools
  if (snapToGrid_ && (currentShape_ == Rectangle || currentShape_ == Circle || 
                       currentShape_ == Line || currentShape_ == Arrow)) {
    ep = snapToGridPoint(ep);
  }
  if (currentShape_ == Selection) { QGraphicsView::mouseReleaseEvent(event); return; }
  if (currentShape_ == Pan) { isPanning_ = false; setCursor(Qt::OpenHandCursor); return; }
  if (currentShape_ == Arrow && tempShapeItem_) {
    scene_->removeItem(tempShapeItem_); delete tempShapeItem_; tempShapeItem_ = nullptr;
    drawArrow(startPoint_, ep); return;
  }
  if (currentShape_ != Pen && currentShape_ != Eraser && tempShapeItem_) tempShapeItem_ = nullptr;
  else if (currentShape_ == Pen) { currentPath_ = nullptr; pointBuffer_.clear(); }
}

void Canvas::updateEraserPreview(const QPointF &pos) {
  if (!eraserPreview_) return;
  qreal r = eraserPen_.width() / 2.0;
  eraserPreview_->setRect(pos.x() - r, pos.y() - r, eraserPen_.width(), eraserPen_.width());
  if (!eraserPreview_->isVisible()) eraserPreview_->show();
}

void Canvas::hideEraserPreview() { if (eraserPreview_) eraserPreview_->hide(); }

void Canvas::copySelectedItems() {
  auto sel = scene_->selectedItems();
  if (sel.isEmpty()) return;
  auto md = new QMimeData();
  QByteArray ba; QDataStream ds(&ba, QIODevice::WriteOnly);
  for (auto item : sel) {
    if (auto r = dynamic_cast<QGraphicsRectItem*>(item)) { ds << QString("Rectangle") << r->rect() << r->pos() << r->pen() << r->brush(); }
    else if (auto e = dynamic_cast<QGraphicsEllipseItem*>(item)) { if (item == eraserPreview_) continue; ds << QString("Ellipse") << e->rect() << e->pos() << e->pen() << e->brush(); }
    else if (auto l = dynamic_cast<QGraphicsLineItem*>(item)) { ds << QString("Line") << l->line() << l->pos() << l->pen(); }
    else if (auto p = dynamic_cast<QGraphicsPathItem*>(item)) { ds << QString("Path") << p->path() << p->pos() << p->pen(); }
    else if (auto lt = dynamic_cast<LatexTextItem*>(item)) { ds << QString("LatexText") << lt->text() << lt->pos() << lt->font() << lt->textColor(); }
    else if (auto t = dynamic_cast<QGraphicsTextItem*>(item)) { ds << QString("Text") << t->toPlainText() << t->pos() << t->font() << t->defaultTextColor(); }
    else if (auto pg = dynamic_cast<QGraphicsPolygonItem*>(item)) { ds << QString("Polygon") << pg->polygon() << pg->pos() << pg->pen() << pg->brush(); }
  }
  md->setData("application/x-canvas-items", ba);
  QApplication::clipboard()->setMimeData(md);
}

void Canvas::cutSelectedItems() {
  auto sel = scene_->selectedItems();
  if (sel.isEmpty()) return;
  copySelectedItems();
  for (auto item : sel) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      addDeleteAction(item);
      scene_->removeItem(item);
    }
  }
}

void Canvas::pasteItems() {
  auto md = QApplication::clipboard()->mimeData();
  if (!md->hasFormat("application/x-canvas-items")) return;
  QByteArray ba = md->data("application/x-canvas-items");
  QDataStream ds(&ba, QIODevice::ReadOnly);
  QList<QGraphicsItem*> pi;
  while (!ds.atEnd()) {
    QString t; ds >> t;
    if (t == "Rectangle") { QRectF r; QPointF p; QPen pn; QBrush b; ds >> r >> p >> pn >> b; auto n = new QGraphicsRectItem(r); n->setPen(pn); n->setBrush(b); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene_->addItem(n); pi.append(n); addDrawAction(n); }
    else if (t == "Ellipse") { QRectF r; QPointF p; QPen pn; QBrush b; ds >> r >> p >> pn >> b; auto n = new QGraphicsEllipseItem(r); n->setPen(pn); n->setBrush(b); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene_->addItem(n); pi.append(n); addDrawAction(n); }
    else if (t == "Line") { QLineF l; QPointF p; QPen pn; ds >> l >> p >> pn; auto n = new QGraphicsLineItem(l); n->setPen(pn); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene_->addItem(n); pi.append(n); addDrawAction(n); }
    else if (t == "Path") { QPainterPath pp; QPointF p; QPen pn; ds >> pp >> p >> pn; auto n = new QGraphicsPathItem(pp); n->setPen(pn); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene_->addItem(n); pi.append(n); addDrawAction(n); }
    else if (t == "LatexText") { QString tx; QPointF p; QFont f; QColor c; ds >> tx >> p >> f >> c; auto n = new LatexTextItem(); n->setText(tx); n->setFont(f); n->setTextColor(c); n->setPos(p + QPointF(20,20)); scene_->addItem(n); pi.append(n); addDrawAction(n); }
    else if (t == "Text") { QString tx; QPointF p; QFont f; QColor c; ds >> tx >> p >> f >> c; auto n = new QGraphicsTextItem(tx); n->setFont(f); n->setDefaultTextColor(c); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene_->addItem(n); pi.append(n); addDrawAction(n); }
    else if (t == "Polygon") { QPolygonF pg; QPointF p; QPen pn; QBrush b; ds >> pg >> p >> pn >> b; auto n = new QGraphicsPolygonItem(pg); n->setPen(pn); n->setBrush(b); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene_->addItem(n); pi.append(n); addDrawAction(n); }
  }
  scene_->clearSelection();
  for (auto i : pi) i->setSelected(true);
}

void Canvas::addPoint(const QPointF &point) {
  if (!currentPath_) return;
  pointBuffer_.append(point);
  const int mpr = 4;
  if (pointBuffer_.size() >= mpr) {
    QPointF p0 = pointBuffer_.at(pointBuffer_.size() - mpr);
    QPointF p1 = pointBuffer_.at(pointBuffer_.size() - mpr + 1);
    QPointF p2 = pointBuffer_.at(pointBuffer_.size() - mpr + 2);
    QPointF p3 = pointBuffer_.at(pointBuffer_.size() - mpr + 3);
    QPainterPath path = currentPath_->path();
    path.cubicTo(p1 + (p2 - p0) / 6.0, p2 - (p3 - p1) / 6.0, p2);
    currentPath_->setPath(path);
  }
  if (pointBuffer_.size() > mpr) pointBuffer_.removeFirst();
}

void Canvas::eraseAt(const QPointF &point) {
  qreal sz = eraserPen_.width();
  QRectF er(point.x() - sz/2, point.y() - sz/2, sz, sz);
  QPainterPath ep; ep.addEllipse(er);
  for (QGraphicsItem *item : scene_->items(er)) {
    if (item == eraserPreview_ || item == backgroundImage_) continue;
    QPainterPathStroker s; s.setWidth(1);
    if (ep.intersects(s.createStroke(item->shape()))) {
      addDeleteAction(item);
      scene_->removeItem(item);
    }
  }
}

void Canvas::dragEnterEvent(QDragEnterEvent *event) {
  // Accept the drag if it contains URLs (files)
  if (event->mimeData()->hasUrls()) {
    dragAccepted_ = true;
    event->acceptProposedAction();
    return;
  }
  dragAccepted_ = false;
  QGraphicsView::dragEnterEvent(event);
}

void Canvas::dragMoveEvent(QDragMoveEvent *event) {
  // Accept the drag move if it contains URLs (files)
  if (dragAccepted_ && event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
    return;
  }
  QGraphicsView::dragMoveEvent(event);
}

void Canvas::dragLeaveEvent(QDragLeaveEvent *event) {
  if (dragAccepted_) {
    dragAccepted_ = false;
    event->accept();
    return;
  }
  QGraphicsView::dragLeaveEvent(event);
}

void Canvas::dropEvent(QDropEvent *event) {
  // Handle the dropped files
  const QMimeData *mimeData = event->mimeData();

  if (mimeData->hasUrls()) {
    dragAccepted_ = false;
    QList<QUrl> urls = mimeData->urls();
    
    // Process each dropped file
    for (const QUrl &url : urls) {
      if (url.isLocalFile()) {
        QString filePath = url.toLocalFile();
        
        // Extract file extension using QFileInfo for robust handling
        QString extension = QFileInfo(filePath).suffix().toLower();
        
        // Check if the file is a PDF - emit signal to open in PDF viewer
        if (extension == "pdf") {
          emit pdfFileDropped(filePath);
          event->acceptProposedAction();
          return;
        }
        // Check if the file is a supported image format
        else if (SUPPORTED_IMAGE_EXTENSIONS.contains(extension)) {
          // Get the drop position in scene coordinates
          QPointF dropPosition = mapToScene(event->position().toPoint());
          
          // Load the dropped image
          loadDroppedImage(filePath, dropPosition);
        } else {
          // Inform user about unsupported file type
          QMessageBox::warning(this, "Unsupported File", 
                             QString("File '%1' is not a supported format.\n\nSupported formats: PNG, JPG, JPEG, BMP, GIF, PDF")
                             .arg(QFileInfo(filePath).fileName()));
        }
      }
    }
    
    event->acceptProposedAction();
    return;
  }

  QGraphicsView::dropEvent(event);
}

void Canvas::loadDroppedImage(const QString &filePath, const QPointF &dropPosition) {
  // Load the image
  QPixmap pixmap(filePath);
  
  if (pixmap.isNull()) {
    // Show error message for invalid image
    QMessageBox::warning(this, "Invalid Image", 
                       QString("Failed to load image from '%1'.\n\nThe file may be corrupted or not a valid image.")
                       .arg(QFileInfo(filePath).fileName()));
    return;
  }
  
  // Show dialog to specify dimensions
  ImageSizeDialog dialog(pixmap.width(), pixmap.height(), this);
  
  if (dialog.exec() == QDialog::Accepted) {
    int newWidth = dialog.getWidth();
    int newHeight = dialog.getHeight();
    
    // Scale the pixmap to the specified dimensions
    // Note: We use IgnoreAspectRatio because the dialog already handled
    // aspect ratio calculations, so we want exact dimensions specified by user
    QPixmap scaledPixmap = pixmap.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
    // Create a graphics pixmap item
    QGraphicsPixmapItem *pixmapItem = scene_->addPixmap(scaledPixmap);
    
    // Position the item at the drop location (centered)
    pixmapItem->setPos(dropPosition.x() - newWidth / 2.0, dropPosition.y() - newHeight / 2.0);
    
    // Make the item selectable and movable
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    
    // Add to undo stack
    addDrawAction(pixmapItem);
  }
}

void Canvas::addImageFromScreenshot(const QImage &image) {
  if (image.isNull()) {
    return;
  }
  
  // Convert QImage to QPixmap
  QPixmap pixmap = QPixmap::fromImage(image);
  
  // Create a graphics pixmap item
  QGraphicsPixmapItem *pixmapItem = scene_->addPixmap(pixmap);
  
  // Position the item at the center of the visible area
  QPointF centerPos = mapToScene(viewport()->rect().center());
  pixmapItem->setPos(centerPos.x() - pixmap.width() / 2.0, centerPos.y() - pixmap.height() / 2.0);
  
  // Make the item selectable and movable
  pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
  
  // Select the newly added item
  scene_->clearSelection();
  pixmapItem->setSelected(true);
  
  // Add to undo stack
  addDrawAction(pixmapItem);
  
  emit canvasModified();
}

void Canvas::contextMenuEvent(QContextMenuEvent *event) {
  // Only show context menu if there are selected items
  if (scene_->selectedItems().isEmpty()) {
    QGraphicsView::contextMenuEvent(event);
    return;
  }

  QMenu contextMenu(this);
  QAction *exportSVGAction = contextMenu.addAction("Export Selection as SVG");
  QAction *exportPNGAction = contextMenu.addAction("Export Selection as PNG");
  QAction *exportJPGAction = contextMenu.addAction("Export Selection as JPG");

#ifdef HAVE_QT_SVG
  connect(exportSVGAction, &QAction::triggered, this, &Canvas::exportSelectionToSVG);
#else
  exportSVGAction->setEnabled(false);
  exportSVGAction->setText("Export Selection as SVG (requires Qt SVG)");
#endif
  connect(exportPNGAction, &QAction::triggered, this, &Canvas::exportSelectionToPNG);
  connect(exportJPGAction, &QAction::triggered, this, &Canvas::exportSelectionToJPG);

  contextMenu.exec(event->globalPos());
}

QRectF Canvas::getSelectionBoundingRect() const {
  QList<QGraphicsItem*> selectedItems = scene_->selectedItems();
  if (selectedItems.isEmpty()) return QRectF();
  
  QRectF boundingRect;
  bool firstItem = true;
  for (QGraphicsItem *item : selectedItems) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      if (firstItem) {
        boundingRect = item->sceneBoundingRect();
        firstItem = false;
      } else {
        boundingRect = boundingRect.united(item->sceneBoundingRect());
      }
    }
  }
  boundingRect.adjust(-10, -10, 10, 10);
  return boundingRect;
}

void Canvas::exportSelectionToSVG() {
#ifndef HAVE_QT_SVG
  QMessageBox::information(this, "SVG Export Unavailable",
                           "SVG export requires the Qt SVG module, which was not found at build time.");
  return;
#else
  if (scene_->selectedItems().isEmpty()) return;

  QString fileName = QFileDialog::getSaveFileName(this, "Export Selection as SVG", "", "SVG (*.svg)");
  if (fileName.isEmpty()) return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty()) return;

  // Create SVG generator
  QSvgGenerator generator;
  generator.setFileName(fileName);
  generator.setSize(boundingRect.size().toSize());
  generator.setViewBox(QRect(0, 0, boundingRect.width(), boundingRect.height()));
  generator.setTitle("Exported Selection");
  generator.setDescription("Selected items exported from FullScreen Pencil Draw");

  // Render selected items to SVG
  QPainter painter;
  painter.begin(&generator);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());
  
  for (QGraphicsItem *item : scene_->selectedItems()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      painter.save();
      painter.setTransform(item->sceneTransform(), true);
      item->paint(&painter, nullptr, nullptr);
      painter.restore();
    }
  }
  
  painter.end();
#endif
}

void Canvas::exportSelectionToPNG() {
  if (scene_->selectedItems().isEmpty()) return;

  QString fileName = QFileDialog::getSaveFileName(this, "Export Selection as PNG", "", "PNG (*.png)");
  if (fileName.isEmpty()) return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty()) return;

  // Create image and render selected items
  QImage image(boundingRect.size().toSize(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());
  
  for (QGraphicsItem *item : scene_->selectedItems()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      painter.save();
      painter.setTransform(item->sceneTransform(), true);
      item->paint(&painter, nullptr, nullptr);
      painter.restore();
    }
  }
  
  painter.end();
  image.save(fileName);
}

void Canvas::exportSelectionToJPG() {
  if (scene_->selectedItems().isEmpty()) return;

  QString fileName = QFileDialog::getSaveFileName(this, "Export Selection as JPG", "", "JPEG (*.jpg)");
  if (fileName.isEmpty()) return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty()) return;

  // Create image with white background and render selected items
  QImage image(boundingRect.size().toSize(), QImage::Format_RGB32);
  image.fill(Qt::white);
  
  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());
  
  for (QGraphicsItem *item : scene_->selectedItems()) {
    if (item != eraserPreview_ && item != backgroundImage_) {
      painter.save();
      painter.setTransform(item->sceneTransform(), true);
      item->paint(&painter, nullptr, nullptr);
      painter.restore();
    }
  }
  
  painter.end();
  image.save(fileName);
}

void Canvas::updateTransformHandles() {
  // Only show transform handles when in selection mode
  if (currentShape_ != Selection) {
    clearTransformHandles();
    return;
  }
  
  QList<QGraphicsItem*> selectedItems = scene_->selectedItems();
  
  // Remove handles for items that are no longer selected
  QMutableListIterator<TransformHandleItem*> it(transformHandles_);
  while (it.hasNext()) {
    TransformHandleItem* handle = it.next();
    if (!selectedItems.contains(handle->targetItem())) {
      scene_->removeItem(handle);
      delete handle;
      it.remove();
    }
  }
  
  // Add handles for newly selected items
  for (QGraphicsItem* item : selectedItems) {
    // Skip non-transformable items
    if (item == eraserPreview_ || item == backgroundImage_)
      continue;
    
    // Skip TransformHandleItems themselves (check by type)
    if (item->type() == TransformHandleItem::Type)
      continue;
    
    // Check if handle already exists for this item
    bool hasHandle = false;
    for (TransformHandleItem* handle : transformHandles_) {
      if (handle->targetItem() == item) {
        hasHandle = true;
        handle->updateHandles();
        break;
      }
    }
    
    if (!hasHandle) {
      TransformHandleItem* handle = new TransformHandleItem(item, this);
      scene_->addItem(handle);
      transformHandles_.append(handle);
      
      // Connect to update handles when transform completes
      connect(handle, &TransformHandleItem::transformCompleted, this, [this, handle]() {
        handle->updateHandles();
        emit canvasModified();
      });
    }
  }
}

void Canvas::clearTransformHandles() {
  for (TransformHandleItem* handle : transformHandles_) {
    scene_->removeItem(handle);
    delete handle;
  }
  transformHandles_.clear();
}
