/**
 * @file test_electronics_elements.cpp
 * @brief Tests for electronics diagram element classes.
 */
#include "electronics_elements.h"
#include <QGraphicsScene>
#include <QTest>

class TestElectronicsElements : public QObject {
  Q_OBJECT

private slots:

  void allElementsHaveCorrectLabel() {
    // Verify each concrete element creates with the expected label
    ResistorElement resistor;
    QCOMPARE(resistor.label(), QString("Resistor"));

    CapacitorElement capacitor;
    QCOMPARE(capacitor.label(), QString("Capacitor"));

    InductorElement inductor;
    QCOMPARE(inductor.label(), QString("Inductor"));

    FuseElement fuse;
    QCOMPARE(fuse.label(), QString("Fuse"));

    CrystalElement crystal;
    QCOMPARE(crystal.label(), QString("Crystal"));

    TransformerElement transformer;
    QCOMPARE(transformer.label(), QString("Transformer"));

    DiodeElement diode;
    QCOMPARE(diode.label(), QString("Diode"));

    LEDElement led;
    QCOMPARE(led.label(), QString("LED"));

    TransistorElement transistor;
    QCOMPARE(transistor.label(), QString("Transistor"));

    MOSFETElement mosfet;
    QCOMPARE(mosfet.label(), QString("MOSFET"));

    OpAmpElement opamp;
    QCOMPARE(opamp.label(), QString("Op-Amp"));

    VoltageRegulatorElement vreg;
    QCOMPARE(vreg.label(), QString("Regulator"));

    BatteryElement battery;
    QCOMPARE(battery.label(), QString("Battery"));

    GroundElement ground;
    QCOMPARE(ground.label(), QString("Ground"));

    SwitchElement sw;
    QCOMPARE(sw.label(), QString("Switch"));

    RelayElement relay;
    QCOMPARE(relay.label(), QString("Relay"));

    MotorElement motor;
    QCOMPARE(motor.label(), QString("Motor"));

    PowerSupplyElement psu;
    QCOMPARE(psu.label(), QString("PSU"));

    MicrocontrollerElement mcu;
    QCOMPARE(mcu.label(), QString("MCU"));

    ICChipElement ic;
    QCOMPARE(ic.label(), QString("IC Chip"));

    SensorElement sensor;
    QCOMPARE(sensor.label(), QString("Sensor"));

    AntennaElement antenna;
    QCOMPARE(antenna.label(), QString("Antenna"));

    SpeakerElement speaker;
    QCOMPARE(speaker.label(), QString("Speaker"));

    ConnectorElement connector;
    QCOMPARE(connector.label(), QString("Connector"));
  }

  void allElementsBoundingRectConsistent() {
    // All elements share the same card dimensions plus pin margin.
    const qreal m = ElectronicsElementItem::PIN_RADIUS + 1.0;
    const QRectF expected(-m, -m, 142.0 + 2 * m, 106.0 + 2 * m);

    ResistorElement resistor;
    QCOMPARE(resistor.boundingRect(), expected);

    DiodeElement diode;
    QCOMPARE(diode.boundingRect(), expected);

    BatteryElement battery;
    QCOMPARE(battery.boundingRect(), expected);

    MicrocontrollerElement mcu;
    QCOMPARE(mcu.boundingRect(), expected);

    SpeakerElement speaker;
    QCOMPARE(speaker.boundingRect(), expected);

    OpAmpElement opamp;
    QCOMPARE(opamp.boundingRect(), expected);

    ConnectorElement connector;
    QCOMPARE(connector.boundingRect(), expected);
  }

  void elementsAreSelectableAndMovable() {
    ResistorElement resistor;
    QVERIFY(resistor.flags() & QGraphicsItem::ItemIsSelectable);
    QVERIFY(resistor.flags() & QGraphicsItem::ItemIsMovable);

    TransistorElement transistor;
    QVERIFY(transistor.flags() & QGraphicsItem::ItemIsSelectable);
    QVERIFY(transistor.flags() & QGraphicsItem::ItemIsMovable);

    ICChipElement ic;
    QVERIFY(ic.flags() & QGraphicsItem::ItemIsSelectable);
    QVERIFY(ic.flags() & QGraphicsItem::ItemIsMovable);
  }

  void elementsCanBeAddedToScene() {
    QGraphicsScene scene;
    auto *resistor = new ResistorElement();
    auto *capacitor = new CapacitorElement();
    auto *diode = new DiodeElement();

    scene.addItem(resistor);
    scene.addItem(capacitor);
    scene.addItem(diode);

    QCOMPARE(scene.items().size(), 3);
  }

  void totalElementCount() {
    // Verify we have the expected total number of element types (24)
    // by checking that all 24 element classes can be instantiated
    QVector<ElectronicsElementItem *> elements;
    elements << new ResistorElement() << new CapacitorElement()
             << new InductorElement() << new FuseElement()
             << new CrystalElement() << new TransformerElement()
             << new DiodeElement() << new LEDElement()
             << new TransistorElement() << new MOSFETElement()
             << new OpAmpElement() << new VoltageRegulatorElement()
             << new BatteryElement() << new GroundElement()
             << new SwitchElement() << new RelayElement()
             << new MotorElement() << new PowerSupplyElement()
             << new MicrocontrollerElement() << new ICChipElement()
             << new SensorElement() << new AntennaElement()
             << new SpeakerElement() << new ConnectorElement();

    QCOMPARE(elements.size(), 24);

    QGraphicsScene scene;
    for (auto *e : elements)
      scene.addItem(e);
    QCOMPARE(scene.items().size(), 24);
  }
};

QTEST_MAIN(TestElectronicsElements)
#include "test_electronics_elements.moc"
