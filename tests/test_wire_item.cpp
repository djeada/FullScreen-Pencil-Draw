/**
 * @file test_wire_item.cpp
 * @brief Tests for WireItem and the ElectronicsElementItem pin system.
 */
#include "electronics_elements.h"
#include "wire_item.h"
#include <QGraphicsScene>
#include <QTest>

class TestWireItem : public QObject {
  Q_OBJECT

private slots:

  // --- Pin tests ----------------------------------------------------------

  void resistorHasTwoPins() {
    ResistorElement r;
    QCOMPARE(r.pins().size(), 2);
    QCOMPARE(r.pins()[0].name, QStringLiteral("pin1"));
    QCOMPARE(r.pins()[1].name, QStringLiteral("pin2"));
  }

  void groundHasOnePin() {
    GroundElement g;
    QCOMPARE(g.pins().size(), 1);
    QCOMPARE(g.pins()[0].name, QStringLiteral("gnd"));
  }

  void transistorHasThreePins() {
    TransistorElement t;
    QCOMPARE(t.pins().size(), 3);
    QCOMPARE(t.pins()[0].name, QStringLiteral("base"));
  }

  void transformerHasFourPins() {
    TransformerElement t;
    QCOMPARE(t.pins().size(), 4);
  }

  void opAmpHasThreePins() {
    OpAmpElement o;
    QCOMPARE(o.pins().size(), 3);
    QCOMPARE(o.pins()[0].name, QStringLiteral("+in"));
    QCOMPARE(o.pins()[1].name, QStringLiteral("-in"));
    QCOMPARE(o.pins()[2].name, QStringLiteral("out"));
  }

  void icChipHasFourPins() {
    ICChipElement ic;
    QCOMPARE(ic.pins().size(), 4);
  }

  void pinScenePosWithoutScene() {
    ResistorElement r;
    r.setPos(100.0, 200.0);
    // Without a scene the item isn't part of a scene graph.
    // mapToScene still works - it maps through pos().
    QPointF p0 = r.pinScenePos(0);
    // pin1 is at item-local (0, ELEM_H/2) == (0, 24)
    QCOMPARE(p0, QPointF(100.0, 224.0));
  }

  void nearestPinFindsClosest() {
    ResistorElement r;
    r.setPos(0, 0);
    // pin1 at (0, 24), pin2 at (64, 24)
    // Point near pin2
    int idx = r.nearestPin(QPointF(62.0, 24.0));
    QCOMPARE(idx, 1);
    // Point near pin1
    idx = r.nearestPin(QPointF(2.0, 24.0));
    QCOMPARE(idx, 0);
    // Point too far from both
    idx = r.nearestPin(QPointF(500.0, 500.0));
    QCOMPARE(idx, -1);
  }

  // --- Wire tests ---------------------------------------------------------

  void wireConnectsTwoElements() {
    QGraphicsScene scene;
    auto *r1 = new ResistorElement();
    auto *r2 = new ResistorElement();
    r1->setPos(0, 0);
    r2->setPos(300, 0);
    scene.addItem(r1);
    scene.addItem(r2);

    auto *wire = new WireItem(r1, 1, r2, 0); // r1 pin2 → r2 pin1
    scene.addItem(wire);

    QCOMPARE(wire->sourceElement(), r1);
    QCOMPARE(wire->sourcePin(), 1);
    QCOMPARE(wire->destElement(), r2);
    QCOMPARE(wire->destPin(), 0);

    // Wire path should be non-empty
    QVERIFY(!wire->path().isEmpty());
  }

  void wireUpdatesWhenElementMoves() {
    QGraphicsScene scene;
    auto *r1 = new ResistorElement();
    auto *r2 = new ResistorElement();
    r1->setPos(0, 0);
    r2->setPos(300, 0);
    scene.addItem(r1);
    scene.addItem(r2);

    auto *wire = new WireItem(r1, 1, r2, 0);
    scene.addItem(wire);

    QPainterPath before = wire->path();

    // Move the second element
    r2->setPos(400, 100);

    QPainterPath after = wire->path();
    // The wire path should have changed
    QVERIFY(before != after);
  }

  void wireDisconnectsOnDestruction() {
    QGraphicsScene scene;
    auto *r1 = new ResistorElement();
    auto *r2 = new ResistorElement();
    r1->setPos(0, 0);
    r2->setPos(300, 0);
    scene.addItem(r1);
    scene.addItem(r2);

    auto *wire = new WireItem(r1, 1, r2, 0);
    scene.addItem(wire);

    // Deleting the wire should not crash when elements still exist
    scene.removeItem(wire);
    delete wire;

    // Moving elements should not crash after wire is deleted
    r1->setPos(50, 50);
    r2->setPos(350, 50);
  }

  void allElementsHavePins() {
    // Every electronics element should have at least one pin
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

    QGraphicsScene scene;
    for (auto *e : elements) {
      scene.addItem(e);
      QVERIFY2(e->pins().size() >= 1,
               qPrintable(e->label() + " has no pins"));
    }
  }
};

QTEST_MAIN(TestWireItem)
#include "test_wire_item.moc"
