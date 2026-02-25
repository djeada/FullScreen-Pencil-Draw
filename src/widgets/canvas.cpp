/**
 * @file canvas.cpp
 * @brief Implementation of the main drawing canvas widget.
 */
#include "canvas.h"
#include "../core/fill_utils.h"
#include "../core/image_filters.h"
#include "../core/item_store.h"
#include "../core/project_serializer.h"
#include "../core/recent_files_manager.h"
#include "../core/scene_controller.h"
#include "../core/transform_action.h"
#include "architecture_elements.h"
#include "image_size_dialog.h"
#include "latex_text_item.h"
#include "mermaid_text_item.h"
#include "perspective_transform_dialog.h"
#include "resize_canvas_dialog.h"
#include "scale_dialog.h"
#include "transform_handle_item.h"
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QDebug>
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
#include <QPageSize>
#include <QPaintEvent>
#include <QPdfWriter>
#include <QPointer>
#include <QQueue>
#include <QScrollBar>
#include <QTabletEvent>
#include <algorithm>
#ifdef HAVE_QT_SVG
#include <QSvgGenerator>
#include <QSvgRenderer>
#endif
#include <QUrl>
#include <QWheelEvent>
#include <cmath>

// Pressure sensitivity constants
static constexpr qreal MIN_PRESSURE_THRESHOLD = 0.1;
static constexpr qreal MIN_POINT_DISTANCE = 1.0;
static constexpr qreal LENGTH_EPSILON = 0.001;
static constexpr int MAX_PRESSURE_BUFFER_SIZE = 200;
static constexpr int PRESSURE_BUFFER_TRIM_SIZE = 100;

// Supported image file extensions for drag-and-drop
static const QSet<QString> SUPPORTED_IMAGE_EXTENSIONS = {
    "png", "jpg", "jpeg", "bmp", "gif", "webp", "tiff", "tif"};

namespace {
constexpr qreal CURVED_ARROW_BASE_FACTOR = 0.28;
constexpr qreal CURVED_ARROW_MIN_OFFSET = 12.0;
constexpr qreal CURVED_ARROW_MAX_OFFSET = 240.0;
constexpr qreal CURVED_ARROW_MIN_LENGTH = 2.0;

qreal curvedArrowMagnitudeForModifiers(Qt::KeyboardModifiers modifiers) {
  qreal factor = CURVED_ARROW_BASE_FACTOR;
  if (modifiers & Qt::AltModifier)
    factor = 0.45;
  if (modifiers & Qt::ControlModifier)
    factor = 0.16;
  return factor;
}

QPointF curvedArrowControlPoint(const QPointF &start, const QPointF &end,
                                Qt::KeyboardModifiers modifiers) {
  const QPointF diff = end - start;
  const qreal len = std::hypot(diff.x(), diff.y());
  const QPointF mid = (start + end) / 2.0;

  if (len < CURVED_ARROW_MIN_LENGTH)
    return mid;

  qreal offset = qBound(CURVED_ARROW_MIN_OFFSET,
                        len * curvedArrowMagnitudeForModifiers(modifiers),
                        CURVED_ARROW_MAX_OFFSET);
  QPointF normal(diff.y() / len, -diff.x() / len);
  if (modifiers & Qt::ShiftModifier) {
    normal = QPointF(-normal.x(), -normal.y());
  }

  return QPointF(mid.x() + normal.x() * offset, mid.y() + normal.y() * offset);
}

QPainterPath curvedArrowPath(const QPointF &start, const QPointF &end,
                             Qt::KeyboardModifiers modifiers, qreal arrowSize) {
  QPainterPath path;
  path.moveTo(start);

  QPointF ctrl = curvedArrowControlPoint(start, end, modifiers);
  path.quadTo(ctrl, end);

  QPointF tangent = end - ctrl;
  if (std::hypot(tangent.x(), tangent.y()) < CURVED_ARROW_MIN_LENGTH) {
    tangent = end - start;
  }

  const qreal angle = std::atan2(tangent.y(), tangent.x());
  const QPointF wingA = end - QPointF(std::cos(angle - M_PI / 6.0) * arrowSize,
                                      std::sin(angle - M_PI / 6.0) * arrowSize);
  const QPointF wingB = end - QPointF(std::cos(angle + M_PI / 6.0) * arrowSize,
                                      std::sin(angle + M_PI / 6.0) * arrowSize);
  path.moveTo(end);
  path.lineTo(wingA);
  path.moveTo(end);
  path.lineTo(wingB);

  return path;
}

Qt::KeyboardModifiers
effectiveCurvedArrowModifiers(Qt::KeyboardModifiers rawModifiers,
                              bool autoBendEnabled, bool manualFlip) {
  Qt::KeyboardModifiers modifiers = rawModifiers;
  if (!autoBendEnabled) {
    modifiers &= ~Qt::ShiftModifier;
    if (manualFlip) {
      modifiers |= Qt::ShiftModifier;
    }
  }
  return modifiers;
}

constexpr int COLOR_SELECTION_MARK = 255;

inline int colorDistanceSquared(QRgb a, QRgb b) {
  const int dr = qRed(a) - qRed(b);
  const int dg = qGreen(a) - qGreen(b);
  const int db = qBlue(a) - qBlue(b);
  const int da = qAlpha(a) - qAlpha(b);
  return dr * dr + dg * dg + db * db + da * da;
}
} // namespace

// Helper function to clone a QGraphicsItem (excluding groups and pixmap items)
static void copyTransformState(QGraphicsItem *dst, const QGraphicsItem *src) {
  if (!dst || !src)
    return;
  dst->setTransformOriginPoint(src->transformOriginPoint());
  dst->setTransform(src->transform());
}

static QGraphicsItem *cloneItem(QGraphicsItem *item,
                                QGraphicsEllipseItem *eraserPreview) {
  if (!item)
    return nullptr;

  if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) {
    auto n = new QGraphicsRectItem(r->rect());
    n->setPen(r->pen());
    n->setBrush(r->brush());
    n->setPos(r->pos());
    copyTransformState(n, r);
    return n;
  } else if (auto e = dynamic_cast<QGraphicsEllipseItem *>(item)) {
    if (item == eraserPreview)
      return nullptr;
    auto n = new QGraphicsEllipseItem(e->rect());
    n->setPen(e->pen());
    n->setBrush(e->brush());
    n->setPos(e->pos());
    copyTransformState(n, e);
    return n;
  } else if (auto l = dynamic_cast<QGraphicsLineItem *>(item)) {
    auto n = new QGraphicsLineItem(l->line());
    n->setPen(l->pen());
    n->setPos(l->pos());
    copyTransformState(n, l);
    return n;
  } else if (auto p = dynamic_cast<QGraphicsPathItem *>(item)) {
    auto n = new QGraphicsPathItem(p->path());
    n->setPen(p->pen());
    n->setBrush(p->brush());
    n->setPos(p->pos());
    copyTransformState(n, p);
    return n;
  } else if (auto lt = dynamic_cast<LatexTextItem *>(item)) {
    auto n = new LatexTextItem();
    n->setText(lt->text());
    n->setFont(lt->font());
    n->setTextColor(lt->textColor());
    n->setPos(lt->pos());
    copyTransformState(n, lt);
    return n;
  } else if (auto t = dynamic_cast<QGraphicsTextItem *>(item)) {
    auto n = new QGraphicsTextItem(t->toPlainText());
    n->setFont(t->font());
    n->setDefaultTextColor(t->defaultTextColor());
    n->setPos(t->pos());
    copyTransformState(n, t);
    return n;
  } else if (auto pg = dynamic_cast<QGraphicsPolygonItem *>(item)) {
    auto n = new QGraphicsPolygonItem(pg->polygon());
    n->setPen(pg->pen());
    n->setBrush(pg->brush());
    n->setPos(pg->pos());
    copyTransformState(n, pg);
    return n;
  }
  return nullptr;
}

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent), scene_(new QGraphicsScene(this)),
      sceneController_(nullptr), layerManager_(nullptr),
      tempShapeItem_(nullptr), currentShape_(Pen), currentPen_(Qt::white, 3),
      eraserPen_(Qt::black, 10), currentPath_(nullptr),
      backgroundColor_(Qt::black), eraserPreview_(nullptr),
      backgroundImage_(nullptr), isPanning_(false),
      snapEngine_(GRID_SIZE, 10.0) {

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

  // Initialize scene controller (single source of truth)
  sceneController_ = new SceneController(scene_, this);

  // Initialize layer manager
  layerManager_ = new LayerManager(scene_, this);
  layerManager_->setSceneController(sceneController_);
  layerManager_->setItemStore(sceneController_->itemStore());
  sceneController_->setLayerManager(layerManager_);
  connect(layerManager_, &LayerManager::layerRemoved, this,
          [this](Layer * /*layer*/) { clearTransformHandles(); });

  if (sceneController_ && sceneController_->itemStore()) {
    connect(sceneController_->itemStore(), &ItemStore::itemAboutToBeDeleted,
            this, [this](const ItemId &id) {
              QMutableListIterator<TransformHandleItem *> it(transformHandles_);
              while (it.hasNext()) {
                TransformHandleItem *handle = it.next();
                if (!handle) {
                  it.remove();
                  continue;
                }
                if (handle->targetItemId().isValid() &&
                    handle->targetItemId() == id) {
                  handle->clearTargetItem();
                  if (scene_)
                    scene_->removeItem(handle);
                  delete handle;
                  it.remove();
                }
              }
            });
  }

  eraserPreview_ =
      scene_->addEllipse(0, 0, eraserPen_.width(), eraserPen_.width(),
                         QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview_->setZValue(1000);
  eraserPreview_->hide();

  this->setMouseTracking(true);
  // Use NoIndex to avoid potential issues with BSP tree and deleted items
  scene_->setItemIndexMethod(QGraphicsScene::NoIndex);

  // Enable drag and drop
  this->setAcceptDrops(true);

  currentPen_.setCapStyle(Qt::RoundCap);
  currentPen_.setJoinStyle(Qt::RoundJoin);

  // Connect to scene selection changes for transform handles
  connect(scene_, &QGraphicsScene::selectionChanged, this,
          &Canvas::updateTransformHandles);
}

Canvas::~Canvas() {
  resetColorSelection();
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
bool Canvas::isSnapToObjectEnabled() const { return snapToObject_; }
bool Canvas::isRulerVisible() const { return showRuler_; }
bool Canvas::isMeasurementToolEnabled() const {
  return measurementToolEnabled_;
}

bool Canvas::hasActiveColorSelection() const {
  if (!colorSelectionItemId_.isValid() || colorSelectionMask_.isNull() ||
      !colorSelectionHasPixels_) {
    return false;
  }

  ItemStore *store = itemStore();
  if (!store) {
    return false;
  }

  return dynamic_cast<QGraphicsPixmapItem *>(
             store->item(colorSelectionItemId_)) != nullptr;
}

ItemStore *Canvas::itemStore() const {
  return sceneController_ ? sceneController_->itemStore() : nullptr;
}

ItemId Canvas::registerItem(QGraphicsItem *item) {
  if (!sceneController_ || !item) {
    return ItemId();
  }
  return sceneController_->addItem(item);
}

// Action management methods
void Canvas::addDrawAction(QGraphicsItem *item) {
  ItemStore *store = itemStore();
  ItemId itemId;
  if (store && item) {
    itemId = store->idForItem(item);
    if (!itemId.isValid()) {
      itemId = registerItem(item);
    }
  }

  QUuid layerId;
  if (layerManager_) {
    if (Layer *active = layerManager_->activeLayer()) {
      layerId = active->id();
    }
  }

  auto onAdd = [this, layerId](QGraphicsItem *added) {
    if (!layerManager_ || !added)
      return;
    if (!layerId.isNull()) {
      if (Layer *layer = layerManager_->layer(layerId)) {
        layer->addItem(added);
        return;
      }
    }
    layerManager_->addItemToActiveLayer(added);
  };

  auto onRemove = [this](QGraphicsItem *removed) { onItemRemoved(removed); };

  if (store && itemId.isValid()) {
    undoStack_.push_back(
        std::make_unique<DrawAction>(itemId, store, onAdd, onRemove));
  } else {
    qWarning() << "Cannot create DrawAction without ItemStore";
  }
  clearRedoStack();
  // Add item to active layer
  if (layerManager_) {
    layerManager_->addItemToActiveLayer(item);
  }
  emit canvasModified();
}

void Canvas::addDeleteAction(QGraphicsItem *item) {
  ItemStore *store = itemStore();
  ItemId itemId;
  if (store && item) {
    itemId = store->idForItem(item);
    if (!itemId.isValid()) {
      itemId = registerItem(item);
    }
  }

  QUuid layerId;
  if (layerManager_) {
    if (Layer *layer = layerManager_->findLayerForItem(item)) {
      layerId = layer->id();
    }
  }

  auto onAdd = [this, layerId](QGraphicsItem *added) {
    if (!layerManager_ || !added)
      return;
    if (!layerId.isNull()) {
      if (Layer *layer = layerManager_->layer(layerId)) {
        layer->addItem(added);
        return;
      }
    }
    layerManager_->addItemToActiveLayer(added);
  };

  auto onRemove = [this](QGraphicsItem *removed) { onItemRemoved(removed); };

  if (store && itemId.isValid()) {
    undoStack_.push_back(
        std::make_unique<DeleteAction>(itemId, store, onAdd, onRemove));
  } else {
    qWarning() << "Cannot create DeleteAction without ItemStore";
  }
  clearRedoStack();
}

void Canvas::onItemRemoved(QGraphicsItem *item) {
  if (!item)
    return;
  ItemStore *store = itemStore();
  ItemId removedId;
  if (store) {
    removedId = store->idForItem(item);
  }
  if (item == colorSelectionOverlay_) {
    colorSelectionOverlay_ = nullptr;
  }
  if ((removedId.isValid() && removedId == colorSelectionItemId_) ||
      item == backgroundImage_) {
    resetColorSelection();
  }
  if (layerManager_) {
    if (Layer *layer = layerManager_->findLayerForItem(item)) {
      layer->removeItem(item);
    }
  }

  // Remove any transform handles that reference this item.
  QMutableListIterator<TransformHandleItem *> it(transformHandles_);
  while (it.hasNext()) {
    TransformHandleItem *handle = it.next();
    if (!handle) {
      it.remove();
      continue;
    }
    if (removedId.isValid()) {
      if (handle->targetItemId().isValid() &&
          handle->targetItemId() == removedId) {
        handle->clearTargetItem();
        if (scene_)
          scene_->removeItem(handle);
        delete handle;
        it.remove();
      }
    } else if (handle->targetItem() == item) {
      // Clear the target item reference before deleting to avoid dangling
      // pointer access
      handle->clearTargetItem();
      if (scene_)
        scene_->removeItem(handle);
      delete handle;
      it.remove();
    }
  }
}

void Canvas::addAction(std::unique_ptr<Action> action) {
  undoStack_.push_back(std::move(action));
  clearRedoStack();
}

void Canvas::clearRedoStack() { redoStack_.clear(); }

bool Canvas::hasNonNormalBlendModes() const {
  if (!layerManager_) {
    return false;
  }
  for (int i = 0; i < layerManager_->layerCount(); ++i) {
    Layer *layer = layerManager_->layer(i);
    if (layer && layer->blendMode() != Layer::BlendMode::Normal) {
      return true;
    }
  }
  return false;
}

void Canvas::paintEvent(QPaintEvent *event) {
  if (!hasNonNormalBlendModes()) {
    // No blend modes active — use default rendering for best performance
    QGraphicsView::paintEvent(event);
    return;
  }

  // Custom layer-by-layer rendering with blend mode compositing.
  // 1. Temporarily hide all managed items so the base scene paint
  //    only renders background/grid/foreground.
  // 2. Paint each layer into an offscreen image with the layer's blend mode.

  QSize vpSize = viewport()->size();
  if (vpSize.isEmpty()) {
    QGraphicsView::paintEvent(event);
    return;
  }

  // Collect all managed items and record their original visibility
  struct ItemState {
    QGraphicsItem *item;
    bool wasVisible;
  };
  QList<ItemState> allStates;

  for (int i = 0; i < layerManager_->layerCount(); ++i) {
    Layer *layer = layerManager_->layer(i);
    if (!layer)
      continue;
    for (QGraphicsItem *item : layer->items()) {
      if (item) {
        allStates.append({item, item->isVisible()});
        item->setVisible(false);
      }
    }
  }

  // Render the base scene (background, grid) without any managed items
  QImage baseImage(vpSize, QImage::Format_ARGB32_Premultiplied);
  baseImage.fill(Qt::transparent);
  {
    QPainter basePainter(&baseImage);
    basePainter.setRenderHints(renderHints());
    QGraphicsView::render(&basePainter, QRectF(), viewport()->rect());
    basePainter.end();
  }

  // Composited result starts with the base
  QImage result = baseImage;
  QPainter resultPainter(&result);
  resultPainter.setRenderHints(renderHints());

  // Render each layer
  for (int i = 0; i < layerManager_->layerCount(); ++i) {
    Layer *layer = layerManager_->layer(i);
    if (!layer || !layer->isVisible())
      continue;

    QList<QGraphicsItem *> layerItems = layer->items();
    if (layerItems.isEmpty())
      continue;

    // Show only this layer's items
    for (QGraphicsItem *item : layerItems) {
      if (item) {
        item->setVisible(true);
      }
    }

    // Render the scene (only this layer's items are visible)
    QImage layerImage(vpSize, QImage::Format_ARGB32_Premultiplied);
    layerImage.fill(Qt::transparent);
    {
      QPainter layerPainter(&layerImage);
      layerPainter.setRenderHints(renderHints());
      // Render scene items only (skip background to avoid double-drawing)
      scene_->render(&layerPainter, QRectF(viewport()->rect()),
                     mapToScene(viewport()->rect()).boundingRect());
      layerPainter.end();
    }

    // Hide items again
    for (QGraphicsItem *item : layerItems) {
      if (item) {
        item->setVisible(false);
      }
    }

    // Composite this layer with its blend mode
    QPainter::CompositionMode mode =
        Layer::toCompositionMode(layer->blendMode());
    resultPainter.setCompositionMode(mode);
    resultPainter.drawImage(0, 0, layerImage);
    resultPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
  }

  resultPainter.end();

  // Restore original visibility
  for (const auto &state : allStates) {
    if (state.item) {
      state.item->setVisible(state.wasVisible);
    }
  }

  // Paint the composited result to the viewport
  QPainter viewportPainter(viewport());
  viewportPainter.drawImage(0, 0, result);

  // Draw foreground elements (ruler, etc.) on top
  if (showRuler_) {
    viewportPainter.setRenderHints(renderHints());
    QRectF sceneRect = mapToScene(viewport()->rect()).boundingRect();
    viewportPainter.setTransform(viewportTransform());
    drawRuler(&viewportPainter, sceneRect);
  }
}

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
  if (shapeType == "Line") {
    currentShape_ = Line;
    setCursor(Qt::CrossCursor);
  } else if (shapeType == "Rectangle") {
    currentShape_ = Rectangle;
    setCursor(Qt::CrossCursor);
  } else if (shapeType == "Circle") {
    currentShape_ = Circle;
    setCursor(Qt::CrossCursor);
  } else if (shapeType == "Selection") {
    currentShape_ = Selection;
    setCursor(Qt::ArrowCursor);
    this->setDragMode(QGraphicsView::RubberBandDrag);
  } else if (shapeType == "ColorSelect") {
    currentShape_ = ColorSelect;
    setCursor(Qt::PointingHandCursor);
  }
  tempShapeItem_ = nullptr;
  if (!scene_)
    return;
  if (currentShape_ != Eraser) {
    hideEraserPreview();
    for (auto item : scene_->items()) {
      if (!item)
        continue;
      if (item != eraserPreview_ && item != backgroundImage_ &&
          item != colorSelectionOverlay_ &&
          item->type() != TransformHandleItem::Type) {
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
  if (currentShape_ == ColorSelect) {
    if (colorSelectionOverlay_) {
      colorSelectionOverlay_->show();
    }
  } else if (colorSelectionOverlay_) {
    colorSelectionOverlay_->hide();
  }
  isPanning_ = false;
}

void Canvas::setPenTool() {
  currentShape_ = Pen;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::CrossCursor);
  isPanning_ = false;
}

void Canvas::setEraserTool() {
  currentShape_ = Eraser;
  tempShapeItem_ = nullptr;
  eraserPen_.setColor(backgroundColor_);
  this->setDragMode(QGraphicsView::NoDrag);
  if (!scene_)
    return;
  scene_->clearSelection();
  for (auto item : scene_->items()) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      item->setFlag(QGraphicsItem::ItemIsSelectable, false);
      item->setFlag(QGraphicsItem::ItemIsMovable, false);
    }
  }
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::BlankCursor);
  isPanning_ = false;
}

