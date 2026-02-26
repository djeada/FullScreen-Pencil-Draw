/**
 * @file fill_utils.h
 * @brief Shared fill operations for canvas and tool-based renderers.
 */
#ifndef FILL_UTILS_H
#define FILL_UTILS_H

#include <QBrush>
#include <QColor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <functional>
#include <memory>

class Action;
class ItemStore;

/**
 * @brief Fill the first supported item at a scene point.
 *
 * Iterates top-most to bottom-most items under @p point and applies fill to the
 * first supported target. Grouped items are treated as one target so arrows and
 * grouped content are updated consistently.
 *
 * For shape items the full brush (solid, gradient, or pattern) is applied.
 * For non-shape items (text, lines, pixmaps) the brush's color() is used.
 *
 * @param scene Scene to query.
 * @param point Scene point to fill at.
 * @param brush Brush to apply (may contain solid color, gradient or pattern).
 * @param store Optional item store for undo/redo action creation.
 * @param backgroundItem Optional item to ignore as immutable background.
 * @param extraSkipItem Optional extra item to ignore (for UI helpers).
 * @param pushAction Callback used to push an undo action when available.
 * @return true when an item was updated, otherwise false.
 */
bool fillTopItemAtPoint(
    QGraphicsScene *scene, const QPointF &point, const QBrush &brush,
    ItemStore *store, QGraphicsItem *backgroundItem,
    QGraphicsItem *extraSkipItem,
    const std::function<void(std::unique_ptr<Action>)> &pushAction);

#endif // FILL_UTILS_H
