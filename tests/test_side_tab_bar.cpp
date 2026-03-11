/**
 * @file test_side_tab_bar.cpp
 * @brief Unit tests for SideTabBar.
 */
#include "../src/widgets/side_tab_bar.h"
#include <QDockWidget>
#include <QMainWindow>
#include <QTest>

class TestSideTabBar : public QObject {
  Q_OBJECT

private slots:
  void initiallyHidden() {
    QMainWindow window;
    SideTabBar bar("Test", &window);
    QVERIFY(!bar.isVisible());
    QCOMPARE(bar.trackedCount(), 0);
    QCOMPARE(bar.visibleTabCount(), 0);
  }

  void trackVisibleDockShowsNoTab() {
    QMainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    SideTabBar bar("Test", &window);
    window.addToolBar(Qt::LeftToolBarArea, &bar);

    QDockWidget dock("Panel", &window);
    window.addDockWidget(Qt::LeftDockWidgetArea, &dock);
    dock.show();

    bar.trackDockWidget(&dock);
    QCOMPARE(bar.trackedCount(), 1);
    // Dock is visible so the tab should not be visible
    QCOMPARE(bar.visibleTabCount(), 0);
    QVERIFY(!bar.isVisible());
  }

  void trackHiddenDockShowsTab() {
    QMainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    SideTabBar bar("Test", &window);
    window.addToolBar(Qt::LeftToolBarArea, &bar);

    QDockWidget dock("Panel", &window);
    window.addDockWidget(Qt::LeftDockWidgetArea, &dock);
    dock.setProperty("restoreViaSideTab", true);
    dock.hide();

    bar.trackDockWidget(&dock);
    QCOMPARE(bar.trackedCount(), 1);
    QCOMPARE(bar.visibleTabCount(), 1);
    QVERIFY(bar.isVisible());
  }

  void hiddenDockWithoutCollapseShowsNoTab() {
    QMainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    SideTabBar bar("Test", &window);
    window.addToolBar(Qt::LeftToolBarArea, &bar);

    QDockWidget dock("Panel", &window);
    window.addDockWidget(Qt::LeftDockWidgetArea, &dock);
    dock.hide();

    bar.trackDockWidget(&dock);
    QCOMPARE(bar.trackedCount(), 1);
    QCOMPARE(bar.visibleTabCount(), 0);
    QVERIFY(!bar.isVisible());
  }

  void hidingDockAddsTab() {
    QMainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    SideTabBar bar("Test", &window);
    window.addToolBar(Qt::LeftToolBarArea, &bar);

    QDockWidget dock("Panel", &window);
    window.addDockWidget(Qt::LeftDockWidgetArea, &dock);
    dock.show();

    bar.trackDockWidget(&dock);
    QCOMPARE(bar.visibleTabCount(), 0);

    dock.setProperty("restoreViaSideTab", true);
    dock.hide();
    QCOMPARE(bar.visibleTabCount(), 1);
    QVERIFY(bar.isVisible());
  }

  void clickingTabRestoresDock() {
    QMainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    SideTabBar bar("Test", &window);
    window.addToolBar(Qt::LeftToolBarArea, &bar);

    QDockWidget dock("Panel", &window);
    window.addDockWidget(Qt::LeftDockWidgetArea, &dock);
    dock.setProperty("restoreViaSideTab", true);
    dock.hide();

    bar.trackDockWidget(&dock);
    QVERIFY(!dock.isVisible());
    QCOMPARE(bar.visibleTabCount(), 1);

    // Trigger the tab action — simulates a click on the tab
    QList<QAction *> actions = bar.actions();
    QVERIFY(!actions.isEmpty());
    actions.first()->trigger();

    QVERIFY(dock.isVisible());
  }

  void duplicateTrackIgnored() {
    QMainWindow window;
    SideTabBar bar("Test", &window);

    QDockWidget dock("Panel", &window);
    window.addDockWidget(Qt::LeftDockWidgetArea, &dock);

    bar.trackDockWidget(&dock);
    bar.trackDockWidget(&dock);
    QCOMPARE(bar.trackedCount(), 1);
  }

  void nullDockIgnored() {
    QMainWindow window;
    SideTabBar bar("Test", &window);
    bar.trackDockWidget(nullptr);
    QCOMPARE(bar.trackedCount(), 0);
  }

  void multipleDocksTracked() {
    QMainWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    SideTabBar bar("Test", &window);
    window.addToolBar(Qt::RightToolBarArea, &bar);

    QDockWidget dock1("Panel1", &window);
    QDockWidget dock2("Panel2", &window);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock1);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock2);
    dock1.show();
    dock2.show();

    bar.trackDockWidget(&dock1);
    bar.trackDockWidget(&dock2);
    QCOMPARE(bar.trackedCount(), 2);
    QCOMPARE(bar.visibleTabCount(), 0);

    // Hide one dock
    dock1.setProperty("restoreViaSideTab", true);
    dock1.hide();
    QCOMPARE(bar.visibleTabCount(), 1);
    QVERIFY(bar.isVisible());

    // Hide both
    dock2.setProperty("restoreViaSideTab", true);
    dock2.hide();
    QCOMPARE(bar.visibleTabCount(), 2);

    // Restore one
    dock1.show();
    QCOMPARE(bar.visibleTabCount(), 1);
    QVERIFY(bar.isVisible());

    // Restore both — bar should hide
    dock2.show();
    QCOMPARE(bar.visibleTabCount(), 0);
    QVERIFY(!bar.isVisible());
  }

  void barPropertiesSetCorrectly() {
    QMainWindow window;
    SideTabBar bar("LeftTabs", &window);
    QVERIFY(!bar.isMovable());
    QVERIFY(!bar.isFloatable());
    QCOMPARE(bar.windowTitle(), QString("LeftTabs"));
  }
};

QTEST_MAIN(TestSideTabBar)
#include "test_side_tab_bar.moc"