void Canvas::setTextTool() {
  currentShape_ = Text;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::IBeamCursor);
  isPanning_ = false;
}

void Canvas::setMermaidTool() {
  currentShape_ = Mermaid;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::CrossCursor);
  isPanning_ = false;
}

void Canvas::setFillTool() {
  currentShape_ = Fill;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::PointingHandCursor);
  isPanning_ = false;
}

void Canvas::setColorSelectTool() {
  currentShape_ = ColorSelect;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_) {
    colorSelectionOverlay_->show();
  }
  setCursor(Qt::PointingHandCursor);
  isPanning_ = false;
}

void Canvas::setArrowTool() {
  currentShape_ = Arrow;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::CrossCursor);
  isPanning_ = false;
}

void Canvas::setCurvedArrowTool() {
  currentShape_ = CurvedArrow;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::CrossCursor);
  isPanning_ = false;
}

void Canvas::setBezierTool() {
  currentShape_ = Bezier;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::CrossCursor);
  isPanning_ = false;
}

void Canvas::setTextOnPathTool() {
  currentShape_ = TextOnPath;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  if (scene_)
    scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::CrossCursor);
  isPanning_ = false;
}

void Canvas::setPanTool() {
  currentShape_ = Pan;
  tempShapeItem_ = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview();
  scene_->clearSelection();
  if (colorSelectionOverlay_)
    colorSelectionOverlay_->hide();
  setCursor(Qt::OpenHandCursor);
  isPanning_ = false;
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
      eraserPreview_->setRect(eraserPreview_->rect().x(),
                              eraserPreview_->rect().y(), eraserPen_.width(),
                              eraserPen_.width());
    emit brushSizeChanged(newSize);
  }
}

void Canvas::decreaseBrushSize() {
  if (currentPen_.width() > MIN_BRUSH_SIZE) {
    int newSize = qMax(currentPen_.width() - BRUSH_SIZE_STEP, MIN_BRUSH_SIZE);
    currentPen_.setWidth(newSize);
    eraserPen_.setWidth(
        qMax(eraserPen_.width() - BRUSH_SIZE_STEP, MIN_BRUSH_SIZE));
    if (currentShape_ == Eraser && eraserPreview_)
      eraserPreview_->setRect(eraserPreview_->rect().x(),
                              eraserPreview_->rect().y(), eraserPen_.width(),
                              eraserPen_.width());
    emit brushSizeChanged(newSize);
  }
}

void Canvas::clearCanvas() {
  if (!scene_)
    return;
  resetColorSelection();
  // Ask for confirmation if there are drawable items on the canvas
  // (excluding system items like eraser preview and background image)
  int drawableItemCount = 0;
  for (auto item : scene_->items()) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      drawableItemCount++;
    }
  }

  if (drawableItemCount > 0) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Clear Canvas",
        "Are you sure you want to clear the canvas? This action cannot be "
        "undone.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) {
      return;
    }
  }

  clearTransformHandles();

  // Clear undo/redo stacks first (they may hold references to items)
  undoStack_.clear();
  redoStack_.clear();

  // Clear via sceneController if available (handles both store and scene)
  // Don't clear layerManager separately as it would double-delete items
  if (sceneController_) {
    sceneController_->clearAll();
  }

  // Reset layer manager state (recreate default layer)
  if (layerManager_) {
    layerManager_->clear();
  }

  if (backgroundImage_) {
    scene_->removeItem(backgroundImage_);
    delete backgroundImage_;
    backgroundImage_ = nullptr;
  }
  scene_->setBackgroundBrush(backgroundColor_);
  if (eraserPreview_) {
    eraserPreview_->hide();
  } else {
    eraserPreview_ =
        scene_->addEllipse(0, 0, eraserPen_.width(), eraserPen_.width(),
                           QPen(Qt::gray), QBrush(Qt::NoBrush));
    eraserPreview_->setZValue(1000);
    eraserPreview_->hide();
  }
}

void Canvas::newCanvas(int width, int height, const QColor &bgColor) {
  clearCanvas();
  if (!scene_)
    return;
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
void Canvas::zoomReset() {
  resetTransform();
  currentZoom_ = 1.0;
  emit zoomChanged(100.0);
}

void Canvas::applyZoom(double factor) {
  double newZoom = currentZoom_ * factor;
  if (newZoom > MAX_ZOOM || newZoom < MIN_ZOOM)
    return;
  currentZoom_ = newZoom;
  scale(factor, factor);
  emit zoomChanged(currentZoom_ * 100.0);
}

void Canvas::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    event->angleDelta().y() > 0 ? zoomIn() : zoomOut();
    event->accept();
  } else
    QGraphicsView::wheelEvent(event);
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
  snapEngine_.setSnapToGridEnabled(snapToGrid_);
  emit snapToGridChanged(snapToGrid_);
}

void Canvas::toggleSnapToObject() {
  snapToObject_ = !snapToObject_;
  snapEngine_.setSnapToObjectEnabled(snapToObject_);
  emit snapToObjectChanged(snapToObject_);
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

void Canvas::togglePressureSensitivity() {
  pressureSensitive_ = !pressureSensitive_;
  emit pressureSensitivityChanged(pressureSensitive_);
}

void Canvas::lockSelectedItems() {
  if (!scene_)
    return;
  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      item->setFlag(QGraphicsItem::ItemIsMovable, false);
      item->setFlag(QGraphicsItem::ItemIsSelectable, false);
      // Store lock state in data
      item->setData(0, "locked");
    }
  }
  scene_->clearSelection();
}

void Canvas::unlockSelectedItems() {
  if (!scene_)
    return;
  // Unlock all locked items (since they can't be selected when locked)
  for (QGraphicsItem *item : scene_->items()) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      if (item->data(0).toString() == "locked") {
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setData(0, QVariant());
      }
    }
  }
}

void Canvas::groupSelectedItems() {
  if (!scene_)
    return;
  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();

  // Need at least 2 items to group
  if (selectedItems.size() < 2)
    return;

  // Filter out items that shouldn't be grouped
  QList<QGraphicsItem *> itemsToGroup;
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item == eraserPreview_ || item == backgroundImage_ ||
        item == colorSelectionOverlay_)
      continue;
    // Skip transform handles
    if (dynamic_cast<TransformHandleItem *>(item))
      continue;
    itemsToGroup.append(item);
  }

  if (itemsToGroup.size() < 2)
    return;

  // Clear transform handles before grouping
  clearTransformHandles();

  ItemStore *store = itemStore();
  SceneController *controller = sceneController_;
  QList<ItemId> itemIds;
  QList<QPointF> originalPositions;
  QHash<QString, QUuid> originalLayerIds;
  QUuid targetLayerId;
  if (store) {
    for (QGraphicsItem *item : itemsToGroup) {
      ItemId id = store->idForItem(item);
      if (!id.isValid()) {
        id = registerItem(item);
      }
      if (id.isValid()) {
        itemIds.append(id);
        originalPositions.append(item->scenePos());
        if (layerManager_) {
          if (Layer *layer = layerManager_->findLayerForItem(id)) {
            originalLayerIds.insert(id.toString(), layer->id());
            if (targetLayerId.isNull()) {
              targetLayerId = layer->id();
            }
          }
        }
      }
    }
  }

  // Create the group
  QGraphicsItemGroup *group = new QGraphicsItemGroup();

  // Remove items from scene and add to group
  for (QGraphicsItem *item : itemsToGroup) {
    if (layerManager_) {
      if (Layer *layer = layerManager_->findLayerForItem(item)) {
        layer->removeItem(item);
      }
    }
    scene_->removeItem(item);
    group->addToGroup(item);
  }

  // Add group to scene (register with controller if available)
  ItemId groupId;
  if (controller) {
    Layer *targetLayer = nullptr;
    if (layerManager_ && !targetLayerId.isNull()) {
      targetLayer = layerManager_->layer(targetLayerId);
    }
    groupId = controller->addItem(group, targetLayer);
  } else {
    scene_->addItem(group);
  }
  group->setFlags(QGraphicsItem::ItemIsSelectable |
                  QGraphicsItem::ItemIsMovable);

  // Create undo action
  if (store && groupId.isValid() && !itemIds.isEmpty()) {
    auto onAdd = [this, store, targetLayerId,
                  originalLayerIds](QGraphicsItem *item) {
      if (!layerManager_ || !store || !item) {
        return;
      }
      ItemId id = store->idForItem(item);
      if (!id.isValid()) {
        return;
      }

      QUuid layerId = targetLayerId;
      if (originalLayerIds.contains(id.toString())) {
        layerId = originalLayerIds.value(id.toString());
      }

      if (!layerId.isNull()) {
        if (Layer *layer = layerManager_->layer(layerId)) {
          layer->addItem(id, store);
          return;
        }
      }
      layerManager_->addItemToActiveLayer(item);
    };
    auto onRemove = [this, store](QGraphicsItem *item) {
      if (!layerManager_ || !store || !item) {
        return;
      }
      ItemId id = store->idForItem(item);
      if (!id.isValid()) {
        return;
      }
      if (Layer *layer = layerManager_->findLayerForItem(id)) {
        layer->removeItem(id);
      }
    };

    auto action = std::make_unique<GroupAction>(
        groupId, itemIds, store, originalPositions, onAdd, onRemove);
    addAction(std::move(action));
  } else {
    qWarning() << "Cannot create GroupAction without ItemStore";
  }

  // Select the new group
  scene_->clearSelection();
  group->setSelected(true);
}

void Canvas::ungroupSelectedItems() {
  if (!scene_)
    return;
  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();

  // Clear transform handles before ungrouping
  clearTransformHandles();

  ItemStore *store = itemStore();
  for (QGraphicsItem *item : selectedItems) {
    QGraphicsItemGroup *group = dynamic_cast<QGraphicsItemGroup *>(item);
    if (!group)
      continue;

    // Get all children of the group
    QList<QGraphicsItem *> childItems = group->childItems();
    if (childItems.isEmpty())
      continue;

    // Store scene positions before ungrouping
    QList<QPointF> scenePositions;
    for (QGraphicsItem *child : childItems) {
      scenePositions.append(child->scenePos());
    }

    // Store group position for undo
    QPointF groupPosition = group->pos();
    QUuid groupLayerId;
    if (layerManager_) {
      if (Layer *layer = layerManager_->findLayerForItem(group)) {
        groupLayerId = layer->id();
        layer->removeItem(group);
      }
    }

    // Remove group from scene
    scene_->removeItem(group);

    // Remove items from group and add to scene
    for (int i = 0; i < childItems.size(); ++i) {
      QGraphicsItem *child = childItems[i];
      group->removeFromGroup(child);
      scene_->addItem(child);
      child->setPos(scenePositions[i]);
      child->setFlags(QGraphicsItem::ItemIsSelectable |
                      QGraphicsItem::ItemIsMovable);
      child->setSelected(true);
      if (layerManager_ && !groupLayerId.isNull()) {
        if (Layer *layer = layerManager_->layer(groupLayerId)) {
          layer->addItem(child);
        }
      }
    }

    // Create undo action
    if (store) {
      ItemId groupId = store->idForItem(group);
      QList<ItemId> itemIds;
      for (QGraphicsItem *child : childItems) {
        ItemId id = store->idForItem(child);
        if (!id.isValid()) {
          id = registerItem(child);
        }
        if (id.isValid()) {
          itemIds.append(id);
        }
      }
      if (groupId.isValid() && !itemIds.isEmpty()) {
        auto onAdd = [this, store, groupLayerId](QGraphicsItem *graphicsItem) {
          if (!layerManager_ || !store || !graphicsItem) {
            return;
          }
          ItemId id = store->idForItem(graphicsItem);
          if (!id.isValid()) {
            return;
          }
          if (!groupLayerId.isNull()) {
            if (Layer *layer = layerManager_->layer(groupLayerId)) {
              layer->addItem(id, store);
              return;
            }
          }
          layerManager_->addItemToActiveLayer(graphicsItem);
        };
        auto onRemove = [this, store](QGraphicsItem *graphicsItem) {
          if (!layerManager_ || !store || !graphicsItem) {
            return;
          }
          ItemId id = store->idForItem(graphicsItem);
          if (!id.isValid()) {
            return;
          }
          if (Layer *layer = layerManager_->findLayerForItem(id)) {
            layer->removeItem(id);
          }
        };

        auto action = std::make_unique<UngroupAction>(
            groupId, itemIds, store, groupPosition, onAdd, onRemove);
        addAction(std::move(action));
      } else {
        qWarning() << "Cannot create UngroupAction without valid IDs";
      }
    } else {
      qWarning() << "Cannot create UngroupAction without ItemStore";
    }
  }
}

