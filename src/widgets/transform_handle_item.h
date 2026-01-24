/**
 * @file transform_handle_item.h
 * @brief Visual transform handles for resize and rotate operations.
 */
#ifndef TRANSFORM_HANDLE_ITEM_H
#define TRANSFORM_HANDLE_ITEM_H

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QBrush>

class SceneRenderer;

/**
 * @brief Enum for handle types
 */
enum class HandleType {
  None,
  TopLeft,
  TopCenter,
  TopRight,
  MiddleLeft,
  MiddleRight,
  BottomLeft,
  BottomCenter,
  BottomRight,
  Rotate
};

/**
 * @brief Visual overlay for transform handles on selected items.
 *
 * This class creates a professional-looking selection box with:
 * - 8 resize handles at corners and edges
 * - A rotation handle above the selection
 * - Visual feedback during transforms
 */
class TransformHandleItem : public QGraphicsObject {
  Q_OBJECT

public:
  // Custom type for qgraphicsitem_cast
  enum { Type = UserType + 100 };
  int type() const override { return Type; }

  explicit TransformHandleItem(QGraphicsItem *targetItem,
                               SceneRenderer *renderer,
                               QGraphicsItem *parent = nullptr);
  ~TransformHandleItem() override;

  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  /**
   * @brief Update the handles to match the target item's bounds
   */
  void updateHandles();

  /**
   * @brief Get the target item being transformed
   */
  QGraphicsItem *targetItem() const { return targetItem_; }

  /**
   * @brief Check if currently transforming
   */
  bool isTransforming() const { return isTransforming_; }

signals:
  /**
   * @brief Emitted when a transform operation is completed
   */
  void transformCompleted();

protected:
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
  bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
  void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
  // Handle constants
  static constexpr qreal HANDLE_SIZE = 10.0;
  static constexpr qreal HANDLE_HALF = HANDLE_SIZE / 2.0;
  static constexpr qreal ROTATION_HANDLE_OFFSET = 30.0;
  static constexpr qreal ROTATION_HANDLE_RADIUS = 7.0;
  static constexpr qreal SELECTION_BORDER_WIDTH = 1.5;

  // Colors for professional appearance
  static inline const QColor HANDLE_FILL_COLOR = QColor(255, 255, 255);
  static inline const QColor HANDLE_BORDER_COLOR = QColor(0, 120, 215);
  static inline const QColor SELECTION_BORDER_COLOR = QColor(0, 120, 215);
  static inline const QColor ROTATION_HANDLE_COLOR = QColor(76, 175, 80);

  // Target item
  QGraphicsItem *targetItem_;
  SceneRenderer *renderer_;
  bool sceneEventFilterInstalled_;

  // State
  bool isTransforming_;
  HandleType activeHandle_;
  QPointF lastMousePos_;
  QPointF transformOrigin_;

  // Pre-transform state for undo
  QTransform originalTransform_;
  QPointF originalPos_;
  QRectF originalBounds_;
  
  // Original item flags to restore after transform
  bool wasMovable_;
  bool wasSelectable_;
  
  // Cached bounds for detecting target item movement
  mutable QRectF cachedTargetBounds_;

  // Helper methods
  HandleType handleAtPoint(const QPointF &pos) const;
  QRectF handleRect(HandleType type) const;
  QCursor cursorForHandle(HandleType type) const;
  void applyResize(const QPointF &mousePos);
  void applyRotation(const QPointF &mousePos);
  QRectF targetBoundsInScene() const;
  void ensureSceneEventFilter();
};

#endif // TRANSFORM_HANDLE_ITEM_H
