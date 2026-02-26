/**
 * @file lasso_selection_tool.cpp
 * @brief Lasso (polygon) selection tool implementation.
 */
#include "lasso_selection_tool.h"
#include "../core/scene_renderer.h"

LassoSelectionTool::LassoSelectionTool(SceneRenderer *renderer)
    : Tool(renderer), lassoPath_(nullptr), drawing_(false) {}

LassoSelectionTool::~LassoSelectionTool() = default;

void LassoSelectionTool::mousePressEvent(QMouseEvent *event,
                                         const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  QGraphicsScene *s = renderer_->scene();
  if (!s)
    return;

  // Clear previous selection
  s->clearSelection();

  // Start collecting points
  points_.clear();
  points_.append(scenePos);
  drawing_ = true;

  // Create visual feedback path
  lassoPath_ = new QGraphicsPathItem();
  QPen dashPen(Qt::DashLine);
  dashPen.setColor(QColor(kLassoColorR, kLassoColorG, kLassoColorB));
  dashPen.setWidth(1);
  dashPen.setCosmetic(true);
  lassoPath_->setPen(dashPen);
  lassoPath_->setBrush(
      QBrush(QColor(kLassoColorR, kLassoColorG, kLassoColorB, kLassoFillAlpha)));
  lassoPath_->setZValue(1e9); // Always on top
  lassoPath_->setFlag(QGraphicsItem::ItemIsSelectable, false);
  lassoPath_->setFlag(QGraphicsItem::ItemIsMovable, false);

  QPainterPath path;
  path.moveTo(scenePos);
  lassoPath_->setPath(path);
  s->addItem(lassoPath_);
}

void LassoSelectionTool::mouseMoveEvent(QMouseEvent *event,
                                        const QPointF &scenePos) {
  if (!drawing_ || !(event->buttons() & Qt::LeftButton) || !lassoPath_)
    return;

  points_.append(scenePos);

  QPainterPath path;
  path.moveTo(points_.first());
  for (int i = 1; i < points_.size(); ++i) {
    path.lineTo(points_.at(i));
  }
  // Show a closing line back to start for visual feedback
  path.lineTo(points_.first());
  lassoPath_->setPath(path);
}

void LassoSelectionTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                           const QPointF & /*scenePos*/) {
  if (!drawing_)
    return;
  drawing_ = false;

  QGraphicsScene *s = renderer_->scene();
  if (!s) {
    removeLassoPath();
    return;
  }

  // Build closed polygon path for hit-testing
  QPainterPath selectionPath;
  if (points_.size() >= 3) {
    selectionPath.moveTo(points_.first());
    for (int i = 1; i < points_.size(); ++i) {
      selectionPath.lineTo(points_.at(i));
    }
    selectionPath.closeSubpath();
  }

  // Remove the visual lasso overlay before selecting
  removeLassoPath();

  // Select items that intersect or are contained by the lasso polygon
  if (!selectionPath.isEmpty()) {
    const QList<QGraphicsItem *> allItems = s->items();
    for (QGraphicsItem *item : allItems) {
      if (!item)
        continue;
      if (!(item->flags() & QGraphicsItem::ItemIsSelectable))
        continue;
      // Map the selection path to the item's coordinate system
      QPainterPath mappedPath = item->mapFromScene(selectionPath);
      if (item->shape().intersects(mappedPath)) {
        item->setSelected(true);
      }
    }
  }

  points_.clear();
}

void LassoSelectionTool::deactivate() {
  removeLassoPath();
  points_.clear();
  drawing_ = false;
  Tool::deactivate();
}

void LassoSelectionTool::removeLassoPath() {
  if (lassoPath_) {
    QGraphicsScene *s = renderer_->scene();
    if (s && lassoPath_->scene() == s) {
      s->removeItem(lassoPath_);
    }
    delete lassoPath_;
    lassoPath_ = nullptr;
  }
}
