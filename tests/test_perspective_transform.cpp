/**
 * @file test_perspective_transform.cpp
 * @brief Unit tests for PerspectiveTransformDialog and
 * PerspectivePreviewWidget.
 */
#include "../src/widgets/perspective_transform_dialog.h"
#include <QTest>
#include <QTransform>
#include <cmath>

class TestPerspectiveTransform : public QObject {
  Q_OBJECT

private slots:

  void identityWhenCornersUnchanged() {
    PerspectiveTransformDialog dlg;
    QRectF rect(0, 0, 100, 100);
    QTransform t = dlg.perspectiveTransform(rect);
    QVERIFY(t.isIdentity());
  }

  void emptyRectReturnsIdentity() {
    PerspectiveTransformDialog dlg;
    QRectF rect;
    QTransform t = dlg.perspectiveTransform(rect);
    QVERIFY(t.isIdentity());
  }

  void previewWidgetResetIsIdentity() {
    PerspectivePreviewWidget widget;
    widget.reset();
    QCOMPARE(widget.topLeft(), QPointF(0.0, 0.0));
    QCOMPARE(widget.topRight(), QPointF(1.0, 0.0));
    QCOMPARE(widget.bottomRight(), QPointF(1.0, 1.0));
    QCOMPARE(widget.bottomLeft(), QPointF(0.0, 1.0));
  }

  void setCornerUpdatesValue() {
    PerspectivePreviewWidget widget;
    widget.setCorner(0, QPointF(0.1, 0.2));
    QCOMPARE(widget.topLeft(), QPointF(0.1, 0.2));
    // Others unchanged
    QCOMPARE(widget.topRight(), QPointF(1.0, 0.0));
  }

  void setCornerOutOfRangeIgnored() {
    PerspectivePreviewWidget widget;
    widget.setCorner(-1, QPointF(0.5, 0.5));
    widget.setCorner(4, QPointF(0.5, 0.5));
    // All corners unchanged
    QCOMPARE(widget.topLeft(), QPointF(0.0, 0.0));
    QCOMPARE(widget.bottomRight(), QPointF(1.0, 1.0));
  }

  void perspectiveTransformMapsCorners() {
    // Create a dialog and modify a corner, then check the transform
    // maps the source rectangle corners to the expected destination.
    PerspectiveTransformDialog dlg;

    // We can't easily set corners through the dialog without using
    // the preview widget, but we can verify the identity case maps
    // correctly.
    QRectF rect(10, 20, 200, 150);
    QTransform t = dlg.perspectiveTransform(rect);

    // Identity transform should map corners to themselves
    QPointF tl = t.map(rect.topLeft());
    QPointF tr = t.map(rect.topRight());
    QPointF br = t.map(rect.bottomRight());
    QPointF bl = t.map(rect.bottomLeft());

    auto fuzzyCompare = [](const QPointF &a, const QPointF &b) {
      return std::abs(a.x() - b.x()) < 0.01 && std::abs(a.y() - b.y()) < 0.01;
    };

    QVERIFY(fuzzyCompare(tl, rect.topLeft()));
    QVERIFY(fuzzyCompare(tr, rect.topRight()));
    QVERIFY(fuzzyCompare(br, rect.bottomRight()));
    QVERIFY(fuzzyCompare(bl, rect.bottomLeft()));
  }

  void resetAfterModification() {
    PerspectivePreviewWidget widget;
    widget.setCorner(0, QPointF(0.3, 0.3));
    widget.setCorner(2, QPointF(0.8, 0.8));
    widget.reset();
    QCOMPARE(widget.topLeft(), QPointF(0.0, 0.0));
    QCOMPARE(widget.bottomRight(), QPointF(1.0, 1.0));
  }
};

QTEST_MAIN(TestPerspectiveTransform)
#include "test_perspective_transform.moc"
