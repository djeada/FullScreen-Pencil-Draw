/**
 * @file snap_engine.h
 * @brief Snap engine for snap-to-grid and snap-to-object functionality.
 *
 * SnapEngine provides a unified snapping system that can snap points to
 * grid intersections and/or nearby object edges, centers, and corners.
 */
#ifndef SNAP_ENGINE_H
#define SNAP_ENGINE_H

#include <QGraphicsItem>
#include <QList>
#include <QPointF>
#include <QSet>
#include <cmath>
#include <limits>

/**
 * @brief Result of a snap operation.
 *
 * Contains the snapped point and metadata about which axes were snapped,
 * along with guide line positions for visual feedback.
 */
struct SnapResult {
  QPointF snappedPoint;
  bool snappedX = false;
  bool snappedY = false;
  qreal guideX = 0;
  qreal guideY = 0;
};

/**
 * @brief Engine that performs snap-to-grid and snap-to-object calculations.
 *
 * The snap engine examines a point and finds the nearest snap target on
 * each axis independently. Grid snapping rounds to the nearest grid
 * intersection. Object snapping looks at bounding-box edges and centers
 * of other items in the scene.
 */
class SnapEngine {
public:
  /**
   * @brief Construct a new SnapEngine
   * @param gridSize Grid spacing in scene units
   * @param snapThreshold Maximum distance to trigger a snap
   */
  explicit SnapEngine(int gridSize = 20, qreal snapThreshold = 10.0);

  void setSnapToGridEnabled(bool enabled);
  void setSnapToObjectEnabled(bool enabled);
  void setGridSize(int size);
  void setSnapThreshold(qreal threshold);

  bool isSnapToGridEnabled() const;
  bool isSnapToObjectEnabled() const;
  int gridSize() const;
  qreal snapThreshold() const;

  /**
   * @brief Snap a point considering both grid and object targets.
   * @param point The input point in scene coordinates
   * @param sceneItems All items in the scene
   * @param excludeItems Items to exclude from object snapping (e.g. the item
   *        being drawn/moved)
   * @return SnapResult with the snapped point and guide info
   */
  SnapResult snap(const QPointF &point,
                  const QList<QGraphicsItem *> &sceneItems,
                  const QSet<QGraphicsItem *> &excludeItems = {}) const;

  /**
   * @brief Snap a point to the grid only.
   * @param point The input point
   * @return SnapResult with grid-snapped coordinates
   */
  SnapResult snapToGrid(const QPointF &point) const;

private:
  int gridSize_;
  qreal snapThreshold_;
  bool snapToGrid_ = false;
  bool snapToObject_ = false;

  /**
   * @brief Collect snap target coordinates from scene items.
   *
   * For each item not in the exclude set, collects the left, right, center-x,
   * top, bottom, and center-y of its scene bounding rect.
   */
  void collectObjectTargets(const QList<QGraphicsItem *> &sceneItems,
                            const QSet<QGraphicsItem *> &excludeItems,
                            QList<qreal> &xTargets,
                            QList<qreal> &yTargets) const;
};

#endif // SNAP_ENGINE_H
