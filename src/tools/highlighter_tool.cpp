/**
 * @file highlighter_tool.cpp
 * @brief Semi-transparent freehand highlighter tool implementation.
 */
#include "highlighter_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"

namespace {
constexpr int kHighlighterAlpha = 96;
constexpr qreal kMinimumHighlighterWidth = 12.0;
constexpr qreal kHighlighterWidthScale = 3.0;
} // namespace

HighlighterTool::HighlighterTool(SceneRenderer *renderer)
    : Tool(renderer), currentPath_(nullptr), currentItemId_() {}

HighlighterTool::~HighlighterTool() = default;

void HighlighterTool::mousePressEvent(QMouseEvent *event,
                                      const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton)) {
    return;
  }

  currentPath_ = new QGraphicsPathItem();
  currentPath_->setPen(highlighterPen());
  currentPath_->setFlags(QGraphicsItem::ItemIsSelectable |
                         QGraphicsItem::ItemIsMovable);

  QPainterPath path;
  path.moveTo(scenePos);
  currentPath_->setPath(path);

  if (SceneController *controller = renderer_->sceneController()) {
    currentItemId_ = controller->addItem(currentPath_);
  } else {
    renderer_->scene()->addItem(currentPath_);
    currentItemId_ = renderer_->registerItem(currentPath_);
  }

  pointBuffer_.clear();
  pointBuffer_.append(scenePos);

  renderer_->addDrawAction(currentPath_);
}

void HighlighterTool::mouseMoveEvent(QMouseEvent *event,
                                     const QPointF &scenePos) {
  if ((event->buttons() & Qt::LeftButton) && currentPath_) {
    addPoint(scenePos);
  }
}

void HighlighterTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                        const QPointF & /*scenePos*/) {
  currentPath_ = nullptr;
  currentItemId_ = ItemId();
  pointBuffer_.clear();
}

void HighlighterTool::deactivate() {
  if (currentPath_) {
    if (SceneController *controller = renderer_->sceneController();
        controller && currentItemId_.isValid()) {
      controller->removeItem(currentItemId_, false);
    } else if (currentPath_->scene()) {
      renderer_->scene()->removeItem(currentPath_);
      delete currentPath_;
    }
    currentPath_ = nullptr;
    currentItemId_ = ItemId();
  }

  pointBuffer_.clear();
  Tool::deactivate();
}

void HighlighterTool::addPoint(const QPointF &point) {
  if (!currentPath_) {
    return;
  }

  pointBuffer_.append(point);

  if (pointBuffer_.size() >= MIN_POINTS_FOR_SPLINE) {
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

QPen HighlighterTool::highlighterPen() const {
  QPen pen = renderer_->currentPen();
  QColor color = pen.color();
  color.setAlpha(qMin(color.alpha(), kHighlighterAlpha));
  pen.setColor(color);
  pen.setWidthF(
      qMax(kMinimumHighlighterWidth, pen.widthF() * kHighlighterWidthScale));
  pen.setCapStyle(Qt::RoundCap);
  pen.setJoinStyle(Qt::RoundJoin);
  return pen;
}
