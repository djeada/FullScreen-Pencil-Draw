/**
 * @file transform_handle_item.cpp
 * @brief Implementation of visual transform handles.
 * 
 * Items are tracked by ItemId only - never by raw pointer.
 */
#include "transform_handle_item.h"
#include "latex_text_item.h"
#include "../core/item_store.h"
#include "../core/scene_renderer.h"
#include "../core/transform_action.h"
#include <QCursor>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
qreal textScaleForHandle(HandleType handle, qreal scaleX, qreal scaleY) {
  switch (handle) {
  case HandleType::MiddleLeft:
  case HandleType::MiddleRight:
    return scaleX;
  case HandleType::TopCenter:
  case HandleType::BottomCenter:
    return scaleY;
  case HandleType::TopLeft:
  case HandleType::TopRight:
  case HandleType::BottomLeft:
  case HandleType::BottomRight:
    return (scaleX + scaleY) / 2.0;
  default:
    return 1.0;
  }
}

qreal effectivePointSize(const QFont &font) {
  qreal size = font.pointSizeF();
  if (size <= 0.0) {
    size = static_cast<qreal>(font.pointSize());
  }
  return (size > 0.0) ? size : 14.0;
}
} // namespace

TransformHandleItem::TransformHandleItem(const ItemId &targetId,
                                         ItemStore *store,
                                         SceneRenderer *renderer,
                                         QGraphicsItem *parent)
    : QGraphicsObject(parent), targetItemId_(targetId),
      itemStore_(store), renderer_(renderer),
      sceneEventFilterInstalled_(false), isTransforming_(false),
      activeHandle_(HandleType::None), wasMovable_(false),
      wasSelectable_(false), cachedTargetBounds_(), previousTargetBounds_() {
  setAcceptHoverEvents(true);
  setFlag(QGraphicsItem::ItemIsSelectable, false);
  setFlag(QGraphicsItem::ItemIsMovable, false);
  // High Z value so handles appear above everything
  setZValue(10000);
  ensureSceneEventFilter();
  updateHandles();
}

QGraphicsItem *TransformHandleItem::targetItem() const {
  if (!itemStore_ || !targetItemId_.isValid()) return nullptr;
  return itemStore_->item(targetItemId_);
}

QGraphicsItem *TransformHandleItem::resolveTargetItem() const {
  if (!itemStore_ || !targetItemId_.isValid()) return nullptr;
  return itemStore_->item(targetItemId_);
}

TransformHandleItem::~TransformHandleItem() {
  QGraphicsItem *target = resolveTargetItem();
  if (target && sceneEventFilterInstalled_) {
    target->removeSceneEventFilter(this);
  }
}

void TransformHandleItem::clearTargetItem() {
  QGraphicsItem *target = resolveTargetItem();
  if (target && sceneEventFilterInstalled_) {
    target->removeSceneEventFilter(this);
    sceneEventFilterInstalled_ = false;
  }
  targetItemId_ = ItemId();
}

QVariant TransformHandleItem::itemChange(GraphicsItemChange change,
                                         const QVariant &value) {
  if (change == QGraphicsItem::ItemSceneHasChanged) {
    ensureSceneEventFilter();
  }
  return QGraphicsObject::itemChange(change, value);
}

QRectF TransformHandleItem::boundingRect() const {
  QGraphicsItem *target = resolveTargetItem();
  if (!target)
    return QRectF();

  QRectF bounds = targetBoundsInScene();
  
  // Helper to expand bounds to include handles and rotation handle
  auto expandForHandles = [](const QRectF &rect) {
    return rect.adjusted(-HANDLE_SIZE, -HANDLE_SIZE - ROTATION_HANDLE_OFFSET,
                         HANDLE_SIZE, HANDLE_SIZE);
  };
  
  QRectF expandedBounds = expandForHandles(bounds);
  
  // If we have previous bounds that differ from current, include them in the
  // bounding rect to ensure the old anchor positions get repainted (cleared).
  // This prevents the visual trail artifact when items are dragged.
  // Note: previousTargetBounds_ is set in updateHandles() before prepareGeometryChange().
  if (!previousTargetBounds_.isEmpty() && previousTargetBounds_ != bounds) {
    QRectF oldExpandedBounds = expandForHandles(previousTargetBounds_);
    return expandedBounds.united(oldExpandedBounds);
  }
  
  return expandedBounds;
}