void Canvas::bringToFront() {
  if (!scene_ || !layerManager_)
    return;
  ItemStore *store = itemStore();
  if (!store)
    return;

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty())
    return;

  // Collect items with their current indices, grouped by layer
  struct ItemInfo {
    ItemId id;
    Layer *layer;
    int oldIndex;
  };
  QList<ItemInfo> items;
  for (QGraphicsItem *item : selected) {
    ItemId id = store->idForItem(item);
    if (!id.isValid())
      continue;
    Layer *layer = layerManager_->findLayerForItem(id);
    if (!layer)
      continue;
    int idx = layer->indexOfItem(id);
    if (idx < layer->itemCount() - 1) {
      items.append({id, layer, idx});
    }
  }
  if (items.isEmpty())
    return;

  // Sort by current index ascending — process lowest first so higher indices
  // stay valid
  std::sort(items.begin(), items.end(),
            [](const ItemInfo &a, const ItemInfo &b) {
              return a.oldIndex < b.oldIndex;
            });

  auto composite = std::make_unique<CompositeAction>();
  for (const auto &info : items) {
    int currentIdx = info.layer->indexOfItem(info.id);
    int newIdx = info.layer->itemCount() - 1;
    if (currentIdx == newIdx)
      continue;
    layerManager_->moveItemToTop(info.id);
    composite->addAction(std::make_unique<ReorderAction>(
        info.id, info.layer->id(), currentIdx, info.layer->itemCount() - 1,
        layerManager_));
  }
  if (!composite->isEmpty()) {
    undoStack_.push_back(std::move(composite));
    clearRedoStack();
  }
  emit canvasModified();
}

void Canvas::bringForward() {
  if (!scene_ || !layerManager_)
    return;
  ItemStore *store = itemStore();
  if (!store)
    return;

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty())
    return;

  struct ItemInfo {
    ItemId id;
    Layer *layer;
    int oldIndex;
  };
  QList<ItemInfo> items;
  for (QGraphicsItem *item : selected) {
    ItemId id = store->idForItem(item);
    if (!id.isValid())
      continue;
    Layer *layer = layerManager_->findLayerForItem(id);
    if (!layer)
      continue;
    int idx = layer->indexOfItem(id);
    if (idx < layer->itemCount() - 1) {
      items.append({id, layer, idx});
    }
  }
  if (items.isEmpty())
    return;

  // Process from highest index down to avoid invalidating lower indices
  std::sort(items.begin(), items.end(),
            [](const ItemInfo &a, const ItemInfo &b) {
              return a.oldIndex > b.oldIndex;
            });

  auto composite = std::make_unique<CompositeAction>();
  for (const auto &info : items) {
    int currentIdx = info.layer->indexOfItem(info.id);
    if (currentIdx >= info.layer->itemCount() - 1)
      continue;
    layerManager_->moveItemUp(info.id);
    composite->addAction(std::make_unique<ReorderAction>(
        info.id, info.layer->id(), currentIdx, currentIdx + 1, layerManager_));
  }
  if (!composite->isEmpty()) {
    undoStack_.push_back(std::move(composite));
    clearRedoStack();
  }
  emit canvasModified();
}

void Canvas::sendBackward() {
  if (!scene_ || !layerManager_)
    return;
  ItemStore *store = itemStore();
  if (!store)
    return;

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty())
    return;

  struct ItemInfo {
    ItemId id;
    Layer *layer;
    int oldIndex;
  };
  QList<ItemInfo> items;
  for (QGraphicsItem *item : selected) {
    ItemId id = store->idForItem(item);
    if (!id.isValid())
      continue;
    Layer *layer = layerManager_->findLayerForItem(id);
    if (!layer)
      continue;
    int idx = layer->indexOfItem(id);
    if (idx > 0) {
      items.append({id, layer, idx});
    }
  }
  if (items.isEmpty())
    return;

  // Process from lowest index up to avoid invalidating higher indices
  std::sort(items.begin(), items.end(),
            [](const ItemInfo &a, const ItemInfo &b) {
              return a.oldIndex < b.oldIndex;
            });

  auto composite = std::make_unique<CompositeAction>();
  for (const auto &info : items) {
    int currentIdx = info.layer->indexOfItem(info.id);
    if (currentIdx <= 0)
      continue;
    layerManager_->moveItemDown(info.id);
    composite->addAction(std::make_unique<ReorderAction>(
        info.id, info.layer->id(), currentIdx, currentIdx - 1, layerManager_));
  }
  if (!composite->isEmpty()) {
    undoStack_.push_back(std::move(composite));
    clearRedoStack();
  }
  emit canvasModified();
}

void Canvas::sendToBack() {
  if (!scene_ || !layerManager_)
    return;
  ItemStore *store = itemStore();
  if (!store)
    return;

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty())
    return;

  struct ItemInfo {
    ItemId id;
    Layer *layer;
    int oldIndex;
  };
  QList<ItemInfo> items;
  for (QGraphicsItem *item : selected) {
    ItemId id = store->idForItem(item);
    if (!id.isValid())
      continue;
    Layer *layer = layerManager_->findLayerForItem(id);
    if (!layer)
      continue;
    int idx = layer->indexOfItem(id);
    if (idx > 0) {
      items.append({id, layer, idx});
    }
  }
  if (items.isEmpty())
    return;

  // Sort by index descending — process highest first so lower indices stay
  // valid
  std::sort(items.begin(), items.end(),
            [](const ItemInfo &a, const ItemInfo &b) {
              return a.oldIndex > b.oldIndex;
            });

  auto composite = std::make_unique<CompositeAction>();
  for (const auto &info : items) {
    int currentIdx = info.layer->indexOfItem(info.id);
    if (currentIdx == 0)
      continue;
    layerManager_->moveItemToBottom(info.id);
    composite->addAction(std::make_unique<ReorderAction>(
        info.id, info.layer->id(), currentIdx, 0, layerManager_));
  }
  if (!composite->isEmpty()) {
    undoStack_.push_back(std::move(composite));
    clearRedoStack();
  }
  emit canvasModified();
}

QString Canvas::calculateDistance(const QPointF &p1, const QPointF &p2) const {
  qreal dx = p2.x() - p1.x();
  qreal dy = p2.y() - p1.y();
  qreal distance = std::sqrt(dx * dx + dy * dy);
  return QString("%1 px").arg(QString::number(distance, 'f', 1));
}

void Canvas::drawForeground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawForeground(painter, rect);

  // Draw snap guide lines
  if (hasActiveSnap_ && (lastSnapResult_.snappedX || lastSnapResult_.snappedY)) {
    painter->save();
    QPen guidePen(QColor(0, 180, 255, 180), 0);
    guidePen.setStyle(Qt::DashLine);
    painter->setPen(guidePen);

    if (lastSnapResult_.snappedX) {
      painter->drawLine(QPointF(lastSnapResult_.guideX, rect.top()),
                        QPointF(lastSnapResult_.guideX, rect.bottom()));
    }
    if (lastSnapResult_.snappedY) {
      painter->drawLine(QPointF(rect.left(), lastSnapResult_.guideY),
                        QPointF(rect.right(), lastSnapResult_.guideY));
    }
    painter->restore();
  }

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
  painter->fillRect(
      QRectF(viewRect.left(), viewRect.top(), viewRect.width(), RULER_SIZE),
      QColor(50, 50, 50, 200));

  // Draw vertical ruler background
  painter->fillRect(
      QRectF(viewRect.left(), viewRect.top(), RULER_SIZE, viewRect.height()),
      QColor(50, 50, 50, 200));

  // Setup pen and font for ruler markings
  painter->setPen(QPen(Qt::white, 1));
  QFont rulerFont;
  rulerFont.setPixelSize(9);
  painter->setFont(rulerFont);

  // Calculate tick spacing based on zoom
  int majorTickSpacing = GRID_SIZE * 5; // Major tick every 100px at 100% zoom
  int minorTickSpacing = GRID_SIZE;     // Minor tick every 20px

  // Draw horizontal ruler ticks
  qreal startX =
      std::floor(viewRect.left() / minorTickSpacing) * minorTickSpacing;
  for (qreal x = startX; x < viewRect.right(); x += minorTickSpacing) {
    bool isMajor = (static_cast<int>(x) % majorTickSpacing) == 0;
    qreal tickHeight = isMajor ? RULER_SIZE * 0.6 : RULER_SIZE * 0.3;
    painter->drawLine(QPointF(x, viewRect.top() + RULER_SIZE - tickHeight),
                      QPointF(x, viewRect.top() + RULER_SIZE));
    if (isMajor) {
      painter->drawText(QPointF(x + 2, viewRect.top() + RULER_SIZE * 0.5),
                        QString::number(static_cast<int>(x)));
    }
  }

  // Draw vertical ruler ticks
  qreal startY =
      std::floor(viewRect.top() / minorTickSpacing) * minorTickSpacing;
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
  painter->fillRect(
      QRectF(viewRect.left(), viewRect.top(), RULER_SIZE, RULER_SIZE),
      QColor(70, 70, 70, 200));

  painter->restore();
}

QPointF Canvas::snapToGridPoint(const QPointF &point) const {
  if (!snapToGrid_)
    return point;

  qreal x = qRound(point.x() / GRID_SIZE) * GRID_SIZE;
  qreal y = qRound(point.y() / GRID_SIZE) * GRID_SIZE;
  return QPointF(x, y);
}

QPointF Canvas::snapPoint(const QPointF &point,
                          const QSet<QGraphicsItem *> &excludeItems) {
  if (!snapToGrid_ && !snapToObject_) {
    hasActiveSnap_ = false;
    return point;
  }

  // Build exclude set: always exclude helper items
  QSet<QGraphicsItem *> exclude = excludeItems;
  if (eraserPreview_)
    exclude.insert(eraserPreview_);
  if (backgroundImage_)
    exclude.insert(backgroundImage_);
  if (colorSelectionOverlay_)
    exclude.insert(colorSelectionOverlay_);
  if (tempShapeItem_)
    exclude.insert(tempShapeItem_);
  for (TransformHandleItem *h : transformHandles_) {
    if (h)
      exclude.insert(h);
  }

  SnapResult result =
      snapEngine_.snap(point, scene_ ? scene_->items() : QList<QGraphicsItem *>(),
                       exclude);
  lastSnapResult_ = result;
  hasActiveSnap_ = result.snappedX || result.snappedY;

  if (hasActiveSnap_)
    viewport()->update();

  return result.snappedPoint;
}

QPointF Canvas::calculateSmartDuplicateOffset() const {
  if (!scene_)
    return QPointF(GRID_SIZE, GRID_SIZE);
  // Calculate smart offset based on selected items and available space
  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty())
    return QPointF(GRID_SIZE, GRID_SIZE);

  // Get bounding rect of selected items
  QRectF boundingRect;
  for (QGraphicsItem *item : selected) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      if (boundingRect.isEmpty()) {
        boundingRect = item->sceneBoundingRect();
      } else {
        boundingRect = boundingRect.united(item->sceneBoundingRect());
      }
    }
  }

  // Use grid-aligned offset that's at least as large as the item + some padding
  qreal gridSize = static_cast<qreal>(GRID_SIZE);
  qreal offsetX =
      std::max(gridSize, std::ceil(boundingRect.width() / gridSize) * gridSize +
                             gridSize);
  qreal offsetY = gridSize;

  // If the offset would go beyond scene width, move down instead
  if (boundingRect.right() + offsetX > scene_->sceneRect().right()) {
    offsetX = gridSize;
    offsetY = std::max(gridSize,
                       std::ceil(boundingRect.height() / gridSize) * gridSize +
                           gridSize);
  }

  return QPointF(offsetX, offsetY);
}

void Canvas::selectAll() {
  if (!scene_)
    return;
  for (auto item : scene_->items())
    if (item && item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_)
      item->setSelected(true);
}

void Canvas::deleteSelectedItems() {
  if (!scene_)
    return;
  clearTransformHandles();
  // Copy the list to avoid modifying while iterating
  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      addDeleteAction(item);
      if (sceneController_) {
        sceneController_->removeItem(item, true);
      } else {
        scene_->removeItem(item);
        onItemRemoved(item);
      }
    }
  }
}

void Canvas::duplicateSelectedItems() {
  if (!scene_)
    return;
  QList<QGraphicsItem *> newItems;
  QPointF offset = calculateSmartDuplicateOffset();
  // Copy the list to avoid issues with modification during iteration
  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (auto g = dynamic_cast<QGraphicsItemGroup *>(item)) {
      // Duplicate entire group
      QGraphicsItemGroup *newGroup = new QGraphicsItemGroup();
      QPointF groupOffset =
          (snapToGrid_ || snapToObject_)
              ? snapPoint(g->pos() + offset)
              : g->pos() + offset;
      newGroup->setPos(groupOffset);

      // Duplicate each child item and add to new group
      for (QGraphicsItem *child : g->childItems()) {
        QGraphicsItem *newChild = cloneItem(child, eraserPreview_);
        if (newChild) {
          newGroup->addToGroup(newChild);
        }
      }

      scene_->addItem(newGroup);
      newGroup->setFlags(QGraphicsItem::ItemIsSelectable |
                         QGraphicsItem::ItemIsMovable);
      newItems.append(newGroup);
      addDrawAction(newGroup);
    } else {
      // Clone individual item
      QGraphicsItem *newItem = cloneItem(item, eraserPreview_);
      if (newItem) {
        QPointF newPos = (snapToGrid_ || snapToObject_)
                             ? snapPoint(item->pos() + offset)
                             : item->pos() + offset;
        newItem->setPos(newPos);
        newItem->setFlags(QGraphicsItem::ItemIsSelectable |
                          QGraphicsItem::ItemIsMovable);
        scene_->addItem(newItem);
        newItems.append(newItem);
        addDrawAction(newItem);
      }
    }
  }
  scene_->clearSelection();
  for (auto i : newItems)
    if (i)
      i->setSelected(true);
}

void Canvas::saveToFile() {
  QString fileName = QFileDialog::getSaveFileName(
      this, "Save Image", "",
#ifdef HAVE_QT_SVG
      "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp);;WebP (*.webp);;TIFF "
      "(*.tiff *.tif);;PDF (*.pdf);;SVG (*.svg);;"
      "Project (*.fspd)");
#else
      "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp);;WebP (*.webp);;TIFF "
      "(*.tiff *.tif);;PDF (*.pdf);;Project (*.fspd)");
#endif
  if (fileName.isEmpty())
    return;

  // Check if saving as project file
  if (fileName.endsWith(".fspd", Qt::CaseInsensitive)) {
    if (ProjectSerializer::saveProject(fileName, scene_, itemStore(),
                                       layerManager_, scene_->sceneRect(),
                                       backgroundColor_)) {
      RecentFilesManager::instance().addRecentFile(fileName);
    }
    return;
  }

  // Check if saving as PDF - pass the filename to avoid double dialog
  if (fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
    exportToPDFWithFilename(fileName);
    return;
  }

