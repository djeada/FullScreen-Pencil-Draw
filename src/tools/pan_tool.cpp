/**
 * @file pan_tool.cpp
 * @brief Pan/scroll tool implementation.
 */
#include "pan_tool.h"
#include "../widgets/canvas.h"
#include <QScrollBar>

PanTool::PanTool(Canvas *canvas) : Tool(canvas), isPanning_(false) {}

PanTool::~PanTool() = default;

QCursor PanTool::cursor() const {
  return isPanning_ ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
}

void PanTool::mousePressEvent(QMouseEvent *event, const QPointF & /*scenePos*/) {
  if (event->button() == Qt::LeftButton) {
    isPanning_ = true;
    lastPanPoint_ = event->pos();
    canvas_->setCursor(Qt::ClosedHandCursor);
  }
}

void PanTool::mouseMoveEvent(QMouseEvent *event, const QPointF & /*scenePos*/) {
  if (isPanning_) {
    QPoint delta = event->pos() - lastPanPoint_;
    lastPanPoint_ = event->pos();

    canvas_->horizontalScrollBar()->setValue(
        canvas_->horizontalScrollBar()->value() - delta.x());
    canvas_->verticalScrollBar()->setValue(
        canvas_->verticalScrollBar()->value() - delta.y());
  }
}

void PanTool::mouseReleaseEvent(QMouseEvent *event,
                                 const QPointF & /*scenePos*/) {
  if (event->button() == Qt::LeftButton) {
    isPanning_ = false;
    canvas_->setCursor(Qt::OpenHandCursor);
  }
}
