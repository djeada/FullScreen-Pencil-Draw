/**
 * @file element_factory.h
 * @brief Creates architecture/electronics diagram elements by their bank id.
 *
 * Shared by the canvas (placing elements from the element bank) and the
 * project serializer (recreating saved elements), so both agree on ids.
 */
#ifndef ELEMENT_FACTORY_H
#define ELEMENT_FACTORY_H

#include <QString>

/// MIME type of an element dragged from the element library onto a canvas;
/// the payload is the element id (UTF-8).
inline constexpr char kElementMimeType[] = "application/x-fspd-element";

class QGraphicsItem;

/**
 * @brief Create the diagram element registered under @p elementId.
 * @return A new, unparented item tagged with its id, or nullptr if the id
 *         is unknown.
 */
QGraphicsItem *createDiagramElement(const QString &elementId);

/**
 * @brief The bank id an element was created with, or an empty string for
 *        items that did not come from createDiagramElement().
 */
QString diagramElementId(const QGraphicsItem *item);

#endif // ELEMENT_FACTORY_H