#ifdef HAVE_QT_SVG
  // Check if saving as SVG
  if (fileName.endsWith(".svg", Qt::CaseInsensitive)) {
    bool ev = eraserPreview_ && eraserPreview_->isVisible();
    if (eraserPreview_)
      eraserPreview_->hide();
    scene_->clearSelection();
    QRectF sr = scene_->itemsBoundingRect();
    if (sr.isEmpty())
      sr = scene_->sceneRect();
    sr.adjust(-10, -10, 10, 10);

    QSvgGenerator generator;
    generator.setFileName(fileName);
    generator.setSize(sr.size().toSize());
    generator.setViewBox(QRect(0, 0, sr.width(), sr.height()));
    generator.setTitle("FullScreen Pencil Draw Export");
    generator.setDescription("Exported from FullScreen Pencil Draw");

    QPainter svgPainter;
    svgPainter.begin(&generator);
    svgPainter.setRenderHint(QPainter::Antialiasing);
    svgPainter.setRenderHint(QPainter::TextAntialiasing);
    scene_->render(&svgPainter, QRectF(), sr);
    svgPainter.end();

    if (ev && eraserPreview_)
      eraserPreview_->show();
    RecentFilesManager::instance().addRecentFile(fileName);
    return;
  }
#endif

  bool ev = eraserPreview_ && eraserPreview_->isVisible();
  if (eraserPreview_)
    eraserPreview_->hide();
  scene_->clearSelection();
  QRectF sr = scene_->itemsBoundingRect();
  if (sr.isEmpty())
    sr = scene_->sceneRect();
  sr.adjust(-10, -10, 10, 10);
  QImage img(sr.size().toSize(), QImage::Format_ARGB32);
  img.fill(backgroundColor_);
  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  scene_->render(&p, QRectF(), sr);
  p.end();
  img.save(fileName);
  if (ev && eraserPreview_)
    eraserPreview_->show();

  // Add to recent files
  RecentFilesManager::instance().addRecentFile(fileName);
}

void Canvas::openFile() {
  resetColorSelection();
  QString fileFilter =
#ifdef HAVE_QT_SVG
      "All Supported (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tiff *.tif *.svg "
      "*.fspd);;Images "
      "(*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tiff *.tif *.svg);;Project "
      "Files "
      "(*.fspd);;All (*)";
#else
      "All Supported (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tiff *.tif "
      "*.fspd);;Images (*.png "
      "*.jpg *.jpeg *.bmp *.gif *.webp *.tiff *.tif);;Project Files "
      "(*.fspd);;All (*)";
#endif
  QString fileName =
      QFileDialog::getOpenFileName(this, "Open File", "", fileFilter);
  if (fileName.isEmpty())
    return;

  // Handle project files
  if (fileName.endsWith(".fspd", Qt::CaseInsensitive)) {
    QRectF loadedRect;
    QColor loadedBg;
    if (ProjectSerializer::loadProject(fileName, scene_, itemStore(),
                                       layerManager_, loadedRect, loadedBg)) {
      backgroundColor_ = loadedBg;
      eraserPen_.setColor(backgroundColor_);
      scene_->setSceneRect(loadedRect);
      scene_->setBackgroundBrush(backgroundColor_);
      RecentFilesManager::instance().addRecentFile(fileName);
      emit canvasModified();
    } else {
      QMessageBox::warning(this, "Error",
                           QString("Could not open project: %1").arg(fileName));
    }
    return;
  }

#ifdef HAVE_QT_SVG
  if (fileName.endsWith(".svg", Qt::CaseInsensitive)) {
    importSvg(fileName);
    RecentFilesManager::instance().addRecentFile(fileName);
    return;
  }
#endif
  QPixmap pm(fileName);
  if (pm.isNull())
    return;
  // Remove old background image - removeItem() doesn't delete, so we must
  // delete manually
  if (backgroundImage_) {
    scene_->removeItem(backgroundImage_);
    delete backgroundImage_;
    backgroundImage_ = nullptr;
  }
  backgroundImage_ = scene_->addPixmap(pm);
  backgroundImage_->setZValue(-1000);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsSelectable, false);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsMovable, false);
  scene_->setSceneRect(0, 0,
                       qMax(scene_->sceneRect().width(), (qreal)pm.width()),
                       qMax(scene_->sceneRect().height(), (qreal)pm.height()));

  // Add to recent files
  RecentFilesManager::instance().addRecentFile(fileName);
}

void Canvas::openRecentFile(const QString &filePath) {
  resetColorSelection();
  if (filePath.isEmpty())
    return;

  // Handle project files
  if (filePath.endsWith(".fspd", Qt::CaseInsensitive)) {
    QRectF loadedRect;
    QColor loadedBg;
    if (ProjectSerializer::loadProject(filePath, scene_, itemStore(),
                                       layerManager_, loadedRect, loadedBg)) {
      backgroundColor_ = loadedBg;
      eraserPen_.setColor(backgroundColor_);
      scene_->setSceneRect(loadedRect);
      scene_->setBackgroundBrush(backgroundColor_);
      RecentFilesManager::instance().addRecentFile(filePath);
      emit canvasModified();
    } else {
      QMessageBox::warning(this, "Error",
                           QString("Could not open project: %1").arg(filePath));
    }
    return;
  }
#ifdef HAVE_QT_SVG
  if (filePath.endsWith(".svg", Qt::CaseInsensitive)) {
    importSvg(filePath);
    RecentFilesManager::instance().addRecentFile(filePath);
    return;
  }
#endif

  QPixmap pm(filePath);
  if (pm.isNull()) {
    QMessageBox::warning(this, "Error",
                         QString("Could not open file: %1").arg(filePath));
    return;
  }
  // Remove old background image - removeItem() doesn't delete, so we must
  // delete manually
  if (backgroundImage_) {
    scene_->removeItem(backgroundImage_);
    delete backgroundImage_;
    backgroundImage_ = nullptr;
  }
  backgroundImage_ = scene_->addPixmap(pm);
  backgroundImage_->setZValue(-1000);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsSelectable, false);
  backgroundImage_->setFlag(QGraphicsItem::ItemIsMovable, false);
  scene_->setSceneRect(0, 0,
                       qMax(scene_->sceneRect().width(), (qreal)pm.width()),
                       qMax(scene_->sceneRect().height(), (qreal)pm.height()));

  // Update recent files
  RecentFilesManager::instance().addRecentFile(filePath);
}

void Canvas::saveProject() {
  QString fileName = QFileDialog::getSaveFileName(
      this, "Save Project", "", ProjectSerializer::fileFilter());
  if (fileName.isEmpty())
    return;
  if (!fileName.endsWith(".fspd", Qt::CaseInsensitive))
    fileName += ".fspd";
  if (ProjectSerializer::saveProject(fileName, scene_, itemStore(),
                                     layerManager_, scene_->sceneRect(),
                                     backgroundColor_)) {
    RecentFilesManager::instance().addRecentFile(fileName);
  } else {
    QMessageBox::warning(this, "Error",
                         QString("Could not save project: %1").arg(fileName));
  }
}

void Canvas::openProject() {
  resetColorSelection();
  QString fileName = QFileDialog::getOpenFileName(
      this, "Open Project", "", ProjectSerializer::fileFilter());
  if (fileName.isEmpty())
    return;
  QRectF loadedRect;
  QColor loadedBg;
  if (ProjectSerializer::loadProject(fileName, scene_, itemStore(),
                                     layerManager_, loadedRect, loadedBg)) {
    backgroundColor_ = loadedBg;
    eraserPen_.setColor(backgroundColor_);
    scene_->setSceneRect(loadedRect);
    scene_->setBackgroundBrush(backgroundColor_);
    RecentFilesManager::instance().addRecentFile(fileName);
    emit canvasModified();
  } else {
    QMessageBox::warning(this, "Error",
                         QString("Could not open project: %1").arg(fileName));
  }
}

void Canvas::exportToPDF() {
  QString fileName =
      QFileDialog::getSaveFileName(this, "Export to PDF", "", "PDF (*.pdf)");
  if (fileName.isEmpty())
    return;
  exportToPDFWithFilename(fileName);
}

void Canvas::exportToPDFWithFilename(const QString &fileName) {
  bool ev = eraserPreview_ && eraserPreview_->isVisible();
  if (eraserPreview_)
    eraserPreview_->hide();
  scene_->clearSelection();

  QRectF sr = scene_->itemsBoundingRect();
  if (sr.isEmpty())
    sr = scene_->sceneRect();
  sr.adjust(-10, -10, 10, 10);

  // Create PDF writer with A4 page size as default
  QPdfWriter pdfWriter(fileName);

  // Use A4 page size for reasonable dimensions
  pdfWriter.setPageSize(QPageSize(QPageSize::A4));
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

  // Calculate scale factor to fit the scene onto the page while maintaining
  // aspect ratio
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

  if (ev && eraserPreview_)
    eraserPreview_->show();

  // Add to recent files
  RecentFilesManager::instance().addRecentFile(fileName);
}

void Canvas::createTextItem(const QPointF &pos) {
  // Create text item with inline editing
  auto *textItem = new LatexTextItem();
  textItem->setFont(QFont("Arial", qMax(12, currentPen_.width() * 3)));
  textItem->setTextColor(currentPen_.color());
  textItem->setPos(pos);

  if (sceneController_) {
    sceneController_->addItem(textItem);
  } else {
    scene_->addItem(textItem);
  }

  // Connect to handle when editing is finished
  // Use QPointer to safely track the textItem in case it gets deleted before
  // signal fires
  connect(textItem, &LatexTextItem::editingFinished, this,
          [this, textItem = QPointer<LatexTextItem>(textItem)]() {
            // Check if textItem is still valid (not deleted)
            if (!textItem) {
              return;
            }
            // If the text is empty after editing, remove the item
            if (textItem->text().trimmed().isEmpty()) {
              if (sceneController_) {
                sceneController_->removeItem(textItem, false);
              } else {
                scene_->removeItem(textItem);
                onItemRemoved(textItem);
                textItem->deleteLater();
              }
            } else {
              // Add to undo stack only when there's actual content
              addDrawAction(textItem);
            }
          });

  // Start inline editing immediately
  textItem->startEditing();
}

void Canvas::createMermaidItem(const QPointF &pos) {
  // Create mermaid item with inline editing
  auto *mermaidItem = new MermaidTextItem();
  mermaidItem->setPos(pos);

  if (sceneController_) {
    sceneController_->addItem(mermaidItem);
  } else {
    scene_->addItem(mermaidItem);
  }

  // Connect to handle when editing is finished
  connect(mermaidItem, &MermaidTextItem::editingFinished, this,
          [this, mermaidItem = QPointer<MermaidTextItem>(mermaidItem)]() {
            // Check if mermaidItem is still valid (not deleted)
            if (!mermaidItem) {
              return;
            }
            // If the code is empty after editing, remove the item
            if (mermaidItem->mermaidCode().trimmed().isEmpty()) {
              if (sceneController_) {
                sceneController_->removeItem(mermaidItem, false);
              } else {
                scene_->removeItem(mermaidItem);
                onItemRemoved(mermaidItem);
                mermaidItem->deleteLater();
              }
            } else {
              // Add to undo stack only when there's actual content
              addDrawAction(mermaidItem);
            }
          });

  // Start inline editing immediately
  mermaidItem->startEditing();
}

QGraphicsPixmapItem *Canvas::findPixmapItemAt(const QPointF &scenePoint) const {
  if (!scene_) {
    return nullptr;
  }

  const QList<QGraphicsItem *> itemsAtPoint = scene_->items(scenePoint);
  for (QGraphicsItem *item : itemsAtPoint) {
    if (!item || item == eraserPreview_ || item == colorSelectionOverlay_) {
      continue;
    }

    auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
    if (!pixmapItem || pixmapItem->pixmap().isNull()) {
      continue;
    }

    const QPointF localPoint = pixmapItem->mapFromScene(scenePoint);
    const QSizeF pixmapSize =
        pixmapItem->pixmap().size() / pixmapItem->pixmap().devicePixelRatio();
    if (QRectF(QPointF(0, 0), pixmapSize).contains(localPoint)) {
      return pixmapItem;
    }
  }

  return nullptr;
}

QImage Canvas::createColorSelectionMask(const QImage &image, const QPoint &seed,
                                        int tolerance, bool contiguous) const {
  QImage src = image.convertToFormat(QImage::Format_ARGB32);
  QImage mask(src.size(), QImage::Format_Grayscale8);
  mask.fill(0);

  if (src.isNull() || !src.rect().contains(seed)) {
    return mask;
  }

  const QRgb seedPixel = src.pixel(seed);
  if (qAlpha(seedPixel) == 0) {
    return mask;
  }

  const int boundedTolerance = qBound(0, tolerance, 255);
  const int toleranceSq = boundedTolerance * boundedTolerance * 4;

  auto matchesSeed = [&](int x, int y) -> bool {
    const QRgb pixel = src.pixel(x, y);
    return qAlpha(pixel) > 0 &&
           colorDistanceSquared(pixel, seedPixel) <= toleranceSq;
  };

  if (contiguous) {
    QQueue<QPoint> queue;
    queue.enqueue(seed);

    while (!queue.isEmpty()) {
      const QPoint p = queue.dequeue();
      if (!src.rect().contains(p)) {
        continue;
      }

      uchar *maskRow = mask.scanLine(p.y());
      if (maskRow[p.x()] == COLOR_SELECTION_MARK) {
        continue;
      }
      if (!matchesSeed(p.x(), p.y())) {
        continue;
      }

      maskRow[p.x()] = COLOR_SELECTION_MARK;
      queue.enqueue(QPoint(p.x() + 1, p.y()));
      queue.enqueue(QPoint(p.x() - 1, p.y()));
      queue.enqueue(QPoint(p.x(), p.y() + 1));
      queue.enqueue(QPoint(p.x(), p.y() - 1));
    }
  } else {
    for (int y = 0; y < src.height(); ++y) {
      uchar *maskRow = mask.scanLine(y);
      for (int x = 0; x < src.width(); ++x) {
        if (matchesSeed(x, y)) {
          maskRow[x] = COLOR_SELECTION_MARK;
        }
      }
    }
  }

  return mask;
}

void Canvas::refreshColorSelectionOverlay() {
  if (!scene_) {
    return;
  }

  ItemStore *store = itemStore();
  auto *sourceItem = store ? dynamic_cast<QGraphicsPixmapItem *>(
                                 store->item(colorSelectionItemId_))
                           : nullptr;
  if (!sourceItem || colorSelectionMask_.isNull() ||
      !colorSelectionHasPixels_) {
    resetColorSelection();
    return;
  }

  QImage sourceImage =
      sourceItem->pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
  if (sourceImage.isNull() ||
      sourceImage.size() != colorSelectionMask_.size()) {
    resetColorSelection();
    return;
  }

  QImage overlayImage(colorSelectionMask_.size(),
                      QImage::Format_ARGB32_Premultiplied);
  overlayImage.fill(Qt::transparent);

  for (int y = 0; y < colorSelectionMask_.height(); ++y) {
    const uchar *maskRow = colorSelectionMask_.constScanLine(y);
    QRgb *overlayRow = reinterpret_cast<QRgb *>(overlayImage.scanLine(y));
    for (int x = 0; x < colorSelectionMask_.width(); ++x) {
      if (maskRow[x] == COLOR_SELECTION_MARK) {
        overlayRow[x] = qRgba(70, 170, 255, 96);
      }
    }
  }

  if (!colorSelectionOverlay_) {
    colorSelectionOverlay_ =
        new QGraphicsPixmapItem(QPixmap::fromImage(overlayImage));
    colorSelectionOverlay_->setAcceptedMouseButtons(Qt::NoButton);
    colorSelectionOverlay_->setFlag(QGraphicsItem::ItemIsSelectable, false);
    colorSelectionOverlay_->setFlag(QGraphicsItem::ItemIsMovable, false);
    scene_->addItem(colorSelectionOverlay_);
  } else {
    colorSelectionOverlay_->setPixmap(QPixmap::fromImage(overlayImage));
  }

  colorSelectionOverlay_->setPos(sourceItem->pos());
  colorSelectionOverlay_->setTransformOriginPoint(
      sourceItem->transformOriginPoint());
  colorSelectionOverlay_->setTransform(sourceItem->transform());
  colorSelectionOverlay_->setOpacity(1.0);
  colorSelectionOverlay_->setZValue(sourceItem->zValue() + 0.5);
  colorSelectionOverlay_->setVisible(currentShape_ == ColorSelect);
}

void Canvas::resetColorSelection() {
  colorSelectionItemId_ = ItemId();
  colorSelectionMask_ = QImage();
  colorSelectionHasPixels_ = false;

  if (colorSelectionOverlay_) {
    if (scene_) {
      scene_->removeItem(colorSelectionOverlay_);
    }
    delete colorSelectionOverlay_;
    colorSelectionOverlay_ = nullptr;
  }
}

