/**
 * @file test_busy_spinner_overlay.cpp
 * @brief Unit tests for BusySpinnerOverlay.
 */
#include "../src/widgets/busy_spinner_overlay.h"
#include <QTest>
#include <QWidget>

class TestBusySpinnerOverlay : public QObject {
  Q_OBJECT

private slots:
  void initiallyHidden() {
    QWidget parent;
    BusySpinnerOverlay spinner(&parent);
    QVERIFY(spinner.isHidden());
  }

  void startMakesVisible() {
    QWidget parent;
    parent.resize(200, 200);
    BusySpinnerOverlay spinner(&parent);
    spinner.start("Working…");
    QVERIFY(!spinner.isHidden());
  }

  void stopHidesWidget() {
    QWidget parent;
    parent.resize(200, 200);
    BusySpinnerOverlay spinner(&parent);
    spinner.start("Working…");
    QVERIFY(!spinner.isHidden());
    spinner.stop();
    QVERIFY(spinner.isHidden());
  }

  void setTextDoesNotShow() {
    QWidget parent;
    BusySpinnerOverlay spinner(&parent);
    spinner.setText("Loading…");
    // setText alone should not make the overlay visible
    QVERIFY(spinner.isHidden());
  }

  void startWithEmptyText() {
    QWidget parent;
    parent.resize(200, 200);
    BusySpinnerOverlay spinner(&parent);
    spinner.start();
    QVERIFY(!spinner.isHidden());
    spinner.stop();
    QVERIFY(spinner.isHidden());
  }

  void multipleStartStopCycles() {
    QWidget parent;
    parent.resize(200, 200);
    BusySpinnerOverlay spinner(&parent);
    for (int i = 0; i < 5; ++i) {
      spinner.start(QString("Cycle %1").arg(i));
      QVERIFY(!spinner.isHidden());
      spinner.stop();
      QVERIFY(spinner.isHidden());
    }
  }

  void geometryMatchesParent() {
    QWidget parent;
    parent.resize(400, 300);
    BusySpinnerOverlay spinner(&parent);
    spinner.start("Test");
    QCOMPARE(spinner.geometry(), parent.rect());
  }
};

QTEST_MAIN(TestBusySpinnerOverlay)
#include "test_busy_spinner_overlay.moc"
