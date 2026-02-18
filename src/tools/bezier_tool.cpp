/**
 * @file bezier_tool.cpp
 * @brief Bezier path drawing tool implementation.
 */
#include "bezier_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"

BezierTool::BezierTool(SceneRenderer *renderer)
    : Tool(renderer), currentPath_(nullptr), currentPathId_(),
      previewSegment_(nullptr), isDragging_(false) {}

BezierTool::~BezierTool() { clearPreviewItems(); }

void BezierTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  // Double-click finishes the path
  if (event->type() == QEvent::MouseButtonDblClick) {
    finalizePath();
    return;
  }

  isDragging_ = true;
  dragStart_ = scenePos;

  // Add a new anchor at the click position (handle set on release)
  AnchorPoint anchor;
  anchor.position = scenePos;
  anchor.handleOut = scenePos;
  anchor.hasHandle = false;
  anchors_.append(anchor);

  // Create the path item on the first anchor
  if (anchors_.size() == 1) {
    currentPath_ = new QGraphicsPathItem();
    currentPath_->setPen(renderer_->currentPen());
    currentPath_->setFlags(QGraphicsItem::ItemIsSelectable |
                           QGraphicsItem::ItemIsMovable);

    SceneController *controller = renderer_->sceneController();
    if (controller) {
      currentPathId_ = controller->addItem(currentPath_);
    } else {
      renderer_->scene()->addItem(currentPath_);
      currentPathId_ = renderer_->registerItem(currentPath_);
    }
  }

  // Add a visual marker for the anchor point
  auto *marker = new QGraphicsEllipseItem(
      scenePos.x() - ANCHOR_MARKER_SIZE / 2,
      scenePos.y() - ANCHOR_MARKER_SIZE / 2, ANCHOR_MARKER_SIZE,
      ANCHOR_MARKER_SIZE);
  QPen markerPen(renderer_->currentPen().color());
  markerPen.setWidth(1);
  marker->setPen(markerPen);
  marker->setBrush(Qt::white);
  marker->setZValue(1000);
  renderer_->scene()->addItem(marker);
  anchorMarkers_.append(marker);

  rebuildPath();
}

void BezierTool::mouseMoveEvent(QMouseEvent * /*event*/,
                                const QPointF &scenePos) {
  if (isDragging_ && !anchors_.isEmpty()) {
    // While dragging, update the outgoing handle of the current anchor
    AnchorPoint &current = anchors_.last();
    current.handleOut = scenePos;
    current.hasHandle = true;
    rebuildPath();
  } else if (!anchors_.isEmpty() && currentPath_) {
    // Not dragging — show a preview segment from the last anchor to the mouse
    updatePreview(scenePos);
  }
}

void BezierTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                   const QPointF &scenePos) {
  if (isDragging_ && !anchors_.isEmpty()) {
    AnchorPoint &current = anchors_.last();
    if (current.position != scenePos) {
      current.handleOut = scenePos;
      current.hasHandle = true;
    }
    isDragging_ = false;
    rebuildPath();
  }
}

void BezierTool::deactivate() {
  finalizePath();
  Tool::deactivate();
}

void BezierTool::finalizePath() {
  clearPreviewItems();

  if (currentPath_ && anchors_.size() >= 2) {
    rebuildPath();
    renderer_->addDrawAction(currentPath_);
  } else if (currentPath_) {
    // Not enough points — remove the path
    SceneController *controller = renderer_->sceneController();
    if (controller && currentPathId_.isValid()) {
      controller->removeItem(currentPathId_, false);
    } else if (currentPath_->scene()) {
      renderer_->scene()->removeItem(currentPath_);
      delete currentPath_;
    }
  }

  currentPath_ = nullptr;
  currentPathId_ = ItemId();
  anchors_.clear();
  isDragging_ = false;
}

void BezierTool::updatePreview(const QPointF &mousePos) {
  if (anchors_.isEmpty())
    return;

  const AnchorPoint &last = anchors_.last();
  QPainterPath preview;
  preview.moveTo(last.position);

  if (last.hasHandle) {
    // Reflect handle across anchor point for C1 continuity
    QPointF mirroredHandle =
        2.0 * last.position - last.handleOut;
    preview.cubicTo(mirroredHandle, mousePos, mousePos);
  } else {
    preview.lineTo(mousePos);
  }

  if (!previewSegment_) {
    previewSegment_ = new QGraphicsPathItem();
    QPen previewPen = renderer_->currentPen();
    previewPen.setStyle(Qt::DashLine);
    previewSegment_->setPen(previewPen);
    previewSegment_->setZValue(999);
    renderer_->scene()->addItem(previewSegment_);
  }
  previewSegment_->setPath(preview);
}

void BezierTool::rebuildPath() {
  if (!currentPath_ || anchors_.isEmpty())
    return;

  QPainterPath path;
  path.moveTo(anchors_.first().position);

  for (int i = 1; i < anchors_.size(); ++i) {
    const AnchorPoint &prev = anchors_[i - 1];
    const AnchorPoint &curr = anchors_[i];

    QPointF cp1, cp2;

    // Outgoing control point from previous anchor
    if (prev.hasHandle) {
      cp1 = prev.handleOut;
    } else {
      cp1 = prev.position;
    }

    // Incoming control point: reflect outgoing handle for C1 continuity
    if (curr.hasHandle) {
      cp2 = 2.0 * curr.position - curr.handleOut;
    } else {
      cp2 = curr.position;
    }

    path.cubicTo(cp1, cp2, curr.position);
  }

  currentPath_->setPath(path);
}

void BezierTool::clearPreviewItems() {
  if (previewSegment_) {
    if (previewSegment_->scene()) {
      renderer_->scene()->removeItem(previewSegment_);
    }
    delete previewSegment_;
    previewSegment_ = nullptr;
  }

  for (auto *marker : anchorMarkers_) {
    if (marker->scene()) {
      renderer_->scene()->removeItem(marker);
    }
    delete marker;
  }
  anchorMarkers_.clear();
}
