/**
 * @file test_rotation_dialog.cpp
 * @brief Unit tests for RotationDialog.
 */
#include "../src/widgets/rotation_dialog.h"
#include <QTest>
#include <cmath>

class TestRotationDialog : public QObject {
  Q_OBJECT

private slots:

  void defaultAngleIsZero() {
    RotationDialog dlg;
    QCOMPARE(dlg.angle(), 0.0);
  }

  void windowTitleIsSet() {
    RotationDialog dlg;
    QCOMPARE(dlg.windowTitle(), QString("Rotate"));
  }

  void dialogIsModal() {
    RotationDialog dlg;
    QVERIFY(dlg.isModal());
  }
};

QTEST_MAIN(TestRotationDialog)
#include "test_rotation_dialog.moc"
