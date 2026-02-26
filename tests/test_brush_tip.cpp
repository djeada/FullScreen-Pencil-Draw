/**
 * @file test_brush_tip.cpp
 * @brief Unit tests for the BrushTip class.
 *
 * Tests cover:
 * - Default construction (Round shape)
 * - Shape getter/setter
 * - Chisel angle property
 * - Stamp spacing property
 * - tipShape() path generation for each shape
 * - renderTip() image generation for each shape
 * - shapeName() human-readable labels
 */
#include "../src/core/brush_tip.h"
#include <QColor>
#include <QImage>
#include <QPainterPath>
#include <QtTest/QtTest>

class TestBrushTip : public QObject {
  Q_OBJECT

private slots:

  // ====== Construction & defaults ======

  void testDefaultConstruction() {
    BrushTip tip;
    QCOMPARE(tip.shape(), BrushTipShape::Round);
    QCOMPARE(tip.angle(), 45.0);
    QCOMPARE(tip.stampSpacing(), 0.25);
    QVERIFY(tip.tipImage().isNull());
  }

  // ====== Property setters ======

  void testSetShape() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Chisel);
    QCOMPARE(tip.shape(), BrushTipShape::Chisel);
    tip.setShape(BrushTipShape::Stamp);
    QCOMPARE(tip.shape(), BrushTipShape::Stamp);
    tip.setShape(BrushTipShape::Textured);
    QCOMPARE(tip.shape(), BrushTipShape::Textured);
  }

  void testSetAngle() {
    BrushTip tip;
    tip.setAngle(30.0);
    QCOMPARE(tip.angle(), 30.0);
  }

  void testSetStampSpacing() {
    BrushTip tip;
    tip.setStampSpacing(0.5);
    QCOMPARE(tip.stampSpacing(), 0.5);
  }

  void testSetTipImage() {
    BrushTip tip;
    QImage img(16, 16, QImage::Format_ARGB32);
    img.fill(Qt::red);
    tip.setTipImage(img);
    QVERIFY(!tip.tipImage().isNull());
    QCOMPARE(tip.tipImage().size(), QSize(16, 16));
  }

  // ====== tipShape() ======

  void testTipShapeRound() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Round);
    QPainterPath path = tip.tipShape(10.0);
    QVERIFY(!path.isEmpty());
    // Round tip should contain the origin
    QVERIFY(path.contains(QPointF(0, 0)));
  }

  void testTipShapeChisel() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Chisel);
    tip.setAngle(0.0);
    QPainterPath path = tip.tipShape(20.0);
    QVERIFY(!path.isEmpty());
    // Horizontal chisel: wider than tall
    QRectF br = path.boundingRect();
    QVERIFY(br.width() > br.height());
  }

  void testTipShapeStampFallsBackToRound() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Stamp);
    QPainterPath path = tip.tipShape(10.0);
    QVERIFY(!path.isEmpty());
    QVERIFY(path.contains(QPointF(0, 0)));
  }

  // ====== renderTip() ======

  void testRenderTipRound() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Round);
    QImage img = tip.renderTip(20.0, Qt::red, 1.0);
    QCOMPARE(img.width(), 20);
    QCOMPARE(img.height(), 20);
    // Centre pixel should be opaque red-ish
    QColor center = img.pixelColor(10, 10);
    QVERIFY(center.alpha() > 0);
  }

  void testRenderTipChisel() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Chisel);
    QImage img = tip.renderTip(20.0, Qt::blue, 1.0);
    QCOMPARE(img.width(), 20);
    QVERIFY(!img.isNull());
  }

  void testRenderTipStamp() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Stamp);
    // Without custom image → fallback star stamp
    QImage img = tip.renderTip(30.0, Qt::green, 0.8);
    QCOMPARE(img.width(), 30);
    QVERIFY(!img.isNull());
  }

  void testRenderTipStampWithImage() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Stamp);
    QImage src(8, 8, QImage::Format_ARGB32);
    src.fill(Qt::yellow);
    tip.setTipImage(src);
    QImage img = tip.renderTip(20.0, Qt::yellow, 1.0);
    QVERIFY(!img.isNull());
  }

  void testRenderTipTextured() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Textured);
    QImage img = tip.renderTip(20.0, Qt::cyan, 0.5);
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), 20);
  }

  void testRenderTipZeroSize() {
    BrushTip tip;
    QImage img = tip.renderTip(0.0, Qt::white, 1.0);
    // Should produce a 1x1 image (clamped)
    QCOMPARE(img.width(), 1);
  }

  // ====== shapeName() ======

  void testShapeNameRound() {
    QCOMPARE(BrushTip::shapeName(BrushTipShape::Round), QString("Round"));
  }

  void testShapeNameChisel() {
    QCOMPARE(BrushTip::shapeName(BrushTipShape::Chisel), QString("Chisel"));
  }

  void testShapeNameStamp() {
    QCOMPARE(BrushTip::shapeName(BrushTipShape::Stamp), QString("Stamp"));
  }

  void testShapeNameTextured() {
    QCOMPARE(BrushTip::shapeName(BrushTipShape::Textured),
             QString("Textured"));
  }
};

QTEST_MAIN(TestBrushTip)
#include "test_brush_tip.moc"
