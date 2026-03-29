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
    // Verify each concrete element creates with the expected reference label
    ResistorElement resistor;
    QCOMPARE(resistor.label(), QString("R"));

    CapacitorElement capacitor;
    QCOMPARE(capacitor.label(), QString("C"));

    InductorElement inductor;
    QCOMPARE(inductor.label(), QString("L"));

    FuseElement fuse;
    QCOMPARE(fuse.label(), QString("F"));

    CrystalElement crystal;
    QCOMPARE(crystal.label(), QString("Y"));

    TransformerElement transformer;
    QCOMPARE(transformer.label(), QString("T"));

    DiodeElement diode;
    QCOMPARE(diode.label(), QString("D"));

    LEDElement led;
    QCOMPARE(led.label(), QString("LED"));

    TransistorElement transistor;
    QCOMPARE(transistor.label(), QString("Q"));

    MOSFETElement mosfet;
    QCOMPARE(mosfet.label(), QString("Q"));

    OpAmpElement opamp;
    QCOMPARE(opamp.label(), QString("U"));

    VoltageRegulatorElement vreg;
    QCOMPARE(vreg.label(), QString("U"));

    BatteryElement battery;
    QCOMPARE(battery.label(), QString("BT"));

    GroundElement ground;
    QCOMPARE(ground.label(), QString("GND"));

    SwitchElement sw;
    QCOMPARE(sw.label(), QString("SW"));

    RelayElement relay;
    QCOMPARE(relay.label(), QString("K"));

    MotorElement motor;
    QCOMPARE(motor.label(), QString("M"));

    PowerSupplyElement psu;
    QCOMPARE(psu.label(), QString("PS"));

    MicrocontrollerElement mcu;
    QCOMPARE(mcu.label(), QString("U"));

    ICChipElement ic;
    QCOMPARE(ic.label(), QString("U"));

    SensorElement sensor;
    QCOMPARE(sensor.label(), QString("S"));

    AntennaElement antenna;
    QCOMPARE(antenna.label(), QString("ANT"));

    SpeakerElement speaker;
    QCOMPARE(speaker.label(), QString("SP"));

    ConnectorElement connector;
    QCOMPARE(connector.label(), QString("J"));
  }

  void allElementsBoundingRectConsistent() {
    // All elements share the same compact symbol dimensions plus pin margin.
    const qreal m = ElectronicsElementItem::PIN_RADIUS + 1.0;
    const QRectF expected(-m, -m, 64.0 + 2 * m, 48.0 + 14.0 + 2 * m);

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
             << new SwitchElement() << new RelayElement() << new MotorElement()
             << new PowerSupplyElement() << new MicrocontrollerElement()
             << new ICChipElement() << new SensorElement()
             << new AntennaElement() << new SpeakerElement()
             << new ConnectorElement();

    QCOMPARE(elements.size(), 24);

    QGraphicsScene scene;
    for (auto *e : elements)
      scene.addItem(e);
    QCOMPARE(scene.items().size(), 24);
  }
};

QTEST_MAIN(TestElectronicsElements)
#include "test_electronics_elements.moc"
