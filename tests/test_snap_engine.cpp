/**
 * @file test_snap_engine.cpp
 * @brief Unit tests for the SnapEngine class.
 *
 * Tests cover:
 * - Snap-to-grid: rounding to nearest grid intersection
 * - Snap-to-object: snapping to bounding-box edges and centers
 * - Combined grid + object snapping (nearest wins)
 * - Threshold behaviour (no snap when too far away)
 * - Exclude set filtering
 */
#include "../src/core/snap_engine.h"
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QtTest/QtTest>

class TestSnapEngine : public QObject {
  Q_OBJECT

private slots:
  // ====== Grid snapping ======

  void testGridSnapBasic() {
    SnapEngine engine(20, 10.0);
    engine.setSnapToGridEnabled(true);

    // Point near a grid intersection
    SnapResult r = engine.snapToGrid(QPointF(22, 38));
    QCOMPARE(r.snappedPoint, QPointF(20, 40));
    QVERIFY(r.snappedX);
    QVERIFY(r.snappedY);
  }

  void testGridSnapExactPoint() {
    SnapEngine engine(20, 10.0);
    engine.setSnapToGridEnabled(true);

    SnapResult r = engine.snapToGrid(QPointF(40, 60));
    QCOMPARE(r.snappedPoint, QPointF(40, 60));
    QVERIFY(r.snappedX);
    QVERIFY(r.snappedY);
  }

  void testGridSnapDisabled() {
    SnapEngine engine(20, 10.0);
    engine.setSnapToGridEnabled(false);

    SnapResult r = engine.snapToGrid(QPointF(22, 38));
    QCOMPARE(r.snappedPoint, QPointF(22, 38));
    QVERIFY(!r.snappedX);
    QVERIFY(!r.snappedY);
  }

  void testGridSnapNegativeCoords() {
    SnapEngine engine(20, 10.0);
    engine.setSnapToGridEnabled(true);

    SnapResult r = engine.snapToGrid(QPointF(-18, -42));
    QCOMPARE(r.snappedPoint, QPointF(-20, -40));
    QVERIFY(r.snappedX);
    QVERIFY(r.snappedY);
  }

  void testGridSnapBeyondThreshold() {
    SnapEngine engine(20, 5.0);
    engine.setSnapToGridEnabled(true);

    // 11 units away from nearest grid line (20) – exceeds threshold of 5
    SnapResult r = engine.snapToGrid(QPointF(11, 11));
    // X: nearest grid = 20, distance = 9 > 5 → no snap
    // Y: same
    // Actually nearest grid for 11 is 20 (dist 9) or 0 (dist 11) - both > 5
    QVERIFY(!r.snappedX);
    QVERIFY(!r.snappedY);
    QCOMPARE(r.snappedPoint, QPointF(11, 11));
  }

  // ====== Object snapping ======

  void testObjectSnapToEdge() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToObjectEnabled(true);

    // Create a rect at (100, 100) with size 50x50
    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPen(QPen(Qt::NoPen));
    rect->setPos(100, 100);
    scene.addItem(rect);

