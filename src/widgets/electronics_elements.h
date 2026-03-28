/**
 * @file electronics_elements.h
 * @brief Custom vector-drawn QGraphicsItem subclasses for electronics
 *        schematic diagram elements.
 *
 * Each element renders a shared card style plus a vector icon, keeping
 * electronics nodes visually consistent and crisp during transform scaling.
 */
#ifndef ELECTRONICS_ELEMENTS_H
#define ELECTRONICS_ELEMENTS_H

#include <QColor>
#include <QGraphicsItem>
#include <QPainter>
#include <QPen>

/**
 * @brief Base class for all electronics diagram elements.
 *
 * Provides a common rounded-card background, a label, icon rendering, and
 * selectable/movable flags.
 */
class ElectronicsElementItem : public QGraphicsItem {
public:
  enum class IconKind {
    Resistor,
    Capacitor,
    Inductor,
    Fuse,
    Crystal,
    Transformer,
    Diode,
    LED,
    Transistor,
    MOSFET,
    OpAmp,
    VoltageRegulator,
    Battery,
    Ground,
    Switch,
    Relay,
    Motor,
    PowerSupply,
    Microcontroller,
    ICChip,
    Sensor,
    Antenna,
    Speaker,
    Connector
  };

  explicit ElectronicsElementItem(const QString &label, IconKind iconKind,
                                  const QColor &accentColor,
                                  QGraphicsItem *parent = nullptr);
  ~ElectronicsElementItem() override = default;

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

class ResistorElement : public ElectronicsElementItem {
public:
  explicit ResistorElement(QGraphicsItem *parent = nullptr);
};

class CapacitorElement : public ElectronicsElementItem {
public:
  explicit CapacitorElement(QGraphicsItem *parent = nullptr);
};

class InductorElement : public ElectronicsElementItem {
public:
  explicit InductorElement(QGraphicsItem *parent = nullptr);
};

class FuseElement : public ElectronicsElementItem {
public:
  explicit FuseElement(QGraphicsItem *parent = nullptr);
};

class CrystalElement : public ElectronicsElementItem {
public:
  explicit CrystalElement(QGraphicsItem *parent = nullptr);
};

class TransformerElement : public ElectronicsElementItem {
public:
  explicit TransformerElement(QGraphicsItem *parent = nullptr);
};

class DiodeElement : public ElectronicsElementItem {
public:
  explicit DiodeElement(QGraphicsItem *parent = nullptr);
};

class LEDElement : public ElectronicsElementItem {
public:
  explicit LEDElement(QGraphicsItem *parent = nullptr);
};

class TransistorElement : public ElectronicsElementItem {
public:
  explicit TransistorElement(QGraphicsItem *parent = nullptr);
};

class MOSFETElement : public ElectronicsElementItem {
public:
  explicit MOSFETElement(QGraphicsItem *parent = nullptr);
};

class OpAmpElement : public ElectronicsElementItem {
public:
  explicit OpAmpElement(QGraphicsItem *parent = nullptr);
};

class VoltageRegulatorElement : public ElectronicsElementItem {
public:
  explicit VoltageRegulatorElement(QGraphicsItem *parent = nullptr);
};

class BatteryElement : public ElectronicsElementItem {
public:
  explicit BatteryElement(QGraphicsItem *parent = nullptr);
};

class GroundElement : public ElectronicsElementItem {
public:
  explicit GroundElement(QGraphicsItem *parent = nullptr);
};

class SwitchElement : public ElectronicsElementItem {
public:
  explicit SwitchElement(QGraphicsItem *parent = nullptr);
};

class RelayElement : public ElectronicsElementItem {
public:
  explicit RelayElement(QGraphicsItem *parent = nullptr);
};

class MotorElement : public ElectronicsElementItem {
public:
  explicit MotorElement(QGraphicsItem *parent = nullptr);
};

class PowerSupplyElement : public ElectronicsElementItem {
public:
  explicit PowerSupplyElement(QGraphicsItem *parent = nullptr);
};

class MicrocontrollerElement : public ElectronicsElementItem {
public:
  explicit MicrocontrollerElement(QGraphicsItem *parent = nullptr);
};

class ICChipElement : public ElectronicsElementItem {
public:
  explicit ICChipElement(QGraphicsItem *parent = nullptr);
};

class SensorElement : public ElectronicsElementItem {
public:
  explicit SensorElement(QGraphicsItem *parent = nullptr);
};

class AntennaElement : public ElectronicsElementItem {
public:
  explicit AntennaElement(QGraphicsItem *parent = nullptr);
};

class SpeakerElement : public ElectronicsElementItem {
public:
  explicit SpeakerElement(QGraphicsItem *parent = nullptr);
};

class ConnectorElement : public ElectronicsElementItem {
public:
  explicit ConnectorElement(QGraphicsItem *parent = nullptr);
};

#endif // ELECTRONICS_ELEMENTS_H