QPainterPath TransformHandleItem::shape() const {
  QPainterPath path;
  
  // Add all handle rects to shape for hit testing
  for (int i = static_cast<int>(HandleType::TopLeft);
       i <= static_cast<int>(HandleType::BottomRight); ++i) {
    HandleType type = static_cast<HandleType>(i);
    path.addRect(handleRect(type));
  }
  
  // Add rotation handle
  QRectF bounds = targetBoundsInScene();
  QPointF rotationCenter(bounds.center().x(),
                         bounds.top() - ROTATION_HANDLE_OFFSET);
  path.addEllipse(rotationCenter, ROTATION_HANDLE_RADIUS + 2, ROTATION_HANDLE_RADIUS + 2);
  
  return path;
}

QRectF TransformHandleItem::targetBoundsInScene() const {
  QGraphicsItem *target = resolveTargetItem();
  if (!target)
    return QRectF();
  return target->mapToScene(target->boundingRect()).boundingRect();
}

void TransformHandleItem::ensureSceneEventFilter() {
  QGraphicsItem *target = resolveTargetItem();
  if (!target || sceneEventFilterInstalled_)
    return;
  if (scene() && target->scene() == scene()) {
    target->installSceneEventFilter(this);
    sceneEventFilterInstalled_ = true;
  }
}

void TransformHandleItem::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem * /*option*/,
                                QWidget * /*widget*/) {
  QGraphicsItem *target = resolveTargetItem();
  if (!target)
    return;

  painter->setRenderHint(QPainter::Antialiasing);

  QRectF bounds = targetBoundsInScene();

  // Draw selection rectangle with solid blue border
  QPen borderPen(SELECTION_BORDER_COLOR, SELECTION_BORDER_WIDTH);
  borderPen.setStyle(Qt::SolidLine);
  painter->setPen(borderPen);
  painter->setBrush(Qt::NoBrush);
  painter->drawRect(bounds);

  // Draw resize handles
  QPen handlePen(HANDLE_BORDER_COLOR, 1.5);
  painter->setPen(handlePen);
  painter->setBrush(HANDLE_FILL_COLOR);

  // Draw all 8 resize handles
  for (int i = static_cast<int>(HandleType::TopLeft);
       i <= static_cast<int>(HandleType::BottomRight); ++i) {
    HandleType type = static_cast<HandleType>(i);
    QRectF rect = handleRect(type);
    painter->drawRect(rect);
  }

  // Draw rotation handle connection line
  QPointF topCenter(bounds.center().x(), bounds.top());
  QPointF rotationCenter(bounds.center().x(),
                         bounds.top() - ROTATION_HANDLE_OFFSET);

  QPen linePen(SELECTION_BORDER_COLOR, 1.0);
  linePen.setStyle(Qt::DashLine);
  painter->setPen(linePen);
  painter->drawLine(topCenter, rotationCenter);

  // Draw rotation handle (circle)
  painter->setPen(QPen(ROTATION_HANDLE_COLOR, 2.0));
  painter->setBrush(HANDLE_FILL_COLOR);
  painter->drawEllipse(rotationCenter, ROTATION_HANDLE_RADIUS,
                       ROTATION_HANDLE_RADIUS);

  // Draw rotation arrow icon inside the handle
  painter->setPen(QPen(ROTATION_HANDLE_COLOR, 1.5));
  qreal arrowSize = ROTATION_HANDLE_RADIUS * 0.6;
  painter->drawArc(
      QRectF(rotationCenter.x() - arrowSize, rotationCenter.y() - arrowSize,
             arrowSize * 2, arrowSize * 2),
      30 * 16, 120 * 16);
}