bool Canvas::selectByColorAt(const QPointF &scenePoint,
                             Qt::KeyboardModifiers modifiers) {
  if (!scene_) {
    return false;
  }

  QGraphicsPixmapItem *pixmapItem = findPixmapItemAt(scenePoint);
  if (!pixmapItem) {
    resetColorSelection();
    return false;
  }

  QImage image =
      pixmapItem->pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
  if (image.isNull()) {
    resetColorSelection();
    return false;
  }

  const QPointF localPoint = pixmapItem->mapFromScene(scenePoint);
  const QPoint seed(static_cast<int>(std::floor(localPoint.x())),
                    static_cast<int>(std::floor(localPoint.y())));
  if (!image.rect().contains(seed)) {
    resetColorSelection();
    return false;
  }

  bool contiguous = colorSelectContiguous_;
  if (modifiers & Qt::ShiftModifier) {
    contiguous = !contiguous;
  }

  QImage mask =
      createColorSelectionMask(image, seed, colorSelectTolerance_, contiguous);

  bool hasPixels = false;
  for (int y = 0; y < mask.height() && !hasPixels; ++y) {
    const uchar *row = mask.constScanLine(y);
    for (int x = 0; x < mask.width(); ++x) {
      if (row[x] == COLOR_SELECTION_MARK) {
        hasPixels = true;
        break;
      }
    }
  }

  if (!hasPixels) {
    resetColorSelection();
    return false;
  }

  ItemStore *store = itemStore();
  if (!store) {
    resetColorSelection();
    return false;
  }

  ItemId itemId = store->idForItem(pixmapItem);
  if (!itemId.isValid()) {
    itemId = store->registerItem(pixmapItem);
  }
  if (!itemId.isValid()) {
    resetColorSelection();
    return false;
  }

  colorSelectionItemId_ = itemId;
  colorSelectionMask_ = std::move(mask);
  colorSelectionHasPixels_ = true;
  refreshColorSelectionOverlay();
  return true;
}

void Canvas::extractColorSelectionToNewLayer() {
  if (!hasActiveColorSelection()) {
    QMessageBox::information(
        this, "No Color Selection",
        "Use the Color Select tool on an image before extracting.");
    return;
  }

  ItemStore *store = itemStore();
  if (!store) {
    return;
  }

  auto *sourceItem =
      dynamic_cast<QGraphicsPixmapItem *>(store->item(colorSelectionItemId_));
  if (!sourceItem) {
    resetColorSelection();
    return;
  }

  QImage originalImage =
      sourceItem->pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
  if (originalImage.isNull() ||
      originalImage.size() != colorSelectionMask_.size()) {
    resetColorSelection();
    return;
  }

  QImage extractedImage(originalImage.size(), QImage::Format_ARGB32);
  extractedImage.fill(Qt::transparent);
  QImage remainingImage = originalImage;

  bool anyExtracted = false;
  for (int y = 0; y < originalImage.height(); ++y) {
    const uchar *maskRow = colorSelectionMask_.constScanLine(y);
    const QRgb *sourceRow =
        reinterpret_cast<const QRgb *>(originalImage.constScanLine(y));
    QRgb *extractedRow = reinterpret_cast<QRgb *>(extractedImage.scanLine(y));
    QRgb *remainingRow = reinterpret_cast<QRgb *>(remainingImage.scanLine(y));

    for (int x = 0; x < originalImage.width(); ++x) {
      if (maskRow[x] == COLOR_SELECTION_MARK) {
        extractedRow[x] = sourceRow[x];
        remainingRow[x] = qRgba(qRed(sourceRow[x]), qGreen(sourceRow[x]),
                                qBlue(sourceRow[x]), 0);
        anyExtracted = true;
      }
    }
  }

  if (!anyExtracted) {
    QMessageBox::information(this, "Empty Selection",
                             "No pixels were selected for extraction.");
    return;
  }

  auto *newItem = new QGraphicsPixmapItem(QPixmap::fromImage(extractedImage));
  newItem->setPos(sourceItem->pos());
  newItem->setTransformOriginPoint(sourceItem->transformOriginPoint());
  newItem->setTransform(sourceItem->transform());
  newItem->setOpacity(sourceItem->opacity());
  newItem->setZValue(sourceItem->zValue() + 0.5);
  newItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  newItem->setFlag(QGraphicsItem::ItemIsMovable, true);

  Layer *newLayer = nullptr;
  QUuid newLayerId;
  if (layerManager_) {
    newLayer = layerManager_->createLayer(
        QString("Color Selection %1").arg(layerManager_->layerCount() + 1),
        Layer::Type::Raster);
    if (newLayer) {
      newLayerId = newLayer->id();
      layerManager_->setActiveLayer(newLayerId);
    }
  }

  ItemId newItemId;
  if (sceneController_) {
    newItemId = sceneController_->addItem(newItem, newLayer);
  } else {
    scene_->addItem(newItem);
    newItemId = registerItem(newItem);
    if (newLayer) {
      newLayer->addItem(newItem);
    }
  }

  if (!newItemId.isValid()) {
    if (scene_) {
      scene_->removeItem(newItem);
    }
    delete newItem;
    return;
  }

  sourceItem->setPixmap(QPixmap::fromImage(remainingImage));

  auto onAdd = [this, newLayerId](QGraphicsItem *added) {
    if (!layerManager_ || !added) {
      return;
    }
    if (!newLayerId.isNull()) {
      if (Layer *layer = layerManager_->layer(newLayerId)) {
        layer->addItem(added);
        return;
      }
    }
    layerManager_->addItemToActiveLayer(added);
  };
  auto onRemove = [this](QGraphicsItem *removed) { onItemRemoved(removed); };

  auto action = std::make_unique<CompositeAction>();
  action->addAction(std::make_unique<RasterPixmapAction>(
      colorSelectionItemId_, store, originalImage, remainingImage));
  action->addAction(
      std::make_unique<DrawAction>(newItemId, store, onAdd, onRemove));
  addAction(std::move(action));

  scene_->clearSelection();
  newItem->setSelected(true);
  resetColorSelection();
  emit canvasModified();
}

void Canvas::clearColorSelection() { resetColorSelection(); }

void Canvas::setColorSelectTolerance() {
  bool ok = false;
  int value = QInputDialog::getInt(this, "Color Select Tolerance",
                                   "Tolerance (0-255):", colorSelectTolerance_,
                                   0, 255, 1, &ok);
  if (!ok) {
    return;
  }
  colorSelectTolerance_ = value;
}

void Canvas::toggleColorSelectContiguous() {
  colorSelectContiguous_ = !colorSelectContiguous_;
}

void Canvas::fillAt(const QPointF &point) {
  if (!scene_)
    return;

  fillTopItemAtPoint(scene_, point, currentPen_.color(), itemStore(),
                     backgroundImage_, eraserPreview_,
                     [this](std::unique_ptr<Action> action) {
                       if (action) {
                         addAction(std::move(action));
                       }
                     });
}

void Canvas::drawArrow(const QPointF &start, const QPointF &end) {
  if (!scene_)
    return;
  auto li = new QGraphicsLineItem(QLineF(start, end));
  li->setPen(currentPen_);
  double angle = std::atan2(end.y() - start.y(), end.x() - start.x());
  double sz = currentPen_.width() * 4;
  QPolygonF ah;
  ah << end
     << end - QPointF(std::cos(angle - M_PI / 6) * sz,
                      std::sin(angle - M_PI / 6) * sz)
     << end - QPointF(std::cos(angle + M_PI / 6) * sz,
                      std::sin(angle + M_PI / 6) * sz);
  auto ahi = new QGraphicsPolygonItem(ah);
  ahi->setPen(currentPen_);
  ahi->setBrush(currentPen_.color());

  auto *group = new QGraphicsItemGroup();
  group->addToGroup(li);
  group->addToGroup(ahi);
  group->setFlags(QGraphicsItem::ItemIsSelectable |
                  QGraphicsItem::ItemIsMovable);
  scene_->addItem(group);
  addDrawAction(group);
}

void Canvas::drawCurvedArrow(const QPointF &start, const QPointF &end,
                             Qt::KeyboardModifiers modifiers) {
  if (!scene_)
    return;
  const qreal arrowSize = qMax<qreal>(8.0, currentPen_.widthF() * 4.0);
  QPainterPath path = curvedArrowPath(start, end, modifiers, arrowSize);

  auto *pathItem = new QGraphicsPathItem(path);
  pathItem->setPen(currentPen_);
  pathItem->setBrush(Qt::NoBrush);
  pathItem->setFlags(QGraphicsItem::ItemIsSelectable |
                     QGraphicsItem::ItemIsMovable);
  scene_->addItem(pathItem);

  addDrawAction(pathItem);
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  if (!event)
    return;
  if (!scene_) {
    QGraphicsView::mousePressEvent(event);
    return;
  }

  // Only handle left-click for tool actions; right-click is for context menu
  if (event->button() != Qt::LeftButton) {
    QGraphicsView::mousePressEvent(event);
    return;
  }

  QPointF sp = mapToScene(event->pos());
  // Apply snapping for shape tools
  if ((snapToGrid_ || snapToObject_) &&
      (currentShape_ == Rectangle || currentShape_ == Circle ||
       currentShape_ == Line || currentShape_ == Arrow ||
       currentShape_ == CurvedArrow)) {
    sp = snapPoint(sp);
  }
  emit cursorPositionChanged(sp);
  if (currentShape_ == Selection) {
    trackingSelectionMove_ = false;
    selectionMoveStartPositions_.clear();
    QGraphicsView::mousePressEvent(event);

    QGraphicsItem *clickedItem = scene_->itemAt(sp, QTransform());
    if (clickedItem && clickedItem->type() == TransformHandleItem::Type) {
      return;
    }

    ItemStore *store = itemStore();
    if (!store) {
      return;
    }

    const QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
    for (QGraphicsItem *item : selectedItems) {
      if (!item)
        continue;
      if (item == eraserPreview_ || item == backgroundImage_ ||
          item == colorSelectionOverlay_)
        continue;
      if (item->type() == TransformHandleItem::Type)
        continue;
      if (!(item->flags() & QGraphicsItem::ItemIsMovable))
        continue;

      ItemId id = store->idForItem(item);
      if (!id.isValid()) {
        id = registerItem(item);
      }
      if (!id.isValid())
        continue;

      selectionMoveStartPositions_.insert(id, item->pos());
    }

    trackingSelectionMove_ = !selectionMoveStartPositions_.isEmpty();
    return;
  }
  if (currentShape_ == Pan) {
    isPanning_ = true;
    lastPanPoint_ = event->pos();
    setCursor(Qt::ClosedHandCursor);
    return;
  }
  startPoint_ = sp;
  switch (currentShape_) {
  case Text:
    createTextItem((snapToGrid_ || snapToObject_) ? snapPoint(sp) : sp);
    break;
  case Mermaid:
    createMermaidItem((snapToGrid_ || snapToObject_) ? snapPoint(sp) : sp);
    break;
  case Fill:
    fillAt(sp);
    break;
  case ColorSelect:
    selectByColorAt(sp, event->modifiers());
    break;
  case Eraser:
    eraseAt(sp);
    break;
  case Pen: {
    currentPath_ = new QGraphicsPathItem();
    if (pressureSensitive_ && tabletActive_) {
      // Use a no-stroke pen and fill with the pen color for pressure strokes
      currentPath_->setPen(Qt::NoPen);
      currentPath_->setBrush(currentPen_.color());
    } else {
      currentPath_->setPen(currentPen_);
    }
    currentPath_->setFlags(QGraphicsItem::ItemIsSelectable |
                           QGraphicsItem::ItemIsMovable);
    QPainterPath p;
    p.moveTo(sp);
    currentPath_->setPath(p);
    scene_->addItem(currentPath_);
    pointBuffer_.clear();
    pressureBuffer_.clear();
    pointBuffer_.append(sp);
    if (pressureSensitive_ && tabletActive_) {
      pressureBuffer_.append(tabletPressure_);
    }
    addDrawAction(currentPath_);
  } break;
  case Rectangle: {
    auto ri = new QGraphicsRectItem(QRectF(startPoint_, startPoint_));
    ri->setPen(currentPen_);
    if (fillShapes_)
      ri->setBrush(currentPen_.color());
    ri->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    scene_->addItem(ri);
    tempShapeItem_ = ri;
    addDrawAction(ri);
  } break;
  case Arrow: {
    auto li = new QGraphicsLineItem(QLineF(startPoint_, startPoint_));
    li->setPen(currentPen_);
    li->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    scene_->addItem(li);
    tempShapeItem_ = li;
  } break;
  case CurvedArrow: {
    curvedArrowManualFlip_ = (event->modifiers() & Qt::ShiftModifier);
    curvedArrowAutoBendEnabled_ = !curvedArrowManualFlip_;
    curvedArrowShiftWasDown_ = curvedArrowManualFlip_;
    Qt::KeyboardModifiers modifiers = effectiveCurvedArrowModifiers(
        event->modifiers(), curvedArrowAutoBendEnabled_,
        curvedArrowManualFlip_);

    auto *pi = new QGraphicsPathItem();
    pi->setPen(currentPen_);
    pi->setBrush(Qt::NoBrush);
    pi->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    pi->setPath(curvedArrowPath(startPoint_, startPoint_, modifiers,
                                qMax<qreal>(8.0, currentPen_.widthF() * 4.0)));
    scene_->addItem(pi);
    tempShapeItem_ = pi;
  } break;
  case Circle: {
    auto ei = new QGraphicsEllipseItem(QRectF(startPoint_, startPoint_));
    ei->setPen(currentPen_);
    if (fillShapes_)
      ei->setBrush(currentPen_.color());
    ei->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    scene_->addItem(ei);
    tempShapeItem_ = ei;
    addDrawAction(ei);
  } break;
  case Line: {
    auto li = new QGraphicsLineItem(QLineF(startPoint_, startPoint_));
    li->setPen(currentPen_);
    li->setFlags(QGraphicsItem::ItemIsSelectable |
                 QGraphicsItem::ItemIsMovable);
    scene_->addItem(li);
    tempShapeItem_ = li;
    addDrawAction(li);
  } break;
  default:
    QGraphicsView::mousePressEvent(event);
    break;
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  if (!event)
    return;
  if (!scene_) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }
  QPointF cp = mapToScene(event->pos());
  // Apply snapping for shape tools during drawing
  if ((snapToGrid_ || snapToObject_) &&
      (currentShape_ == Rectangle || currentShape_ == Circle ||
       currentShape_ == Line || currentShape_ == Arrow ||
       currentShape_ == CurvedArrow)) {
    cp = snapPoint(cp);
  }
  emit cursorPositionChanged(cp);

  // Emit measurement when drawing shapes (only when we have a valid start
  // point)
  if (measurementToolEnabled_ && (event->buttons() & Qt::LeftButton) &&
      tempShapeItem_) {
    if (currentShape_ == Rectangle || currentShape_ == Circle ||
        currentShape_ == Line || currentShape_ == Arrow ||
        currentShape_ == CurvedArrow) {
      emit measurementUpdated(calculateDistance(startPoint_, cp));
    }
  }

  if (currentShape_ == Selection) {
    QGraphicsView::mouseMoveEvent(event);
    if ((event->buttons() & Qt::LeftButton) && !transformHandles_.isEmpty()) {
      for (TransformHandleItem *handle : transformHandles_) {
        if (handle)
          handle->updateHandles();
      }
    }
    return;
  }
  if (currentShape_ == Pan && isPanning_) {
    QPointF d = event->pos() - lastPanPoint_;
    lastPanPoint_ = event->pos();
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
    return;
  }
  switch (currentShape_) {
  case Pen:
    if (event->buttons() & Qt::LeftButton) {
      if (pressureSensitive_ && tabletActive_)
        addPressurePoint(cp, tabletPressure_);
      else
        addPoint(cp);
    }
    break;
  case Eraser:
    if (event->buttons() & Qt::LeftButton)
      eraseAt(cp);
    updateEraserPreview(cp);
    break;
  case Rectangle:
    if (tempShapeItem_)
      static_cast<QGraphicsRectItem *>(tempShapeItem_)
          ->setRect(QRectF(startPoint_, cp).normalized());
    break;
  case Arrow:
    if (tempShapeItem_)
      static_cast<QGraphicsLineItem *>(tempShapeItem_)
          ->setLine(QLineF(startPoint_, cp));
    break;
  case CurvedArrow:
    if (tempShapeItem_) {
      const bool shiftDown = (event->modifiers() & Qt::ShiftModifier);
      if (shiftDown && !curvedArrowShiftWasDown_) {
        curvedArrowManualFlip_ = !curvedArrowManualFlip_;
        curvedArrowAutoBendEnabled_ = false;
      }
      curvedArrowShiftWasDown_ = shiftDown;

      Qt::KeyboardModifiers modifiers = effectiveCurvedArrowModifiers(
          event->modifiers(), curvedArrowAutoBendEnabled_,
          curvedArrowManualFlip_);
      auto *pathItem = dynamic_cast<QGraphicsPathItem *>(tempShapeItem_);
      if (pathItem) {
        pathItem->setPath(
            curvedArrowPath(startPoint_, cp, modifiers,
                            qMax<qreal>(8.0, currentPen_.widthF() * 4.0)));
      }
    }
    break;
  case Circle:
    if (tempShapeItem_)
      static_cast<QGraphicsEllipseItem *>(tempShapeItem_)
          ->setRect(QRectF(startPoint_, cp).normalized());
    break;
  case Line:
    if (tempShapeItem_)
      static_cast<QGraphicsLineItem *>(tempShapeItem_)
          ->setLine(QLineF(startPoint_, cp));
    break;
  default:
    QGraphicsView::mouseMoveEvent(event);
    break;
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  if (!event)
    return;
  if (!scene_) {
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }
  QPointF ep = mapToScene(event->pos());
  // Apply snapping for shape tools
  if ((snapToGrid_ || snapToObject_) &&
      (currentShape_ == Rectangle || currentShape_ == Circle ||
       currentShape_ == Line || currentShape_ == Arrow ||
       currentShape_ == CurvedArrow)) {
    ep = snapPoint(ep);
  }

  // Clear snap guides on release
  hasActiveSnap_ = false;
  viewport()->update();
  if (currentShape_ == Selection) {
    QGraphicsView::mouseReleaseEvent(event);
    if (!transformHandles_.isEmpty()) {
      for (TransformHandleItem *handle : transformHandles_) {
        if (handle)
          handle->updateHandles();
      }
    }

    if (trackingSelectionMove_) {
      ItemStore *store = itemStore();
      if (store) {
        auto moveAction = std::make_unique<CompositeAction>();

        for (auto it = selectionMoveStartPositions_.cbegin();
             it != selectionMoveStartPositions_.cend(); ++it) {
          const ItemId id = it.key();
          QGraphicsItem *item = store->item(id);
          if (!item)
            continue;

          const QPointF oldPos = it.value();
          const QPointF newPos = item->pos();
          if (QLineF(oldPos, newPos).length() > 0.01) {
            moveAction->addAction(
                std::make_unique<MoveAction>(id, store, oldPos, newPos));
          }
        }

        if (!moveAction->isEmpty()) {
          addAction(std::move(moveAction));
          emit canvasModified();
        }
      }
    }

    trackingSelectionMove_ = false;
    selectionMoveStartPositions_.clear();
    return;
  }
  if (currentShape_ == Pan) {
    isPanning_ = false;
    setCursor(Qt::OpenHandCursor);
    return;
  }
  if (currentShape_ == Arrow && tempShapeItem_) {
    // tempShapeItem_ added with addItem - removeItem doesn't delete, so we
    // delete manually
    scene_->removeItem(tempShapeItem_);
    delete tempShapeItem_;
    tempShapeItem_ = nullptr;
    drawArrow(startPoint_, ep);
    return;
  }
  if (currentShape_ == CurvedArrow && tempShapeItem_) {
    Qt::KeyboardModifiers modifiers = effectiveCurvedArrowModifiers(
        event->modifiers(), curvedArrowAutoBendEnabled_,
        curvedArrowManualFlip_);
    scene_->removeItem(tempShapeItem_);
    delete tempShapeItem_;
    tempShapeItem_ = nullptr;
    drawCurvedArrow(startPoint_, ep, modifiers);
    curvedArrowAutoBendEnabled_ = true;
    curvedArrowManualFlip_ = false;
    curvedArrowShiftWasDown_ = false;
    return;
  }
  if (currentShape_ != Pen && currentShape_ != Eraser && tempShapeItem_)
    tempShapeItem_ = nullptr;
  else if (currentShape_ == Pen) {
    currentPath_ = nullptr;
    pointBuffer_.clear();
    pressureBuffer_.clear();
    tabletActive_ = false;
  }
}

void Canvas::tabletEvent(QTabletEvent *event) {
  if (!event)
    return;

  tabletPressure_ = event->pressure();

  switch (event->type()) {
  case QEvent::TabletPress:
    tabletActive_ = true;
    event->accept();
    break;
  case QEvent::TabletMove:
    event->accept();
    break;
  case QEvent::TabletRelease:
    tabletActive_ = false;
    tabletPressure_ = 1.0;
    event->accept();
    break;
  default:
    break;
  }

  // Let the mouse events handle the actual drawing;
  // the tablet event just updates pressure state.
  QGraphicsView::tabletEvent(event);
}

void Canvas::updateEraserPreview(const QPointF &pos) {
  if (!eraserPreview_)
    return;
  qreal r = eraserPen_.width() / 2.0;
  eraserPreview_->setRect(pos.x() - r, pos.y() - r, eraserPen_.width(),
                          eraserPen_.width());
  if (!eraserPreview_->isVisible())
    eraserPreview_->show();
}

void Canvas::hideEraserPreview() {
  if (eraserPreview_)
    eraserPreview_->hide();
}

void Canvas::copySelectedItems() {
  if (!scene_)
    return;
  auto sel = scene_->selectedItems();
  if (sel.isEmpty())
    return;
  auto md = new QMimeData();
  QByteArray ba;
  QDataStream ds(&ba, QIODevice::WriteOnly);
  for (auto item : sel) {
    if (!item)
      continue;
    if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) {
      ds << QString("RectangleT") << r->rect() << r->pos() << r->pen()
         << r->brush() << r->transform();
    } else if (auto e = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      if (item == eraserPreview_)
        continue;
      ds << QString("EllipseT") << e->rect() << e->pos() << e->pen()
         << e->brush() << e->transform();
    } else if (auto l = dynamic_cast<QGraphicsLineItem *>(item)) {
      ds << QString("LineT") << l->line() << l->pos() << l->pen()
         << l->transform();
    } else if (auto p = dynamic_cast<QGraphicsPathItem *>(item)) {
      ds << QString("PathV3") << p->path() << p->pos() << p->pen() << p->brush()
         << p->transform();
    } else if (auto lt = dynamic_cast<LatexTextItem *>(item)) {
      ds << QString("LatexTextT") << lt->text() << lt->pos() << lt->font()
         << lt->textColor() << lt->transform();
    } else if (auto t = dynamic_cast<QGraphicsTextItem *>(item)) {
      ds << QString("TextT") << t->toPlainText() << t->pos() << t->font()
         << t->defaultTextColor() << t->transform();
    } else if (auto pg = dynamic_cast<QGraphicsPolygonItem *>(item)) {
      ds << QString("PolygonT") << pg->polygon() << pg->pos() << pg->pen()
         << pg->brush() << pg->transform();
    }
  }
  md->setData("application/x-canvas-items", ba);
  QApplication::clipboard()->setMimeData(md);
}