    // Point near the left edge of rect (100)
    SnapResult r =
        engine.snap(QPointF(103, 70), scene.items());
    QVERIFY(r.snappedX);
    QCOMPARE(r.snappedPoint.x(), 100.0);
  }

  void testObjectSnapToCenter() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToObjectEnabled(true);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPen(QPen(Qt::NoPen));
    rect->setPos(100, 100);
    scene.addItem(rect);

    // Center of rect bounding box is (125, 125)
    SnapResult r =
        engine.snap(QPointF(123, 127), scene.items());
    QVERIFY(r.snappedX);
    QVERIFY(r.snappedY);
    QCOMPARE(r.snappedPoint.x(), 125.0);
    QCOMPARE(r.snappedPoint.y(), 125.0);
  }

  void testObjectSnapExcludeItems() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToObjectEnabled(true);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPen(QPen(Qt::NoPen));
    rect->setPos(100, 100);
    scene.addItem(rect);

    // Exclude the rect → no object targets, no snap
    QSet<QGraphicsItem *> excludeSet;
    excludeSet.insert(rect);

    SnapResult r =
        engine.snap(QPointF(103, 103), scene.items(), excludeSet);
    QVERIFY(!r.snappedX);
    QVERIFY(!r.snappedY);
    QCOMPARE(r.snappedPoint, QPointF(103, 103));
  }

  void testObjectSnapDisabled() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToObjectEnabled(false);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPen(QPen(Qt::NoPen));
    rect->setPos(100, 100);
    scene.addItem(rect);

    SnapResult r =
        engine.snap(QPointF(103, 103), scene.items());
    QVERIFY(!r.snappedX);
    QVERIFY(!r.snappedY);
  }

  // ====== Combined snapping ======

  void testCombinedSnapGridWins() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToGridEnabled(true);
    engine.setSnapToObjectEnabled(true);

    // Object at (105, 105)
    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPen(QPen(Qt::NoPen));
    rect->setPos(105, 105);
    scene.addItem(rect);

    // Point at (99, 99) – grid at (100,100) is 1 away, object left edge (105)
    // is 6 away Grid wins on both axes
    SnapResult r =
        engine.snap(QPointF(99, 99), scene.items());
    QCOMPARE(r.snappedPoint.x(), 100.0);
    QCOMPARE(r.snappedPoint.y(), 100.0);
    QVERIFY(r.snappedX);
    QVERIFY(r.snappedY);
  }

  void testCombinedSnapObjectWins() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToGridEnabled(true);
    engine.setSnapToObjectEnabled(true);

    // Object at (102, 102)
    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPen(QPen(Qt::NoPen));
    rect->setPos(102, 102);
    scene.addItem(rect);

    // Point at (103, 103) – grid at (100,100) is 3 away, object left edge
    // (102) is 1 away. Object wins.
    SnapResult r =
        engine.snap(QPointF(103, 103), scene.items());
    QCOMPARE(r.snappedPoint.x(), 102.0);
    QCOMPARE(r.snappedPoint.y(), 102.0);
    QVERIFY(r.snappedX);
    QVERIFY(r.snappedY);
  }

  void testNothingEnabled() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);

    SnapResult r =
        engine.snap(QPointF(123, 456), scene.items());
    QCOMPARE(r.snappedPoint, QPointF(123, 456));
    QVERIFY(!r.snappedX);
    QVERIFY(!r.snappedY);
  }

  // ====== Configuration ======

  void testSettersAndGetters() {
    SnapEngine engine;
    QCOMPARE(engine.gridSize(), 20);
    QCOMPARE(engine.snapThreshold(), 10.0);
    QVERIFY(!engine.isSnapToGridEnabled());
    QVERIFY(!engine.isSnapToObjectEnabled());

    engine.setGridSize(40);
    engine.setSnapThreshold(5.0);
    engine.setSnapToGridEnabled(true);
    engine.setSnapToObjectEnabled(true);

    QCOMPARE(engine.gridSize(), 40);
    QCOMPARE(engine.snapThreshold(), 5.0);
    QVERIFY(engine.isSnapToGridEnabled());
    QVERIFY(engine.isSnapToObjectEnabled());
  }

  void testGuideLineValues() {
    SnapEngine engine(20, 10.0);
    engine.setSnapToGridEnabled(true);

    SnapResult r = engine.snapToGrid(QPointF(22, 38));
    QCOMPARE(r.guideX, 20.0);
    QCOMPARE(r.guideY, 40.0);
  }

  void testSnapToObjectMultipleItems() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToObjectEnabled(true);

    // Two rectangles
    auto *rect1 = new QGraphicsRectItem(0, 0, 50, 50);
    rect1->setPen(QPen(Qt::NoPen));
    rect1->setPos(100, 100);
    scene.addItem(rect1);

    auto *rect2 = new QGraphicsRectItem(0, 0, 50, 50);
    rect2->setPen(QPen(Qt::NoPen));
    rect2->setPos(200, 200);
    scene.addItem(rect2);

    // Point near rect2's left edge (200)
    SnapResult r =
        engine.snap(QPointF(202, 202), scene.items());
    QVERIFY(r.snappedX);
    QVERIFY(r.snappedY);
    QCOMPARE(r.snappedPoint.x(), 200.0);
    QCOMPARE(r.snappedPoint.y(), 200.0);
  }

  void testHiddenItemsIgnored() {
    QGraphicsScene scene;
    SnapEngine engine(20, 10.0);
    engine.setSnapToObjectEnabled(true);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setPen(QPen(Qt::NoPen));
    rect->setPos(100, 100);
    rect->setVisible(false);
    scene.addItem(rect);

    SnapResult r =
        engine.snap(QPointF(103, 103), scene.items());
    QVERIFY(!r.snappedX);
    QVERIFY(!r.snappedY);
  }
};

QTEST_MAIN(TestSnapEngine)
#include "test_snap_engine.moc"
