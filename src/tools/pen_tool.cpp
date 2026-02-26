/**
 * @file pen_tool.cpp
 * @brief Freehand drawing tool implementation.
 */
#include "pen_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"
#include "../widgets/brush_stroke_item.h"

PenTool::PenTool(SceneRenderer *renderer)
    : Tool(renderer), currentPath_(nullptr), currentStroke_(nullptr),
      currentItem_(nullptr), currentItemId_() {}

PenTool::~PenTool() = default;

void PenTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  const BrushTip &tip = renderer_->currentBrushTip();
  bool useStroke = (tip.shape() != BrushTipShape::Round);

  if (useStroke) {
    // Custom brush tip: use BrushStrokeItem
    QPen pen = renderer_->currentPen();
    currentStroke_ = new BrushStrokeItem(
        tip, pen.widthF(), pen.color(),
        pen.color().alphaF());
    currentStroke_->setFlags(QGraphicsItem::ItemIsSelectable |
                             QGraphicsItem::ItemIsMovable);
    currentItem_ = currentStroke_;
    currentPath_ = nullptr;
  } else {
    // Default round tip: use existing QGraphicsPathItem path
    currentPath_ = new QGraphicsPathItem();
    currentPath_->setPen(renderer_->currentPen());
    currentPath_->setFlags(QGraphicsItem::ItemIsSelectable |
                           QGraphicsItem::ItemIsMovable);

    QPainterPath path;
    path.moveTo(scenePos);
    currentPath_->setPath(path);
    currentItem_ = currentPath_;
    currentStroke_ = nullptr;
  }

  // Use SceneController if available, otherwise fall back to direct scene
  // access
  SceneController *controller = renderer_->sceneController();
  if (controller) {
    currentItemId_ = controller->addItem(currentItem_);
  } else {
    renderer_->scene()->addItem(currentItem_);
    currentItemId_ = renderer_->registerItem(currentItem_);
  }

  pointBuffer_.clear();
  pointBuffer_.append(scenePos);

  if (useStroke) {
    currentStroke_->addPoint(scenePos);
  }

  renderer_->addDrawAction(currentItem_);
}

void PenTool::mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->buttons() & Qt::LeftButton) {
    if (currentStroke_) {
      currentStroke_->addPoint(scenePos);
    } else {
      addPoint(scenePos);
    }
  }
}

void PenTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                const QPointF & /*scenePos*/) {
  currentPath_ = nullptr;
  currentStroke_ = nullptr;
  currentItem_ = nullptr;
  currentItemId_ = ItemId();
  pointBuffer_.clear();
}

void PenTool::deactivate() {
  // Clean up any in-progress stroke
  if (currentItem_) {
    SceneController *controller = renderer_->sceneController();
    if (controller && currentItemId_.isValid()) {
      controller->removeItem(currentItemId_, false); // Don't keep for undo
    } else if (currentItem_->scene()) {
      renderer_->scene()->removeItem(currentItem_);
      delete currentItem_;
    }
    currentPath_ = nullptr;
    currentStroke_ = nullptr;
    currentItem_ = nullptr;
    currentItemId_ = ItemId();
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