void Canvas::cutSelectedItems() {
  if (!scene_)
    return;
  auto sel = scene_->selectedItems();
  if (sel.isEmpty())
    return;
  copySelectedItems();
  for (auto item : sel) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      addDeleteAction(item);
      if (sceneController_) {
        sceneController_->removeItem(item, true);
      } else {
        scene_->removeItem(item);
        onItemRemoved(item);
      }
    }
  }
}

void Canvas::pasteItems() {
  if (!scene_)
    return;
  auto md = QApplication::clipboard()->mimeData();
  if (!md)
    return;

  // Handle canvas items format (internal copy/paste) - highest priority
  if (md->hasFormat("application/x-canvas-items")) {
    QByteArray ba = md->data("application/x-canvas-items");
    QDataStream ds(&ba, QIODevice::ReadOnly);
    QList<QGraphicsItem *> pi;
    while (!ds.atEnd()) {
      QString t;
      ds >> t;
      if (t == "RectangleT") {
        QRectF r;
        QPointF p;
        QPen pn;
        QBrush b;
        QTransform tr;
        ds >> r >> p >> pn >> b >> tr;
        auto n = new QGraphicsRectItem(r);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setTransform(tr);
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "Rectangle") {
        QRectF r;
        QPointF p;
        QPen pn;
        QBrush b;
        ds >> r >> p >> pn >> b;
        auto n = new QGraphicsRectItem(r);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "EllipseT") {
        QRectF r;
        QPointF p;
        QPen pn;
        QBrush b;
        QTransform tr;
        ds >> r >> p >> pn >> b >> tr;
        auto n = new QGraphicsEllipseItem(r);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setTransform(tr);
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "Ellipse") {
        QRectF r;
        QPointF p;
        QPen pn;
        QBrush b;
        ds >> r >> p >> pn >> b;
        auto n = new QGraphicsEllipseItem(r);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "LineT") {
        QLineF l;
        QPointF p;
        QPen pn;
        QTransform tr;
        ds >> l >> p >> pn >> tr;
        auto n = new QGraphicsLineItem(l);
        n->setPen(pn);
        n->setPos(p + QPointF(20, 20));
        n->setTransform(tr);
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "Line") {
        QLineF l;
        QPointF p;
        QPen pn;
        ds >> l >> p >> pn;
        auto n = new QGraphicsLineItem(l);
        n->setPen(pn);
        n->setPos(p + QPointF(20, 20));
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "PathV3") {
        QPainterPath pp;
        QPointF p;
        QPen pn;
        QBrush b;
        QTransform tr;
        ds >> pp >> p >> pn >> b >> tr;
        auto n = new QGraphicsPathItem(pp);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setTransform(tr);
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "PathV2") {
        QPainterPath pp;
        QPointF p;
        QPen pn;
        QBrush b;
        ds >> pp >> p >> pn >> b;
        auto n = new QGraphicsPathItem(pp);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "Path") {
        // Backward compatibility with clipboard data from older versions.
        QPainterPath pp;
        QPointF p;
        QPen pn;
        ds >> pp >> p >> pn;
        auto n = new QGraphicsPathItem(pp);
        n->setPen(pn);
        n->setPos(p + QPointF(20, 20));
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "LatexTextT") {
        QString tx;
        QPointF p;
        QFont f;
        QColor c;
        QTransform tr;
        ds >> tx >> p >> f >> c >> tr;
        auto n = new LatexTextItem();
        n->setText(tx);
        n->setFont(f);
        n->setTextColor(c);
        n->setPos(p + QPointF(20, 20));
        n->setTransform(tr);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "LatexText") {
        QString tx;
        QPointF p;
        QFont f;
        QColor c;
        ds >> tx >> p >> f >> c;
        auto n = new LatexTextItem();
        n->setText(tx);
        n->setFont(f);
        n->setTextColor(c);
        n->setPos(p + QPointF(20, 20));
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "TextT") {
        QString tx;
        QPointF p;
        QFont f;
        QColor c;
        QTransform tr;
        ds >> tx >> p >> f >> c >> tr;
        auto n = new QGraphicsTextItem(tx);
        n->setFont(f);
        n->setDefaultTextColor(c);
        n->setPos(p + QPointF(20, 20));
        n->setTransform(tr);
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "Text") {
        QString tx;
        QPointF p;
        QFont f;
        QColor c;
        ds >> tx >> p >> f >> c;
        auto n = new QGraphicsTextItem(tx);
        n->setFont(f);
        n->setDefaultTextColor(c);
        n->setPos(p + QPointF(20, 20));
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "PolygonT") {
        QPolygonF pg;
        QPointF p;
        QPen pn;
        QBrush b;
        QTransform tr;
        ds >> pg >> p >> pn >> b >> tr;
        auto n = new QGraphicsPolygonItem(pg);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setTransform(tr);
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      } else if (t == "Polygon") {
        QPolygonF pg;
        QPointF p;
        QPen pn;
        QBrush b;
        ds >> pg >> p >> pn >> b;
        auto n = new QGraphicsPolygonItem(pg);
        n->setPen(pn);
        n->setBrush(b);
        n->setPos(p + QPointF(20, 20));
        n->setFlags(QGraphicsItem::ItemIsSelectable |
                    QGraphicsItem::ItemIsMovable);
        scene_->addItem(n);
        pi.append(n);
        addDrawAction(n);
      }
    }
    scene_->clearSelection();
    for (auto i : pi)
      if (i)
        i->setSelected(true);
    return;
  }

  // Handle image from clipboard (screenshots, copied images)
  if (md->hasImage()) {
    QImage image = qvariant_cast<QImage>(md->imageData());
    if (!image.isNull()) {
      QPointF centerPos = mapToScene(viewport()->rect().center());
      QPixmap pixmap = QPixmap::fromImage(image);
      auto *pixmapItem = new QGraphicsPixmapItem(pixmap);
      pixmapItem->setPos(centerPos -
                         QPointF(pixmap.width() / 2.0, pixmap.height() / 2.0));
      pixmapItem->setFlags(QGraphicsItem::ItemIsSelectable |
                           QGraphicsItem::ItemIsMovable);
      scene_->addItem(pixmapItem);
      addDrawAction(pixmapItem);

      scene_->clearSelection();
      pixmapItem->setSelected(true);
      emit canvasModified();
      return;
    }
  }

  // Handle plain text from clipboard (external paste) - lowest priority
  if (md->hasText()) {
    QString text = md->text().trimmed();
    if (text.isEmpty())
      return;

    // Create text item at center of visible area
    QPointF centerPos = mapToScene(viewport()->rect().center());

    auto *textItem = new LatexTextItem();
    textItem->setText(text);
    textItem->setFont(QFont("Arial", qMax(12, currentPen_.width() * 3)));
    textItem->setTextColor(currentPen_.color());
    textItem->setPos(centerPos);
    scene_->addItem(textItem);
    addDrawAction(textItem);

    scene_->clearSelection();
    textItem->setSelected(true);
    emit canvasModified();
    return;
  }

  // Unsupported clipboard content - silently ignore
}

void Canvas::addPoint(const QPointF &point) {
  if (!currentPath_)
    return;
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
  if (pointBuffer_.size() > mpr)
    pointBuffer_.removeFirst();
}

void Canvas::addPressurePoint(const QPointF &point, qreal pressure) {
  if (!currentPath_)
    return;

  if (pointBuffer_.isEmpty()) {
    pointBuffer_.append(point);
    pressureBuffer_.append(pressure);
    return;
  }

  QPointF prev = pointBuffer_.last();
  qreal dist = QLineF(prev, point).length();
  if (dist < MIN_POINT_DISTANCE)
    return;

  pointBuffer_.append(point);
  pressureBuffer_.append(pressure);

  // Build the filled stroke outline from the accumulated points
  if (pointBuffer_.size() < 2)
    return;

  QPainterPath outline;
  QVector<QPointF> leftSide;
  QVector<QPointF> rightSide;

  for (int i = 0; i < pointBuffer_.size(); ++i) {
    qreal p = pressureBuffer_.at(i);
    qreal w =
        currentPen_.widthF() * qBound(MIN_PRESSURE_THRESHOLD, p, 1.0) * 0.5;

    QPointF dir;
    if (i == 0) {
      dir = pointBuffer_.at(1) - pointBuffer_.at(0);
    } else if (i == pointBuffer_.size() - 1) {
      dir = pointBuffer_.at(i) - pointBuffer_.at(i - 1);
    } else {
      dir = pointBuffer_.at(i + 1) - pointBuffer_.at(i - 1);
    }

    qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (len < LENGTH_EPSILON)
      continue;

    QPointF normal(-dir.y() / len, dir.x() / len);
    leftSide.append(pointBuffer_.at(i) + normal * w);
    rightSide.append(pointBuffer_.at(i) - normal * w);
  }

  if (leftSide.size() < 2)
    return;

  // Build a closed path: left side forward, right side backward
  outline.moveTo(leftSide.first());
  for (int i = 1; i < leftSide.size(); ++i)
    outline.lineTo(leftSide.at(i));
  for (int i = rightSide.size() - 1; i >= 0; --i)
    outline.lineTo(rightSide.at(i));
  outline.closeSubpath();

  currentPath_->setPath(outline);

  // Keep a sliding window to avoid unbounded growth
  if (pointBuffer_.size() > MAX_PRESSURE_BUFFER_SIZE) {
    pointBuffer_.remove(0, PRESSURE_BUFFER_TRIM_SIZE);
    pressureBuffer_.remove(0, PRESSURE_BUFFER_TRIM_SIZE);
  }
}

