/**
 * @file shape_tool.cpp
 * @brief Abstract base class for shape drawing tools implementation.
 */
#include "shape_tool.h"
#include "../core/scene_renderer.h"

ShapeTool::ShapeTool(SceneRenderer *renderer)
    : Tool(renderer), tempShape_(nullptr) {}

ShapeTool::~ShapeTool() = default;

void ShapeTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  startPoint_ = scenePos;
  tempShape_ = createShape(scenePos);

  if (tempShape_) {
    tempShape_->setFlags(QGraphicsItem::ItemIsSelectable |
                         QGraphicsItem::ItemIsMovable);
    renderer_->scene()->addItem(tempShape_);
  }
}

void ShapeTool::mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) {
  if ((event->buttons() & Qt::LeftButton) && tempShape_) {
    updateShape(startPoint_, scenePos);
  }
}

void ShapeTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                  const QPointF &scenePos) {
  if (tempShape_) {
    finalizeShape(startPoint_, scenePos);
    if (tempShape_) {
      renderer_->addDrawAction(tempShape_);
    }
    tempShape_ = nullptr;
  }
}

void ShapeTool::finalizeShape(const QPointF & /*startPos*/,
                              const QPointF & /*endPos*/) {
  // Default implementation does nothing
  // Subclasses can override for special finalization
}
