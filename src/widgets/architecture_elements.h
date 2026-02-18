/**
 * @file architecture_elements.h
 * @brief Custom vector-drawn QGraphicsItem subclasses for CS architecture
 *        diagram elements.
 *
 * Each element renders its own icon using QPainter vector operations so that
 * the output scales cleanly and does not depend on emoji font availability.
 */
#ifndef ARCHITECTURE_ELEMENTS_H
#define ARCHITECTURE_ELEMENTS_H

#include <QGraphicsItem>
#include <QPainter>
#include <QPen>

/**
 * @brief Base class for all architecture diagram elements.
 *
 * Provides a common rounded-rectangle background, a label, and the
 * selectable/movable flags.  Subclasses override paintIcon() to draw
 * their unique vector icon.
 */
class ArchitectureElementItem : public QGraphicsItem {
public:
  explicit ArchitectureElementItem(const QString &label,
                                   QGraphicsItem *parent = nullptr);
  ~ArchitectureElementItem() override = default;

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  /// Returns the element's display label.
  QString label() const { return label_; }

protected:
  /// Override in subclasses to draw the element-specific vector icon.
  /// @p rect is the area reserved for the icon (centred horizontally).
  virtual void paintIcon(QPainter *painter, const QRectF &rect) const = 0;

  static constexpr qreal ELEM_W = 110.0;
  static constexpr qreal ELEM_H = 80.0;
  static constexpr qreal CORNER = 8.0;
  static constexpr qreal ICON_SIZE = 36.0;

private:
  QString label_;
};

// ----- Concrete element classes -----

class ClientElement : public ArchitectureElementItem {
public:
  explicit ClientElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class LoadBalancerElement : public ArchitectureElementItem {
public:
  explicit LoadBalancerElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class ApiGatewayElement : public ArchitectureElementItem {
public:
  explicit ApiGatewayElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class AppServerElement : public ArchitectureElementItem {
public:
  explicit AppServerElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class CacheElement : public ArchitectureElementItem {
public:
  explicit CacheElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class MessageQueueElement : public ArchitectureElementItem {
public:
  explicit MessageQueueElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class DatabaseElement : public ArchitectureElementItem {
public:
  explicit DatabaseElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class ObjectStorageElement : public ArchitectureElementItem {
public:
  explicit ObjectStorageElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class AuthElement : public ArchitectureElementItem {
public:
  explicit AuthElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

class MonitoringElement : public ArchitectureElementItem {
public:
  explicit MonitoringElement(QGraphicsItem *parent = nullptr);

protected:
  void paintIcon(QPainter *painter, const QRectF &rect) const override;
};

#endif // ARCHITECTURE_ELEMENTS_H
