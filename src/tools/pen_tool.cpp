// pen_tool.cpp
#include "pen_tool.h"
#include "../widgets/canvas.h"

PenTool::PenTool(Canvas *canvas)
    : Tool(canvas), currentPath_(nullptr) {}

PenTool::~PenTool() = default;

void PenTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  currentPath_ = new QGraphicsPathItem();
  currentPath_->setPen(canvas_->currentPen());
  currentPath_->setFlags(QGraphicsItem::ItemIsSelectable |
                         QGraphicsItem::ItemIsMovable);

  QPainterPath path;
  path.moveTo(scenePos);
  currentPath_->setPath(path);

  canvas_->scene()->addItem(currentPath_);

  pointBuffer_.clear();
  pointBuffer_.append(scenePos);

  canvas_->addDrawAction(currentPath_);
}

void PenTool::mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->buttons() & Qt::LeftButton) {
    addPoint(scenePos);
  }
}

void PenTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                 const QPointF & /*scenePos*/) {
  currentPath_ = nullptr;
  pointBuffer_.clear();
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
