/**
 * @file architecture_elements.h
 * @brief Custom vector-drawn QGraphicsItem subclasses for CS architecture
 *        diagram elements.
 *
 * Each element renders a shared card style plus a vector icon, keeping
 * architecture nodes visually consistent and crisp during transform scaling.
 */
#ifndef ARCHITECTURE_ELEMENTS_H
#define ARCHITECTURE_ELEMENTS_H

#include <QColor>
#include <QGraphicsItem>
#include <QPainter>
#include <QPen>

/**
 * @brief Base class for all architecture diagram elements.
 *
 * Provides a common rounded-card background, a label, icon rendering, and
 * selectable/movable flags.
 */
class ArchitectureElementItem : public QGraphicsItem {
public:
  enum class IconKind {
    Client,
    LoadBalancer,
    ApiGateway,
    AppServer,
    Cache,
    MessageQueue,
    Database,
    ObjectStorage,
    Auth,
    Monitoring
  };

  explicit ArchitectureElementItem(const QString &label, IconKind iconKind,
                                   const QColor &accentColor,
                                   QGraphicsItem *parent = nullptr);
  ~ArchitectureElementItem() override = default;

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  /// Returns the element's display label.
  QString label() const { return label_; }

protected:
  /// Draws the element icon in vector form for crisp scaling.
  void paintIcon(QPainter *painter, const QRectF &rect) const;

  static constexpr qreal ELEM_W = 142.0;
  static constexpr qreal ELEM_H = 106.0;
  static constexpr qreal CORNER = 13.0;
  static constexpr qreal ICON_SIZE = 46.0;

private:
  QString label_;
  IconKind iconKind_;
  QColor accentColor_;
};

// ----- Concrete element classes -----

class ClientElement : public ArchitectureElementItem {
public:
  explicit ClientElement(QGraphicsItem *parent = nullptr);
};

class LoadBalancerElement : public ArchitectureElementItem {
public:
  explicit LoadBalancerElement(QGraphicsItem *parent = nullptr);
};

class ApiGatewayElement : public ArchitectureElementItem {
public:
  explicit ApiGatewayElement(QGraphicsItem *parent = nullptr);
};

class AppServerElement : public ArchitectureElementItem {
public:
  explicit AppServerElement(QGraphicsItem *parent = nullptr);
};

class CacheElement : public ArchitectureElementItem {
public:
  explicit CacheElement(QGraphicsItem *parent = nullptr);
};

class MessageQueueElement : public ArchitectureElementItem {
public:
  explicit MessageQueueElement(QGraphicsItem *parent = nullptr);
};

class DatabaseElement : public ArchitectureElementItem {
public:
  explicit DatabaseElement(QGraphicsItem *parent = nullptr);
};

class ObjectStorageElement : public ArchitectureElementItem {
public:
  explicit ObjectStorageElement(QGraphicsItem *parent = nullptr);
};

class AuthElement : public ArchitectureElementItem {
public:
  explicit AuthElement(QGraphicsItem *parent = nullptr);
};

class MonitoringElement : public ArchitectureElementItem {
public:
  explicit MonitoringElement(QGraphicsItem *parent = nullptr);
};

#endif // ARCHITECTURE_ELEMENTS_H
