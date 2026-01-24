/**
 * @file pan_tool.cpp
 * @brief Pan/scroll tool implementation.
 */
#include "pan_tool.h"
#include "../core/scene_renderer.h"
#include <QScrollBar>

PanTool::PanTool(SceneRenderer *renderer) : Tool(renderer), isPanning_(false) {}

PanTool::~PanTool() = default;

QCursor PanTool::cursor() const {
  return isPanning_ ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
}

void PanTool::mousePressEvent(QMouseEvent *event, const QPointF & /*scenePos*/) {
  if (event->button() == Qt::LeftButton) {
    isPanning_ = true;
    lastPanPoint_ = event->pos();
    renderer_->setCursor(Qt::ClosedHandCursor);
  }
}

void PanTool::mouseMoveEvent(QMouseEvent *event, const QPointF & /*scenePos*/) {
  if (isPanning_) {
    QPoint delta = event->pos() - lastPanPoint_;
    lastPanPoint_ = event->pos();

    renderer_->horizontalScrollBar()->setValue(
        renderer_->horizontalScrollBar()->value() - delta.x());
    renderer_->verticalScrollBar()->setValue(
        renderer_->verticalScrollBar()->value() - delta.y());
  }
}

void PanTool::mouseReleaseEvent(QMouseEvent *event,
                                 const QPointF & /*scenePos*/) {
  if (event->button() == Qt::LeftButton) {
    isPanning_ = false;
    renderer_->setCursor(Qt::OpenHandCursor);
  }
}
