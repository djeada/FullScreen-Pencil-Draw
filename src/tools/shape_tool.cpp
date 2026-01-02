// shape_tool.cpp
#include "shape_tool.h"
#include "../widgets/canvas.h"

ShapeTool::ShapeTool(Canvas *canvas)
    : Tool(canvas), tempShape_(nullptr) {}

ShapeTool::~ShapeTool() = default;

void ShapeTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  startPoint_ = scenePos;
  tempShape_ = createShape(scenePos);

  if (tempShape_) {
    tempShape_->setFlags(QGraphicsItem::ItemIsSelectable |
                         QGraphicsItem::ItemIsMovable);
    canvas_->scene()->addItem(tempShape_);
    canvas_->addDrawAction(tempShape_);
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
    tempShape_ = nullptr;
  }
}

void ShapeTool::finalizeShape(const QPointF & /*startPos*/,
                              const QPointF & /*endPos*/) {
  // Default implementation does nothing
  // Subclasses can override for special finalization
}
