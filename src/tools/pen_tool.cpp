/**
 * @file pen_tool.cpp
 * @brief Freehand drawing tool implementation.
 */
#include "pen_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"

PenTool::PenTool(SceneRenderer *renderer)
    : Tool(renderer), currentPath_(nullptr), currentPathId_() {}

PenTool::~PenTool() = default;

void PenTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  currentPath_ = new QGraphicsPathItem();
  currentPath_->setPen(renderer_->currentPen());
  currentPath_->setFlags(QGraphicsItem::ItemIsSelectable |
                         QGraphicsItem::ItemIsMovable);

  QPainterPath path;
  path.moveTo(scenePos);
  currentPath_->setPath(path);

  // Use SceneController if available, otherwise fall back to direct scene access
  SceneController *controller = renderer_->sceneController();
  if (controller) {
    currentPathId_ = controller->addItem(currentPath_);
  } else {
    renderer_->scene()->addItem(currentPath_);
    currentPathId_ = renderer_->registerItem(currentPath_);
  }

  pointBuffer_.clear();
  pointBuffer_.append(scenePos);

  renderer_->addDrawAction(currentPath_);
}

void PenTool::mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->buttons() & Qt::LeftButton) {
    addPoint(scenePos);
  }
}

void PenTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                 const QPointF & /*scenePos*/) {
  currentPath_ = nullptr;
  currentPathId_ = ItemId();
  pointBuffer_.clear();
}

void PenTool::deactivate() {
  // Clean up any in-progress path
  if (currentPath_) {
    SceneController *controller = renderer_->sceneController();
    if (controller && currentPathId_.isValid()) {
      controller->removeItem(currentPathId_, false);  // Don't keep for undo
    } else if (currentPath_->scene()) {
      renderer_->scene()->removeItem(currentPath_);
      delete currentPath_;
    }
    currentPath_ = nullptr;
    currentPathId_ = ItemId();
  }
  pointBuffer_.clear();
  Tool::deactivate();
}

void PenTool::addPoint(const QPointF &point) {
  if (!currentPath_)
    return;

  pointBuffer_.append(point);

  if (pointBuffer_.size() >= MIN_POINTS_FOR_SPLINE) {
    // Use Catmull-Rom spline interpolation for smooth curves
    QPointF p0 = pointBuffer_.at(pointBuffer_.size() - MIN_POINTS_FOR_SPLINE);
    QPointF p1 =
        pointBuffer_.at(pointBuffer_.size() - MIN_POINTS_FOR_SPLINE + 1);
    QPointF p2 =
        pointBuffer_.at(pointBuffer_.size() - MIN_POINTS_FOR_SPLINE + 2);
    QPointF p3 =
        pointBuffer_.at(pointBuffer_.size() - MIN_POINTS_FOR_SPLINE + 3);

    QPainterPath path = currentPath_->path();
    path.cubicTo(p1 + (p2 - p0) / 6.0, p2 - (p3 - p1) / 6.0, p2);
    currentPath_->setPath(path);
  }

  if (pointBuffer_.size() > MIN_POINTS_FOR_SPLINE) {
    pointBuffer_.removeFirst();
  }
}
