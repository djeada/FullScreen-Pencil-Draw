/**
 * @file snap_engine.cpp
 * @brief Implementation of the SnapEngine class.
 */
#include "snap_engine.h"

SnapEngine::SnapEngine(int gridSize, qreal snapThreshold)
    : gridSize_(gridSize), snapThreshold_(snapThreshold) {}

void SnapEngine::setSnapToGridEnabled(bool enabled) { snapToGrid_ = enabled; }
void SnapEngine::setSnapToObjectEnabled(bool enabled) {
  snapToObject_ = enabled;
}
void SnapEngine::setGridSize(int size) { gridSize_ = size; }
void SnapEngine::setSnapThreshold(qreal threshold) {
  snapThreshold_ = threshold;
}

bool SnapEngine::isSnapToGridEnabled() const { return snapToGrid_; }
bool SnapEngine::isSnapToObjectEnabled() const { return snapToObject_; }
int SnapEngine::gridSize() const { return gridSize_; }
qreal SnapEngine::snapThreshold() const { return snapThreshold_; }

SnapResult SnapEngine::snapToGrid(const QPointF &point) const {
  SnapResult result;
  result.snappedPoint = point;

  if (!snapToGrid_ || gridSize_ <= 0)
    return result;

  qreal snappedX =
      std::round(point.x() / gridSize_) * static_cast<qreal>(gridSize_);
  qreal snappedY =
      std::round(point.y() / gridSize_) * static_cast<qreal>(gridSize_);

  if (std::abs(snappedX - point.x()) <= snapThreshold_) {
    result.snappedPoint.setX(snappedX);
    result.snappedX = true;
    result.guideX = snappedX;
  }
  if (std::abs(snappedY - point.y()) <= snapThreshold_) {
    result.snappedPoint.setY(snappedY);
    result.snappedY = true;
    result.guideY = snappedY;
  }
  return result;
}

void SnapEngine::collectObjectTargets(
    const QList<QGraphicsItem *> &sceneItems,
    const QSet<QGraphicsItem *> &excludeItems, QList<qreal> &xTargets,
    QList<qreal> &yTargets) const {
  for (QGraphicsItem *item : sceneItems) {
    if (!item || excludeItems.contains(item))
      continue;
    if (!item->isVisible())
      continue;
    // Skip items that are children of other items (they're part of groups)
    if (item->parentItem())
      continue;

    QRectF br = item->sceneBoundingRect();
    if (br.isEmpty())
      continue;

    xTargets.append(br.left());
    xTargets.append(br.right());
    xTargets.append(br.center().x());

    yTargets.append(br.top());
    yTargets.append(br.bottom());
    yTargets.append(br.center().y());
  }
}

SnapResult
SnapEngine::snap(const QPointF &point,
                 const QList<QGraphicsItem *> &sceneItems,
                 const QSet<QGraphicsItem *> &excludeItems) const {
  SnapResult result;
  result.snappedPoint = point;

  if (!snapToGrid_ && !snapToObject_)
    return result;

  qreal bestDx = std::numeric_limits<qreal>::max();
  qreal bestDy = std::numeric_limits<qreal>::max();
  qreal bestX = point.x();
  qreal bestY = point.y();

  // Grid candidates
  if (snapToGrid_ && gridSize_ > 0) {
    qreal gx =
        std::round(point.x() / gridSize_) * static_cast<qreal>(gridSize_);
    qreal gy =
        std::round(point.y() / gridSize_) * static_cast<qreal>(gridSize_);
    qreal dx = std::abs(gx - point.x());
    qreal dy = std::abs(gy - point.y());
    if (dx <= snapThreshold_ && dx < bestDx) {
      bestDx = dx;
      bestX = gx;
    }
    if (dy <= snapThreshold_ && dy < bestDy) {
      bestDy = dy;
      bestY = gy;
    }
  }

  // Object candidates
  if (snapToObject_) {
    QList<qreal> xTargets, yTargets;
    collectObjectTargets(sceneItems, excludeItems, xTargets, yTargets);

    for (qreal tx : xTargets) {
      qreal dx = std::abs(tx - point.x());
      if (dx <= snapThreshold_ && dx < bestDx) {
        bestDx = dx;
        bestX = tx;
      }
    }
    for (qreal ty : yTargets) {
      qreal dy = std::abs(ty - point.y());
      if (dy <= snapThreshold_ && dy < bestDy) {
        bestDy = dy;
        bestY = ty;
      }
    }
  }

  if (bestDx < std::numeric_limits<qreal>::max()) {
    result.snappedPoint.setX(bestX);
    result.snappedX = true;
    result.guideX = bestX;
  }
  if (bestDy < std::numeric_limits<qreal>::max()) {
    result.snappedPoint.setY(bestY);
    result.snappedY = true;
    result.guideY = bestY;
  }

  return result;
}
