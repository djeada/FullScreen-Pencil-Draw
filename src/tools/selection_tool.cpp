// selection_tool.cpp
#include "selection_tool.h"
#include "../widgets/canvas.h"

SelectionTool::SelectionTool(Canvas *canvas) : Tool(canvas) {}

SelectionTool::~SelectionTool() = default;

void SelectionTool::mousePressEvent(QMouseEvent * /*event*/,
                                     const QPointF & /*scenePos*/) {
  // Let QGraphicsView handle the selection
}

void SelectionTool::mouseMoveEvent(QMouseEvent * /*event*/,
                                    const QPointF & /*scenePos*/) {
  // Let QGraphicsView handle the move
}

void SelectionTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                       const QPointF & /*scenePos*/) {
  // Let QGraphicsView handle the release
}
