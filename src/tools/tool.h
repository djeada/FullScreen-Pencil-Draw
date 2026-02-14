/**
 * @file tool.h
 * @brief Abstract base class for all drawing tools.
 */
#ifndef TOOL_H
#define TOOL_H

#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPen>
#include <QPointF>
#include <QString>

#include "../core/item_id.h"

class SceneRenderer;
class ItemStore;

/**
 * @brief Abstract base class for all drawing tools.
 *
 * This class provides a common interface for all tools in the application.
 * Each tool implements mouse event handlers and provides its own cursor.
 * The tool system allows for easy extension with new tools.
 *
 * Tools work with a SceneRenderer interface, which is implemented by both
 * Canvas and PdfViewer, allowing the same tools to work with both.
 */
class Tool {
public:
  /**
   * @brief Construct a new Tool object
   * @param renderer Pointer to the scene renderer this tool operates on
   */
  explicit Tool(SceneRenderer *renderer);
  virtual ~Tool();

  /**
   * @brief Get the name of this tool
   * @return QString The tool name for display purposes
   */
  virtual QString name() const = 0;

  /**
   * @brief Get the cursor to use when this tool is active
   * @return QCursor The appropriate cursor for this tool
   */
  virtual QCursor cursor() const = 0;

  /**
   * @brief Called when the tool becomes active
   */
  virtual void activate();

  /**
   * @brief Called when the tool becomes inactive
   */
  virtual void deactivate();

  /**
   * @brief Handle mouse press event
   * @param event The mouse event
   * @param scenePos The position in scene coordinates
   */
  virtual void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) = 0;

  /**
   * @brief Handle mouse move event
   * @param event The mouse event
   * @param scenePos The position in scene coordinates
   */
  virtual void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) = 0;

  /**
   * @brief Handle mouse release event
   * @param event The mouse event
   * @param scenePos The position in scene coordinates
   */
  virtual void mouseReleaseEvent(QMouseEvent *event,
                                 const QPointF &scenePos) = 0;

  /**
   * @brief Check if this tool uses rubber band selection
   * @return true if the tool uses rubber band selection
   */
  virtual bool usesRubberBandSelection() const { return false; }

  /**
   * @brief Check if this tool makes items selectable/movable
   * @return true if items should be selectable/movable
   */
  virtual bool itemsSelectable() const { return true; }

protected:
  SceneRenderer *renderer_;
};

#endif // TOOL_H