void TransformHandleItem::updateHandles() {
  // Store the current cached bounds as previous bounds before updating.
  // This allows boundingRect() to include the old region for proper repainting,
  // which clears the old anchor positions when the item moves.
  previousTargetBounds_ = cachedTargetBounds_;
  
  // Notify the scene that geometry is about to change.
  // At this point, boundingRect() will return a union of old and new bounds
  // because previousTargetBounds_ holds the old position.
  prepareGeometryChange();
  
  // Update the cached bounds to the new position
  cachedTargetBounds_ = targetBoundsInScene();
  
  // Note: We don't clear previousTargetBounds_ here because prepareGeometryChange()
  // has already captured the bounding rect. The previous bounds will be updated
  // on the next call to updateHandles().
  
  update();
}

HandleType TransformHandleItem::handleAtPoint(const QPointF &pos) const {
  // Check rotation handle first
  QRectF bounds = targetBoundsInScene();
  QPointF rotationCenter(bounds.center().x(),
                         bounds.top() - ROTATION_HANDLE_OFFSET);
  if (QLineF(pos, rotationCenter).length() <= ROTATION_HANDLE_RADIUS + 4)
    return HandleType::Rotate;

  // Check resize handles
  for (int i = static_cast<int>(HandleType::TopLeft);
       i <= static_cast<int>(HandleType::BottomRight); ++i) {
    HandleType type = static_cast<HandleType>(i);
    if (handleRect(type).contains(pos))
      return type;
  }

  return HandleType::None;
}

QRectF TransformHandleItem::handleRect(HandleType type) const {
  QGraphicsItem *target = resolveTargetItem();
  if (!target)
    return QRectF();

  QRectF bounds = targetBoundsInScene();
  QPointF center;

  switch (type) {
  case HandleType::TopLeft:
    center = bounds.topLeft();
    break;
  case HandleType::TopCenter:
    center = QPointF(bounds.center().x(), bounds.top());
    break;
  case HandleType::TopRight:
    center = bounds.topRight();
    break;
  case HandleType::MiddleLeft:
    center = QPointF(bounds.left(), bounds.center().y());
    break;
  case HandleType::MiddleRight:
    center = QPointF(bounds.right(), bounds.center().y());
    break;
  case HandleType::BottomLeft:
    center = bounds.bottomLeft();
    break;
  case HandleType::BottomCenter:
    center = QPointF(bounds.center().x(), bounds.bottom());
    break;
  case HandleType::BottomRight:
    center = bounds.bottomRight();
    break;
  default:
    return QRectF();
  }

  return QRectF(center.x() - HANDLE_HALF, center.y() - HANDLE_HALF, HANDLE_SIZE,
                HANDLE_SIZE);
}

QCursor TransformHandleItem::cursorForHandle(HandleType type) const {
  switch (type) {
  case HandleType::TopLeft:
  case HandleType::BottomRight:
    return Qt::SizeFDiagCursor;
  case HandleType::TopRight:
  case HandleType::BottomLeft:
    return Qt::SizeBDiagCursor;
  case HandleType::TopCenter:
  case HandleType::BottomCenter:
    return Qt::SizeVerCursor;
  case HandleType::MiddleLeft:
  case HandleType::MiddleRight:
    return Qt::SizeHorCursor;
  case HandleType::Rotate:
    return Qt::CrossCursor;
  default:
    return Qt::ArrowCursor;
  }
}

void TransformHandleItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
  HandleType handle = handleAtPoint(event->scenePos());
  if (handle != HandleType::None) {
    setCursor(cursorForHandle(handle));
  } else {
    unsetCursor();
  }
  QGraphicsObject::hoverMoveEvent(event);
}

bool TransformHandleItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
  QGraphicsItem *target = resolveTargetItem();
  if (watched == target) {
    QEvent::Type eventType = event->type();
    if (eventType == QEvent::GraphicsSceneMove ||
        eventType == QEvent::GraphicsSceneMouseMove ||
        eventType == QEvent::GraphicsSceneMouseRelease) {
      updateHandles();
    }
  }
  return QGraphicsObject::sceneEventFilter(watched, event);
}

void TransformHandleItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  unsetCursor();
  QGraphicsObject::hoverLeaveEvent(event);
}

void TransformHandleItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() != Qt::LeftButton) {
    event->ignore();
    return;
  }

  activeHandle_ = handleAtPoint(event->scenePos());
  if (activeHandle_ == HandleType::None) {
    event->ignore();
    return;
  }

  event->accept();
  isTransforming_ = true;
  lastMousePos_ = event->scenePos();

  // Store original state for undo
  QGraphicsItem *target = resolveTargetItem();
  if (target) {
    originalTransform_ = target->transform();
    originalPos_ = target->pos();
    originalBounds_ = targetBoundsInScene();
    transformOrigin_ = originalBounds_.center();
    
    // Temporarily disable item's movable/selectable flags to prevent interference
    wasMovable_ = target->flags() & QGraphicsItem::ItemIsMovable;
    wasSelectable_ = target->flags() & QGraphicsItem::ItemIsSelectable;
    target->setFlag(QGraphicsItem::ItemIsMovable, false);
  }
}

void TransformHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (!isTransforming_ || activeHandle_ == HandleType::None) {
    event->ignore();
    return;
  }

  event->accept();

  if (activeHandle_ == HandleType::Rotate) {
    applyRotation(event->scenePos());
  } else {
    applyResize(event->scenePos());
  }

  lastMousePos_ = event->scenePos();
  updateHandles();
}

void TransformHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (!isTransforming_) {
    event->ignore();
    return;
  }

  event->accept();

  // Create undo action if transform changed
  QGraphicsItem *target = resolveTargetItem();
  if (target && renderer_) {
    QTransform newTransform = target->transform();
    QPointF newPos = target->pos();

    if (newTransform != originalTransform_ || newPos != originalPos_) {
      if (itemStore_ && targetItemId_.isValid()) {
        renderer_->addAction(std::make_unique<TransformAction>(
            targetItemId_, itemStore_, originalTransform_, newTransform, originalPos_, newPos));
      }
    }
    
    // Restore the item's original flags
    target->setFlag(QGraphicsItem::ItemIsMovable, wasMovable_);
  }

  isTransforming_ = false;
  activeHandle_ = HandleType::None;

  emit transformCompleted();
}

