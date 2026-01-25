/**
 * @file shape_tool.cpp
 * @brief Abstract base class for shape drawing tools implementation.
 */
#include "shape_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"

ShapeTool::ShapeTool(SceneRenderer *renderer)
    : Tool(renderer), tempShape_(nullptr), tempShapeId_() {}

ShapeTool::~ShapeTool() = default;

void ShapeTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  startPoint_ = scenePos;
  tempShape_ = createShape(scenePos);

  if (tempShape_) {
    tempShape_->setFlags(QGraphicsItem::ItemIsSelectable |
                         QGraphicsItem::ItemIsMovable);
    
    // Use SceneController if available, otherwise fall back to direct scene access
    SceneController *controller = renderer_->sceneController();
    if (controller) {
      tempShapeId_ = controller->addItem(tempShape_);
    } else {
      renderer_->scene()->addItem(tempShape_);
      tempShapeId_ = renderer_->registerItem(tempShape_);
    }
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
    tempShapeId_ = ItemId();
  }
}

void ShapeTool::finalizeShape(const QPointF & /*startPos*/,
                              const QPointF & /*endPos*/) {
  // Default implementation does nothing
  // Subclasses can override for special finalization
}

void ShapeTool::deactivate() {
  // Clean up any in-progress shape
  if (tempShape_) {
    SceneController *controller = renderer_->sceneController();
    if (controller && tempShapeId_.isValid()) {
      controller->removeItem(tempShapeId_, false);  // Don't keep for undo
    } else if (tempShape_->scene()) {
      renderer_->scene()->removeItem(tempShape_);
      delete tempShape_;
    }
    tempShape_ = nullptr;
    tempShapeId_ = ItemId();
  }
  Tool::deactivate();
}

QGraphicsItem *ShapeTool::tempShape() const {
  // If we have an ItemStore, resolve from there for safety
  ItemStore *store = renderer_->itemStore();
  if (store && tempShapeId_.isValid()) {
    return store->item(tempShapeId_);
  }
  return tempShape_;
}
