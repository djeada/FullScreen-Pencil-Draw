/**
 * @file transform_handle_item.cpp
 * @brief Implementation of visual transform handles.
 */
#include "transform_handle_item.h"
#include "../core/scene_renderer.h"
#include "../core/transform_action.h"
#include <QCursor>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

TransformHandleItem::TransformHandleItem(QGraphicsItem *targetItem,
                                         SceneRenderer *renderer,
                                         QGraphicsItem *parent)
    : QGraphicsObject(parent), targetItem_(targetItem), renderer_(renderer),
      isTransforming_(false), activeHandle_(HandleType::None),
      wasMovable_(false), wasSelectable_(false), cachedTargetBounds_() {
  setAcceptHoverEvents(true);
  setFlag(QGraphicsItem::ItemIsSelectable, false);
  setFlag(QGraphicsItem::ItemIsMovable, false);
  // High Z value so handles appear above everything
  setZValue(10000);
  if (targetItem_) {
    targetItem_->installSceneEventFilter(this);
  }
  updateHandles();
}

TransformHandleItem::~TransformHandleItem() {
  if (targetItem_) {
    targetItem_->removeSceneEventFilter(this);
  }
}

QRectF TransformHandleItem::boundingRect() const {
  if (!targetItem_)
    return QRectF();

  QRectF bounds = targetBoundsInScene();
  
  // Helper to expand bounds to include handles and rotation handle
  auto expandForHandles = [](const QRectF &rect) {
    return rect.adjusted(-HANDLE_SIZE, -HANDLE_SIZE - ROTATION_HANDLE_OFFSET,
                         HANDLE_SIZE, HANDLE_SIZE);
  };
  
  QRectF expandedBounds = expandForHandles(bounds);
  
  // If target item has moved, return union of old and new bounds to ensure
  // the old anchor positions get repainted (cleared) along with the new positions.
  // This prevents the visual trail artifact when items are dragged.
  if (bounds != cachedTargetBounds_ && !cachedTargetBounds_.isEmpty()) {
    QRectF oldExpandedBounds = expandForHandles(cachedTargetBounds_);
    cachedTargetBounds_ = bounds;
    return expandedBounds.united(oldExpandedBounds);
  }
  
  cachedTargetBounds_ = bounds;
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
  if (!targetItem_)
    return QRectF();
  return targetItem_->mapToScene(targetItem_->boundingRect()).boundingRect();
}

void TransformHandleItem::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem * /*option*/,
                                QWidget * /*widget*/) {
  if (!targetItem_)
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
  prepareGeometryChange();
  cachedTargetBounds_ = targetBoundsInScene();
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
  if (!targetItem_)
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
  if (watched == targetItem_ && event->type() == QEvent::GraphicsSceneMove) {
    updateHandles();
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
  if (targetItem_) {
    originalTransform_ = targetItem_->transform();
    originalPos_ = targetItem_->pos();
    originalBounds_ = targetBoundsInScene();
    transformOrigin_ = originalBounds_.center();
    
    // Temporarily disable item's movable/selectable flags to prevent interference
    wasMovable_ = targetItem_->flags() & QGraphicsItem::ItemIsMovable;
    wasSelectable_ = targetItem_->flags() & QGraphicsItem::ItemIsSelectable;
    targetItem_->setFlag(QGraphicsItem::ItemIsMovable, false);
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
  if (targetItem_ && renderer_) {
    QTransform newTransform = targetItem_->transform();
    QPointF newPos = targetItem_->pos();

    if (newTransform != originalTransform_ || newPos != originalPos_) {
      renderer_->addAction(std::make_unique<TransformAction>(
          targetItem_, originalTransform_, newTransform, originalPos_, newPos));
    }
    
    // Restore the item's original flags
    targetItem_->setFlag(QGraphicsItem::ItemIsMovable, wasMovable_);
  }

  isTransforming_ = false;
  activeHandle_ = HandleType::None;

  emit transformCompleted();
}

void TransformHandleItem::applyResize(const QPointF &mousePos) {
  if (!targetItem_)
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

  // Apply scale transformation
  QTransform t = targetItem_->transform();
  QPointF localAnchor = targetItem_->mapFromScene(anchor);

  // Create scaling transform around the anchor point
  QTransform scaleTransform;
  scaleTransform.translate(localAnchor.x(), localAnchor.y());
  scaleTransform.scale(scaleX, scaleY);
  scaleTransform.translate(-localAnchor.x(), -localAnchor.y());

  targetItem_->setTransform(t * scaleTransform);

  // Adjust position to keep anchor point fixed
  QPointF newAnchorPos = targetItem_->mapToScene(localAnchor);
  QPointF posAdjust = anchor - newAnchorPos;
  targetItem_->setPos(targetItem_->pos() + posAdjust);
}

void TransformHandleItem::applyRotation(const QPointF &mousePos) {
  if (!targetItem_)
    return;

  // Calculate rotation center (center of original bounds)
  QPointF center = targetBoundsInScene().center();

  // Calculate angles
  QLineF lineToLast(center, lastMousePos_);
  QLineF lineToCurrent(center, mousePos);
  qreal angleDelta = lineToCurrent.angle() - lineToLast.angle();

  // Get the center in item coordinates
  QPointF localCenter = targetItem_->mapFromScene(center);

  // Create rotation transform around center
  // Note: Qt's coordinate system has y-axis pointing downward, which reverses
  // the direction of positive rotation compared to mathematical convention.
  // We negate angleDelta to get the expected rotation direction.
  QTransform rotateTransform;
  rotateTransform.translate(localCenter.x(), localCenter.y());
  rotateTransform.rotate(-angleDelta);
  rotateTransform.translate(-localCenter.x(), -localCenter.y());

  QTransform t = targetItem_->transform();
  targetItem_->setTransform(t * rotateTransform);

  // Adjust position to keep center fixed
  QPointF newCenterPos = targetItem_->mapToScene(localCenter);
  QPointF posAdjust = center - newCenterPos;
  targetItem_->setPos(targetItem_->pos() + posAdjust);
}
