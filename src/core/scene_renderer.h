/**
 * @file scene_renderer.h
 * @brief Abstract interface for scene renderers that support drawing tools.
 *
 * This interface abstracts the common functionality needed by drawing tools,
 * allowing them to work with both Canvas and PdfViewer (or any other renderer).
 */
#ifndef SCENE_RENDERER_H
#define SCENE_RENDERER_H

#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPen>
#include <QScrollBar>
#include <memory>

#include "item_id.h"
#include "scene_controller.h"

class Action;
class ItemStore;

/**
 * @brief Abstract interface for scene renderers.
 *
 * This interface defines the common methods that drawing tools need
 * to interact with a graphics scene. Both Canvas and PdfViewer
 * implement this interface, allowing a single set of tools to work
 * with both renderers.
 */
class SceneRenderer {
public:
  virtual ~SceneRenderer() = default;

  /**
   * @brief Get the graphics scene
   * @return Pointer to the QGraphicsScene
   */
  virtual QGraphicsScene *scene() const = 0;

  /**
   * @brief Get the current pen for drawing
   * @return Reference to the current pen
   */
  virtual const QPen &currentPen() const = 0;

  /**
   * @brief Get the eraser pen
   * @return Reference to the eraser pen
   */
  virtual const QPen &eraserPen() const = 0;

  /**
   * @brief Get the background image item (if any)
   * @return Pointer to the background image item, or nullptr
   */
  virtual QGraphicsPixmapItem *backgroundImageItem() const = 0;

  /**
   * @brief Check if shapes should be filled
   * @return true if shapes should be filled
   */
  virtual bool isFilledShapes() const = 0;

  /**
   * @brief Check if pressure sensitivity is enabled
   * @return true if brush dynamics should respond to stylus pressure
   */
  virtual bool isPressureSensitive() const { return false; }

  /**
   * @brief Add a draw action to the undo stack
   * @param item The graphics item that was drawn
   */
  virtual void addDrawAction(QGraphicsItem *item) = 0;

  /**
   * @brief Add a delete action to the undo stack
   * @param item The graphics item that was deleted
   */
  virtual void addDeleteAction(QGraphicsItem *item) = 0;

  /**
   * @brief Notify the renderer that an item was removed from the scene
   * @param item The graphics item that was removed
   */
  virtual void onItemRemoved(QGraphicsItem *item) = 0;

  /**
   * @brief Add a custom action to the undo stack
   * @param action The action to add
   */
  virtual void addAction(std::unique_ptr<Action> action) = 0;

  /**
   * @brief Set the cursor
   * @param cursor The cursor to set
   */
  virtual void setCursor(const QCursor &cursor) = 0;

  /**
   * @brief Get the horizontal scroll bar
   * @return Pointer to the horizontal scroll bar
   */
  virtual QScrollBar *horizontalScrollBar() const = 0;

  /**
   * @brief Get the vertical scroll bar
   * @return Pointer to the vertical scroll bar
   */
  virtual QScrollBar *verticalScrollBar() const = 0;

  /**
   * @brief Get the scene controller for item lifecycle management
   * @return Pointer to the SceneController, or nullptr if not available
   * @note Tools should use this for creating/removing items when available
   */
  virtual SceneController *sceneController() const { return nullptr; }

  /**
   * @brief Get the item store for ItemId resolution
   * @return Pointer to the ItemStore, or nullptr if not available
   */
  virtual ItemStore *itemStore() const { return nullptr; }

  /**
   * @brief Register an item and get its ItemId
   * @param item The item to register
   * @return The assigned ItemId
   * @note If sceneController() is available, uses that; otherwise returns null
   * ItemId
   */
  virtual ItemId registerItem(QGraphicsItem *item) {
    if (auto *controller = sceneController()) {
      return controller->addItem(item);
    }
    return ItemId();
  }
};

#endif // SCENE_RENDERER_H