void TransformHandleItem::applyResize(const QPointF &mousePos) {
  QGraphicsItem *target = resolveTargetItem();
  if (!target)
    return;

  QRectF currentBounds = targetBoundsInScene();
  QPointF delta = mousePos - lastMousePos_;

  // Calculate new bounds based on which handle is being dragged
  QRectF newBounds = currentBounds;

  switch (activeHandle_) {
  case HandleType::TopLeft:
    newBounds.setTopLeft(newBounds.topLeft() + delta);
    break;
  case HandleType::TopCenter:
    newBounds.setTop(newBounds.top() + delta.y());
    break;
  case HandleType::TopRight:
    newBounds.setTopRight(newBounds.topRight() + delta);
    break;
  case HandleType::MiddleLeft:
    newBounds.setLeft(newBounds.left() + delta.x());
    break;
  case HandleType::MiddleRight:
    newBounds.setRight(newBounds.right() + delta.x());
    break;
  case HandleType::BottomLeft:
    newBounds.setBottomLeft(newBounds.bottomLeft() + delta);
    break;
  case HandleType::BottomCenter:
    newBounds.setBottom(newBounds.bottom() + delta.y());
    break;
  case HandleType::BottomRight:
    newBounds.setBottomRight(newBounds.bottomRight() + delta);
    break;
  default:
    return;
  }

  // Ensure minimum size
  const qreal minSize = 10.0;
  if (newBounds.width() < minSize || newBounds.height() < minSize)
    return;

  // Calculate scale factors
  qreal scaleX = newBounds.width() / currentBounds.width();
  qreal scaleY = newBounds.height() / currentBounds.height();

  // Get the anchor point (opposite corner/edge) in scene coordinates
  QPointF anchor;
  switch (activeHandle_) {
  case HandleType::TopLeft:
    anchor = currentBounds.bottomRight();
    break;
  case HandleType::TopCenter:
    anchor = QPointF(currentBounds.center().x(), currentBounds.bottom());
    break;
  case HandleType::TopRight:
    anchor = currentBounds.bottomLeft();
    break;
  case HandleType::MiddleLeft:
    anchor = QPointF(currentBounds.right(), currentBounds.center().y());
    break;
  case HandleType::MiddleRight:
    anchor = QPointF(currentBounds.left(), currentBounds.center().y());
    break;
  case HandleType::BottomLeft:
    anchor = currentBounds.topRight();
    break;
  case HandleType::BottomCenter:
    anchor = QPointF(currentBounds.center().x(), currentBounds.top());
    break;
  case HandleType::BottomRight:
    anchor = currentBounds.topLeft();
    break;
  default:
    anchor = currentBounds.center();
    break;
  }

  // Handle text items specially - adjust font size instead of transform scaling
  if (LatexTextItem *textItem = dynamic_cast<LatexTextItem *>(target)) {
    // Use axis-aware scale so side handles feel as responsive as corner handles.
    qreal uniformScale = textScaleForHandle(activeHandle_, scaleX, scaleY);
    QFont currentFont = textItem->font();
    qreal currentSize = effectivePointSize(currentFont);
    qreal newSize = qBound(8.0, currentSize * uniformScale, 256.0);
    if (qAbs(newSize - currentSize) > 0.01) {
      currentFont.setPointSizeF(newSize);
      textItem->setFont(currentFont);
    }
    // Emit signal for other selected items
    emit resizeApplied(scaleX, scaleY, anchor);
    return;
  }

  // Apply scale transformation for non-text items
  QTransform t = target->transform();
  QPointF localAnchor = target->mapFromScene(anchor);

  // Create scaling transform around the anchor point
  QTransform scaleTransform;
  scaleTransform.translate(localAnchor.x(), localAnchor.y());
  scaleTransform.scale(scaleX, scaleY);
  scaleTransform.translate(-localAnchor.x(), -localAnchor.y());

  target->setTransform(t * scaleTransform);

  // Adjust position to keep anchor point fixed
  QPointF newAnchorPos = target->mapToScene(localAnchor);
  QPointF posAdjust = anchor - newAnchorPos;
  target->setPos(target->pos() + posAdjust);

  // Emit signal for other selected items to follow
  emit resizeApplied(scaleX, scaleY, anchor);
}

void TransformHandleItem::applyRotation(const QPointF &mousePos) {
  QGraphicsItem *target = resolveTargetItem();
  if (!target)
    return;

  // Calculate rotation center (center of original bounds)
  QPointF center = targetBoundsInScene().center();

  // Calculate angles
  QLineF lineToLast(center, lastMousePos_);
  QLineF lineToCurrent(center, mousePos);
  qreal angleDelta = lineToCurrent.angle() - lineToLast.angle();

  // Get the center in item coordinates
  QPointF localCenter = target->mapFromScene(center);

  // Create rotation transform around center
  // Note: Qt's coordinate system has y-axis pointing downward, which reverses
  // the direction of positive rotation compared to mathematical convention.
  // We negate angleDelta to get the expected rotation direction.
  QTransform rotateTransform;
  rotateTransform.translate(localCenter.x(), localCenter.y());
  rotateTransform.rotate(-angleDelta);
  rotateTransform.translate(-localCenter.x(), -localCenter.y());

  QTransform t = target->transform();
  target->setTransform(t * rotateTransform);

  // Adjust position to keep center fixed
  QPointF newCenterPos = target->mapToScene(localCenter);
  QPointF posAdjust = center - newCenterPos;
  target->setPos(target->pos() + posAdjust);

  // Emit signal for other selected items to follow
  emit rotationApplied(-angleDelta, center);
}