void Canvas::eraseAt(const QPointF &point) {
  if (!scene_)
    return;
  clearTransformHandles();
  qreal sz = eraserPen_.width();
  QRectF er(point.x() - sz / 2, point.y() - sz / 2, sz, sz);
  QPainterPath ep;
  ep.addEllipse(er);
  // Copy the list to avoid modifying the scene while iterating
  QList<QGraphicsItem *> itemsToCheck = scene_->items(er);
  QList<QGraphicsItem *> itemsToRemove;
  for (QGraphicsItem *item : itemsToCheck) {
    if (!item)
      continue;
    if (item == eraserPreview_ || item == backgroundImage_ ||
        item == colorSelectionOverlay_)
      continue;
    if (item->type() == TransformHandleItem::Type)
      continue;
    QPainterPath itemShape = item->sceneTransform().map(item->shape());
    // Check if eraser intersects either the item's shape (for filled items like
    // pixmaps) or the stroked outline (for line-based items like paths)
    QPainterPathStroker s;
    s.setWidth(1);
    if (ep.intersects(itemShape) || ep.intersects(s.createStroke(itemShape))) {
      itemsToRemove.append(item);
    }
  }
  // Now safely remove items after iteration is complete
  for (QGraphicsItem *item : itemsToRemove) {
    if (!item)
      continue;
    addDeleteAction(item);
    if (sceneController_) {
      sceneController_->removeItem(item, true);
    } else {
      scene_->removeItem(item);
      onItemRemoved(item);
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

#ifdef HAVE_QT_SVG
        // Check if the file is an SVG
        if (extension == "svg") {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          QPointF dropPosition = mapToScene(event->position().toPoint());
#else
          QPointF dropPosition = mapToScene(event->pos());
#endif
          importSvg(filePath, dropPosition);
        } else
#endif
          // Check if the file is a supported image format
          if (SUPPORTED_IMAGE_EXTENSIONS.contains(extension)) {
            // Get the drop position in scene coordinates
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPointF dropPosition = mapToScene(event->position().toPoint());
#else
          QPointF dropPosition = mapToScene(event->pos());
#endif

            // Load the dropped image
            loadDroppedImage(filePath, dropPosition);
          } else {
            // Inform user about unsupported file type
            QMessageBox::warning(
                this, "Unsupported File",
                QString(
                    "File '%1' is not a supported format.\n\nSupported "
#ifdef HAVE_QT_SVG
                    "formats: PNG, JPG, JPEG, BMP, GIF, WebP, TIFF, SVG, PDF")
#else
                  "formats: PNG, JPG, JPEG, BMP, GIF, WebP, TIFF, PDF")
#endif
                    .arg(QFileInfo(filePath).fileName()));
          }
      }
    }

    event->acceptProposedAction();
    return;
  }

  QGraphicsView::dropEvent(event);
}

void Canvas::loadDroppedImage(const QString &filePath,
                              const QPointF &dropPosition) {
  // Load the image
  QPixmap pixmap(filePath);

  if (pixmap.isNull()) {
    // Show error message for invalid image
    QMessageBox::warning(this, "Invalid Image",
                         QString("Failed to load image from '%1'.\n\nThe file "
                                 "may be corrupted or not a valid image.")
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
    QPixmap scaledPixmap = pixmap.scaled(
        newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Create a graphics pixmap item
    QGraphicsPixmapItem *pixmapItem = new QGraphicsPixmapItem(scaledPixmap);

    // Position the item at the drop location (centered)
    pixmapItem->setPos(dropPosition.x() - newWidth / 2.0,
                       dropPosition.y() - newHeight / 2.0);

    // Make the item selectable and movable
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);

    // Add to scene via SceneController if available
    if (sceneController_) {
      sceneController_->addItem(pixmapItem);
    } else {
      scene_->addItem(pixmapItem);
    }

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

  // Show dialog to specify dimensions (same as loadDroppedImage)
  ImageSizeDialog dialog(pixmap.width(), pixmap.height(), this);

  if (dialog.exec() == QDialog::Accepted) {
    int newWidth = dialog.getWidth();
    int newHeight = dialog.getHeight();

    QPixmap scaledPixmap = pixmap.scaled(
        newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Create a graphics pixmap item
    QGraphicsPixmapItem *pixmapItem = new QGraphicsPixmapItem(scaledPixmap);

    // Position the item at the center of the visible area
    QPointF centerPos = mapToScene(viewport()->rect().center());
    pixmapItem->setPos(centerPos.x() - newWidth / 2.0,
                       centerPos.y() - newHeight / 2.0);

    // Make the item selectable and movable
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);

    // Add to scene via SceneController if available
    if (sceneController_) {
      sceneController_->addItem(pixmapItem);
    } else {
      scene_->addItem(pixmapItem);
    }

    // Select the newly added item
    scene_->clearSelection();
    pixmapItem->setSelected(true);

    // Add to undo stack
    addDrawAction(pixmapItem);

    emit canvasModified();
  }
}

void Canvas::contextMenuEvent(QContextMenuEvent *event) {
  if (!scene_) {
    QGraphicsView::contextMenuEvent(event);
    return;
  }

  QMenu contextMenu(this);

  // Clipboard actions - always available
  auto md = QApplication::clipboard()->mimeData();
  bool canPaste = md && (md->hasFormat("application/x-canvas-items") ||
                         md->hasImage() || md->hasText());

  QAction *pasteAction = contextMenu.addAction("Paste");
  pasteAction->setShortcut(QKeySequence::Paste);
  pasteAction->setEnabled(canPaste);
  connect(pasteAction, &QAction::triggered, this, &Canvas::pasteItems);

  // Selection-specific actions
  if (!scene_->selectedItems().isEmpty()) {
    contextMenu.addSeparator();

    QAction *copyAction = contextMenu.addAction("Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &Canvas::copySelectedItems);

    QAction *cutAction = contextMenu.addAction("Cut");
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, this, &Canvas::cutSelectedItems);

    QAction *deleteAction = contextMenu.addAction("Delete");
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this,
            &Canvas::deleteSelectedItems);

    QAction *duplicateAction = contextMenu.addAction("Duplicate");
    duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(duplicateAction, &QAction::triggered, this,
            &Canvas::duplicateSelectedItems);

    contextMenu.addSeparator();

    // Arrange submenu for z-order control
    QMenu *arrangeMenu = contextMenu.addMenu("Arrange");

    QAction *bringToFrontAction = arrangeMenu->addAction("Bring to Front");
    bringToFrontAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_BracketRight));
    connect(bringToFrontAction, &QAction::triggered, this,
            &Canvas::bringToFront);

    QAction *bringForwardAction = arrangeMenu->addAction("Bring Forward");
    bringForwardAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_BracketRight));
    connect(bringForwardAction, &QAction::triggered, this,
            &Canvas::bringForward);

    QAction *sendBackwardAction = arrangeMenu->addAction("Send Backward");
    sendBackwardAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_BracketLeft));
    connect(sendBackwardAction, &QAction::triggered, this,
            &Canvas::sendBackward);

    QAction *sendToBackAction = arrangeMenu->addAction("Send to Back");
    sendToBackAction->setShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_BracketLeft));
    connect(sendToBackAction, &QAction::triggered, this, &Canvas::sendToBack);

    contextMenu.addSeparator();

    QAction *exportSVGAction = contextMenu.addAction("Export Selection as SVG");
    QAction *exportPNGAction = contextMenu.addAction("Export Selection as PNG");
    QAction *exportJPGAction = contextMenu.addAction("Export Selection as JPG");
    QAction *exportWebPAction =
        contextMenu.addAction("Export Selection as WebP");
    QAction *exportTIFFAction =
        contextMenu.addAction("Export Selection as TIFF");

#ifdef HAVE_QT_SVG
    connect(exportSVGAction, &QAction::triggered, this,
            &Canvas::exportSelectionToSVG);
#else
    exportSVGAction->setEnabled(false);
    exportSVGAction->setText("Export Selection as SVG (requires Qt SVG)");
#endif
    connect(exportPNGAction, &QAction::triggered, this,
            &Canvas::exportSelectionToPNG);
    connect(exportJPGAction, &QAction::triggered, this,
            &Canvas::exportSelectionToJPG);
    connect(exportWebPAction, &QAction::triggered, this,
            &Canvas::exportSelectionToWebP);
    connect(exportTIFFAction, &QAction::triggered, this,
            &Canvas::exportSelectionToTIFF);

    contextMenu.addSeparator();

    QAction *perspectiveAction =
        contextMenu.addAction("Perspective Transform...");
    connect(perspectiveAction, &QAction::triggered, this,
            &Canvas::perspectiveTransformSelectedItems);
  }

  if (hasActiveColorSelection()) {
    contextMenu.addSeparator();
    QAction *extractAction =
        contextMenu.addAction("Extract Color Selection to New Layer");
    connect(extractAction, &QAction::triggered, this,
            &Canvas::extractColorSelectionToNewLayer);

    QAction *clearSelectionAction =
        contextMenu.addAction("Clear Color Selection");
    connect(clearSelectionAction, &QAction::triggered, this,
            &Canvas::clearColorSelection);
  }

  contextMenu.exec(event->globalPos());
}

QRectF Canvas::getSelectionBoundingRect() const {
  if (!scene_)
    return QRectF();
  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  if (selectedItems.isEmpty())
    return QRectF();

  QRectF boundingRect;
  bool firstItem = true;
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
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
                           "SVG export requires the Qt SVG module, which was "
                           "not found at build time.");
  return;
#else
  if (!scene_ || scene_->selectedItems().isEmpty())
    return;

  QString fileName = QFileDialog::getSaveFileName(
      this, "Export Selection as SVG", "", "SVG (*.svg)");
  if (fileName.isEmpty())
    return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty())
    return;

  // Create SVG generator
  QSvgGenerator generator;
  generator.setFileName(fileName);
  generator.setSize(boundingRect.size().toSize());
  generator.setViewBox(
      QRect(0, 0, boundingRect.width(), boundingRect.height()));
  generator.setTitle("Exported Selection");
  generator.setDescription(
      "Selected items exported from FullScreen Pencil Draw");

  // Render selected items to SVG
  QPainter painter;
  painter.begin(&generator);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());

  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      painter.save();
      painter.setTransform(item->sceneTransform(), true);
      item->paint(&painter, nullptr, nullptr);
      painter.restore();
    }
  }

  painter.end();
#endif
}

void Canvas::importSvg(const QString &filePath, const QPointF &position) {
#ifndef HAVE_QT_SVG
  Q_UNUSED(filePath)
  Q_UNUSED(position)
  QMessageBox::information(this, "SVG Import Unavailable",
                           "SVG import requires the Qt SVG module, which was "
                           "not found at build time.");
#else
  QSvgRenderer renderer(filePath);
  if (!renderer.isValid()) {
    QMessageBox::warning(
        this, "Invalid SVG",
        QString("Failed to load SVG from '%1'.\n\nThe file may be corrupted or "
                "not a valid SVG.")
            .arg(QFileInfo(filePath).fileName()));
    return;
  }

  QSize svgSize = renderer.defaultSize();
  if (svgSize.isEmpty())
    svgSize = QSize(400, 400);

  QImage image(svgSize, QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  QPainter painter(&image);
  renderer.render(&painter);
  painter.end();

  QPixmap pixmap = QPixmap::fromImage(image);
  QGraphicsPixmapItem *pixmapItem = new QGraphicsPixmapItem(pixmap);

  pixmapItem->setPos(position.x() - svgSize.width() / 2.0,
                     position.y() - svgSize.height() / 2.0);
  pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);

  if (sceneController_) {
    sceneController_->addItem(pixmapItem);
  } else {
    scene_->addItem(pixmapItem);
  }
  addDrawAction(pixmapItem);
#endif
}

void Canvas::exportSelectionToPNG() {
  if (!scene_ || scene_->selectedItems().isEmpty())
    return;

  QString fileName = QFileDialog::getSaveFileName(
      this, "Export Selection as PNG", "", "PNG (*.png)");
  if (fileName.isEmpty())
    return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty())
    return;

  // Create image and render selected items
  QImage image(boundingRect.size().toSize(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());

  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
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
  if (!scene_ || scene_->selectedItems().isEmpty())
    return;

  QString fileName = QFileDialog::getSaveFileName(
      this, "Export Selection as JPG", "", "JPEG (*.jpg)");
  if (fileName.isEmpty())
    return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty())
    return;

  // Create image with white background and render selected items
  QImage image(boundingRect.size().toSize(), QImage::Format_RGB32);
  image.fill(Qt::white);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());

  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      painter.save();
      painter.setTransform(item->sceneTransform(), true);
      item->paint(&painter, nullptr, nullptr);
      painter.restore();
    }
  }

  painter.end();
  image.save(fileName);
}

void Canvas::exportSelectionToWebP() {
  if (!scene_ || scene_->selectedItems().isEmpty())
    return;

  QString fileName = QFileDialog::getSaveFileName(
      this, "Export Selection as WebP", "", "WebP (*.webp)");
  if (fileName.isEmpty())
    return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty())
    return;

  // Create image with transparency and render selected items
  QImage image(boundingRect.size().toSize(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());

  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      painter.save();
      painter.setTransform(item->sceneTransform(), true);
      item->paint(&painter, nullptr, nullptr);
      painter.restore();
    }
  }

  painter.end();
  image.save(fileName, "WEBP");
}

void Canvas::exportSelectionToTIFF() {
  if (!scene_ || scene_->selectedItems().isEmpty())
    return;

  QString fileName = QFileDialog::getSaveFileName(
      this, "Export Selection as TIFF", "", "TIFF (*.tiff *.tif)");
  if (fileName.isEmpty())
    return;

  QRectF boundingRect = getSelectionBoundingRect();
  if (boundingRect.isEmpty())
    return;

  // Create image with transparency and render selected items
  QImage image(boundingRect.size().toSize(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.translate(-boundingRect.topLeft());

  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    if (item != eraserPreview_ && item != backgroundImage_ &&
        item != colorSelectionOverlay_) {
      painter.save();
      painter.setTransform(item->sceneTransform(), true);
      item->paint(&painter, nullptr, nullptr);
      painter.restore();
    }
  }

  painter.end();
  image.save(fileName, "TIFF");
}

void Canvas::updateTransformHandles() {
  // Only show transform handles when in selection mode
  if (currentShape_ != Selection) {
    clearTransformHandles();
    return;
  }

  if (!scene_)
    return;
  ItemStore *store = itemStore();
  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();

  // Remove handles for items that are no longer selected
  QMutableListIterator<TransformHandleItem *> it(transformHandles_);
  while (it.hasNext()) {
    TransformHandleItem *handle = it.next();
    if (!handle) {
      it.remove();
      continue;
    }
    bool stillSelected = false;
    if (store && handle->targetItemId().isValid()) {
      for (QGraphicsItem *item : selectedItems) {
        if (!item)
          continue;
        ItemId id = store->idForItem(item);
        if (id.isValid() && id == handle->targetItemId()) {
          stillSelected = true;
          break;
        }
      }
    } else {
      stillSelected = selectedItems.contains(handle->targetItem());
    }
    if (!stillSelected) {
      if (handle) {
        // Clear target item reference before deleting to avoid dangling pointer
        // access
        handle->clearTargetItem();
        if (scene_)
          scene_->removeItem(handle);
        delete handle;
      }
      it.remove();
    }
  }

  // Add handles for newly selected items
  for (QGraphicsItem *item : selectedItems) {
    if (!item)
      continue;
    // Skip non-transformable items
    if (item == eraserPreview_ || item == backgroundImage_ ||
        item == colorSelectionOverlay_)
      continue;

    // Skip TransformHandleItems themselves (check by type)
    if (item->type() == TransformHandleItem::Type)
      continue;

    // Check if handle already exists for this item
    bool hasHandle = false;
    for (TransformHandleItem *handle : transformHandles_) {
      if (!handle)
        continue;
      if (store && handle->targetItemId().isValid()) {
        ItemId id = store->idForItem(item);
        if (id.isValid() && handle->targetItemId() == id) {
          hasHandle = true;
          handle->updateHandles();
          break;
        }
      } else if (handle->targetItem() == item) {
        hasHandle = true;
        handle->updateHandles();
        break;
      }
    }

    if (!hasHandle) {
      TransformHandleItem *handle = nullptr;
      if (store) {
        ItemId id = store->idForItem(item);
        if (!id.isValid()) {
          id = registerItem(item);
        }
        if (id.isValid()) {
          handle = new TransformHandleItem(id, store, this);
        }
      }
      if (!handle) {
        qWarning() << "Cannot create TransformHandleItem without ItemStore";
        continue;
      }
      scene_->addItem(handle);
      transformHandles_.append(handle);

      // Connect to update handles when transform completes
      connect(handle, &TransformHandleItem::transformCompleted, this,
              [this, handle]() {
                if (handle)
                  handle->updateHandles();
                emit canvasModified();
              });

      // Connect to apply resize/rotation to other selected items
      connect(
          handle, &TransformHandleItem::resizeApplied, this,
          [this, handle](qreal scaleX, qreal scaleY, const QPointF &anchor) {
            applyResizeToOtherItems(handle->targetItem(), scaleX, scaleY,
                                    anchor);
          });
      connect(handle, &TransformHandleItem::rotationApplied, this,
              [this, handle](qreal angleDelta, const QPointF &center) {
                applyRotationToOtherItems(handle->targetItem(), angleDelta,
                                          center);
              });
    }
  }
}

void Canvas::clearTransformHandles() {
  for (TransformHandleItem *handle : transformHandles_) {
    if (handle) {
      // Clear target item reference before deleting to avoid dangling pointer
      // access
      handle->clearTargetItem();
      if (scene_)
        scene_->removeItem(handle);
      delete handle;
    }
  }
  transformHandles_.clear();
}

void Canvas::applyResizeToOtherItems(QGraphicsItem *sourceItem, qreal scaleX,
                                     qreal scaleY, const QPointF &anchor) {
  if (!scene_)
    return;

  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item || item == sourceItem)
      continue;
    if (item == eraserPreview_ || item == backgroundImage_ ||
        item == colorSelectionOverlay_)
      continue;
    if (item->type() == TransformHandleItem::Type)
      continue;

    // Handle text items specially - adjust font size instead of transform
    // scaling, then reposition relative to the shared anchor.
    if (LatexTextItem *textItem = dynamic_cast<LatexTextItem *>(item)) {
      // Prefer the axis that actually changed; use average for corner drags.
      qreal dx = qAbs(scaleX - 1.0);
      qreal dy = qAbs(scaleY - 1.0);
      qreal uniformScale = 1.0;
      if (dx > 0.0001 && dy > 0.0001) {
        uniformScale = (scaleX + scaleY) / 2.0;
      } else if (dx > 0.0001) {
        uniformScale = scaleX;
      } else if (dy > 0.0001) {
        uniformScale = scaleY;
      }

      QFont currentFont = textItem->font();
      qreal currentSize = currentFont.pointSizeF();
      if (currentSize <= 0.0) {
        currentSize = static_cast<qreal>(currentFont.pointSize());
      }
      if (currentSize <= 0.0) {
        currentSize = 14.0;
      }

      qreal newSize = qBound(8.0, currentSize * uniformScale, 256.0);
      if (qAbs(newSize - currentSize) > 0.01) {
        currentFont.setPointSizeF(newSize);
        textItem->setFont(currentFont);
      }

      // Reposition so the text's center moves relative to the shared anchor
      // just like every other item, preserving relative layout.
      QRectF bounds =
          textItem->mapToScene(textItem->boundingRect()).boundingRect();
      QPointF center = bounds.center();
      QPointF newCenter = anchor + QPointF((center.x() - anchor.x()) * scaleX,
                                           (center.y() - anchor.y()) * scaleY);
      QPointF localCenter = textItem->mapFromScene(center);
      QPointF newSceneCenter = textItem->mapToScene(localCenter);
      textItem->setPos(textItem->pos() + (newCenter - newSceneCenter));
      continue;
    }

    // Scale around the shared anchor point so relative positions are preserved
    QPointF localAnchor = item->mapFromScene(anchor);

    QTransform scaleTransform;
    scaleTransform.translate(localAnchor.x(), localAnchor.y());
    scaleTransform.scale(scaleX, scaleY);
    scaleTransform.translate(-localAnchor.x(), -localAnchor.y());

    item->setTransform(item->transform() * scaleTransform);

    // Keep the anchor point fixed in scene space
    QPointF newAnchorPos = item->mapToScene(localAnchor);
    QPointF posAdjust = anchor - newAnchorPos;
    item->setPos(item->pos() + posAdjust);
  }

  // Update all transform handles
  for (TransformHandleItem *handle : transformHandles_) {
    if (handle)
      handle->updateHandles();
  }
}

void Canvas::applyRotationToOtherItems(QGraphicsItem *sourceItem,
                                       qreal angleDelta,
                                       const QPointF &center) {
  if (!scene_)
    return;

  QList<QGraphicsItem *> selectedItems = scene_->selectedItems();
  for (QGraphicsItem *item : selectedItems) {
    if (!item || item == sourceItem)
      continue;
    if (item == eraserPreview_ || item == backgroundImage_ ||
        item == colorSelectionOverlay_)
      continue;
    if (item->type() == TransformHandleItem::Type)
      continue;

    // Rotate around the shared center so relative positions are preserved
    QPointF localCenter = item->mapFromScene(center);

    QTransform rotateTransform;
    rotateTransform.translate(localCenter.x(), localCenter.y());
    rotateTransform.rotate(angleDelta);
    rotateTransform.translate(-localCenter.x(), -localCenter.y());

    item->setTransform(item->transform() * rotateTransform);

    // Keep the shared center fixed in scene space
    QPointF newCenterPos = item->mapToScene(localCenter);
    QPointF posAdjust = center - newCenterPos;
    item->setPos(item->pos() + posAdjust);
  }

  // Update all transform handles
  for (TransformHandleItem *handle : transformHandles_) {
    if (handle)
      handle->updateHandles();
  }
}

void Canvas::scaleSelectedItems() {
  if (!scene_) {
    return;
  }

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty()) {
    return;
  }

  ScaleDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  double sx = dialog.scaleX();
  double sy = dialog.scaleY();
  if (qFuzzyCompare(sx, 1.0) && qFuzzyCompare(sy, 1.0)) {
    return;
  }

  // Compute combined bounding center
  QRectF bounds;
  for (QGraphicsItem *item : selected) {
    bounds =
        bounds.united(item->mapToScene(item->boundingRect()).boundingRect());
  }
  QPointF center = bounds.center();

  for (QGraphicsItem *item : selected) {
    QTransform oldTransform = item->transform();
    QPointF oldPos = item->pos();

    QTransform newTransform = oldTransform;
    newTransform.scale(sx, sy);
    item->setTransform(newTransform);

    QPointF offset = oldPos - center;
    QPointF newPos = center + QPointF(offset.x() * sx, offset.y() * sy);
    item->setPos(newPos);

    // Record undo action
    if (sceneController_) {
      ItemId id = sceneController_->idForItem(item);
      if (id.isValid()) {
        addAction(std::make_unique<TransformAction>(
            id, sceneController_->itemStore(), oldTransform, item->transform(),
            oldPos, item->pos()));
      }
    }
  }

  updateTransformHandles();
  emit canvasModified();
}

void Canvas::perspectiveTransformSelectedItems() {
  if (!scene_) {
    return;
  }

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty()) {
    return;
  }

  PerspectiveTransformDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  // Compute combined bounding rect of selected items
  QRectF bounds;
  for (QGraphicsItem *item : selected) {
    bounds =
        bounds.united(item->mapToScene(item->boundingRect()).boundingRect());
  }

  QTransform perspective = dialog.perspectiveTransform(bounds);
  if (perspective.isIdentity()) {
    return;
  }

  for (QGraphicsItem *item : selected) {
    QTransform oldTransform = item->transform();
    QPointF oldPos = item->pos();

    // The perspective transform is defined in scene coordinates.
    // To apply it correctly we translate into scene space, apply the
    // perspective, then translate back into item-local space.
    //   localToScene = item->sceneTransform()
    //   newSceneTransform = localToScene * perspective
    // However setTransform() only sets the item-local transform (not
    // including pos), so we factor pos out.
    //
    // sceneTransform() == T(pos) * itemTransform
    // We want: T(newPos) * newItemTransform == oldScene * perspective
    //
    // Compute the full new scene matrix, then extract pos and local
    // transform separately.
    QTransform oldScene = item->sceneTransform();
    QTransform newScene = oldScene * perspective;

    // The position is the translation component of the scene transform
    // when the item-local transform has no translation itself (which is
    // the convention used throughout this codebase).  We use the origin
    // mapping to get the new position.
    QPointF newPos = newScene.map(QPointF(0, 0));

    // Remove the translation to get the pure local transform.
    QTransform newLocal =
        newScene * QTransform::fromTranslate(-newPos.x(), -newPos.y());

    item->setTransform(newLocal);
    item->setPos(newPos);

    // Record undo action
    if (sceneController_) {
      ItemId id = sceneController_->idForItem(item);
      if (id.isValid()) {
        addAction(std::make_unique<TransformAction>(
            id, sceneController_->itemStore(), oldTransform, item->transform(),
            oldPos, item->pos()));
      }
    }
  }

  updateTransformHandles();
  emit canvasModified();
}

void Canvas::scaleActiveLayer() {
  if (!layerManager_ || !sceneController_) {
    return;
  }

  Layer *layer = layerManager_->activeLayer();
  if (!layer || layer->itemCount() == 0) {
    return;
  }

  ScaleDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  double sx = dialog.scaleX();
  double sy = dialog.scaleY();
  if (qFuzzyCompare(sx, 1.0) && qFuzzyCompare(sy, 1.0)) {
    return;
  }

  // Save state for undo before scaling
  struct ItemState {
    ItemId id;
    QTransform transform;
    QPointF pos;
  };
  QList<ItemState> oldStates;
  for (const ItemId &id : layer->itemIds()) {
    QGraphicsItem *item = sceneController_->item(id);
    if (item) {
      oldStates.append({id, item->transform(), item->pos()});
    }
  }

  sceneController_->scaleLayer(layer, sx, sy);

  // Record undo actions
  for (const ItemState &state : oldStates) {
    QGraphicsItem *item = sceneController_->item(state.id);
    if (item) {
      addAction(std::make_unique<TransformAction>(
          state.id, sceneController_->itemStore(), state.transform,
          item->transform(), state.pos, item->pos()));
    }
  }

  updateTransformHandles();
  emit canvasModified();
}

void Canvas::resizeCanvas() {
  if (!scene_)
    return;

  int oldW = static_cast<int>(scene_->sceneRect().width());
  int oldH = static_cast<int>(scene_->sceneRect().height());

  ResizeCanvasDialog dialog(oldW, oldH, this);
  if (dialog.exec() != QDialog::Accepted)
    return;

  int newW = dialog.getWidth();
  int newH = dialog.getHeight();
  if (newW == oldW && newH == oldH)
    return;

  // Compute offset based on anchor position
  int dw = newW - oldW;
  int dh = newH - oldH;
  qreal offsetX = 0, offsetY = 0;

  switch (dialog.getAnchor()) {
  case ResizeCanvasDialog::TopLeft:
    break;
  case ResizeCanvasDialog::TopCenter:
    offsetX = dw / 2.0;
    break;
  case ResizeCanvasDialog::TopRight:
    offsetX = dw;
    break;
  case ResizeCanvasDialog::MiddleLeft:
    offsetY = dh / 2.0;
    break;
  case ResizeCanvasDialog::Center:
    offsetX = dw / 2.0;
    offsetY = dh / 2.0;
    break;
  case ResizeCanvasDialog::MiddleRight:
    offsetX = dw;
    offsetY = dh / 2.0;
    break;
  case ResizeCanvasDialog::BottomLeft:
    offsetY = dh;
    break;
  case ResizeCanvasDialog::BottomCenter:
    offsetX = dw / 2.0;
    offsetY = dh;
    break;
  case ResizeCanvasDialog::BottomRight:
    offsetX = dw;
    offsetY = dh;
    break;
  }

  // Move existing items so they stay at the correct position relative to anchor
  if (offsetX != 0.0 || offsetY != 0.0) {
    for (auto *item : scene_->items()) {
      if (!item)
        continue;
      if (item == eraserPreview_ || item == colorSelectionOverlay_)
        continue;
      item->moveBy(offsetX, offsetY);
    }
  }

  scene_->setSceneRect(0, 0, newW, newH);
  emit canvasModified();
}

void Canvas::applyBlurToSelection() {
  if (!scene_ || !sceneController_)
    return;

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty())
    return;

  bool ok = false;
  int radius = QInputDialog::getInt(this, "Blur", "Radius:", 3, 1, 20, 1, &ok);
  if (!ok)
    return;

  bool applied = false;
  for (QGraphicsItem *item : selected) {
    auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
    if (!pixmapItem)
      continue;

    QImage oldImage = pixmapItem->pixmap().toImage();
    QImage newImage = ImageFilters::blur(oldImage, radius);
    pixmapItem->setPixmap(QPixmap::fromImage(newImage));

    ItemId id = sceneController_->idForItem(pixmapItem);
    if (id.isValid()) {
      addAction(std::make_unique<RasterPixmapAction>(
          id, sceneController_->itemStore(), oldImage, newImage));
    }
    applied = true;
  }

  if (applied)
    emit canvasModified();
}

void Canvas::applySharpenToSelection() {
  if (!scene_ || !sceneController_)
    return;

  QList<QGraphicsItem *> selected = scene_->selectedItems();
  if (selected.isEmpty())
    return;

  bool ok = false;
  double strength = QInputDialog::getDouble(this, "Sharpen", "Strength:", 1.0,
                                            0.1, 5.0, 1, &ok);
  if (!ok)
    return;

  bool applied = false;
  for (QGraphicsItem *item : selected) {
    auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
    if (!pixmapItem)
      continue;

    QImage oldImage = pixmapItem->pixmap().toImage();
    QImage newImage = ImageFilters::sharpen(oldImage, 3, strength);
    pixmapItem->setPixmap(QPixmap::fromImage(newImage));

    ItemId id = sceneController_->idForItem(pixmapItem);
    if (id.isValid()) {
      addAction(std::make_unique<RasterPixmapAction>(
          id, sceneController_->itemStore(), oldImage, newImage));
    }
    applied = true;
  }

  if (applied)
    emit canvasModified();
}

void Canvas::placeElement(const QString &elementId) {
  if (!scene_)
    return;

  // Create the appropriate custom vector-drawn element
  ArchitectureElementItem *elem = nullptr;
  if (elementId == "client")
    elem = new ClientElement();
  else if (elementId == "load_balancer")
    elem = new LoadBalancerElement();
  else if (elementId == "api_gateway")
    elem = new ApiGatewayElement();
  else if (elementId == "app_server")
    elem = new AppServerElement();
  else if (elementId == "cache")
    elem = new CacheElement();
  else if (elementId == "message_queue")
    elem = new MessageQueueElement();
  else if (elementId == "database")
    elem = new DatabaseElement();
  else if (elementId == "object_storage")
    elem = new ObjectStorageElement();
  else if (elementId == "auth")
    elem = new AuthElement();
  else if (elementId == "monitoring")
    elem = new MonitoringElement();

  if (!elem)
    return;

  // Position at the centre of the visible viewport
  QRectF br = elem->boundingRect();
  QPointF center = mapToScene(viewport()->rect().center());
  elem->setPos(center.x() - br.width() / 2.0, center.y() - br.height() / 2.0);

  // Add to scene via SceneController
  if (sceneController_) {
    sceneController_->addItem(elem);
  } else {
    scene_->addItem(elem);
  }
  addDrawAction(elem);
  emit canvasModified();
}
