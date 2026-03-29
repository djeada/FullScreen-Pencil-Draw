/**
 * @file test_canvas_interaction.cpp
 * @brief Interaction tests for Canvas selection and dragging.
 */
#include "../src/widgets/canvas.h"
#include "../src/widgets/transform_handle_item.h"
#include "../src/windows/main_window.h"
#include <QApplication>
#include <QGraphicsRectItem>
#include <QTest>

namespace {
bool hasTransformHandle(QGraphicsScene *scene) {
  if (!scene)
    return false;
  for (QGraphicsItem *item : scene->items()) {
    if (item && item->type() == TransformHandleItem::Type)
      return true;
  }
  return false;
}
} // namespace

class TestCanvasInteraction : public QObject {
  Q_OBJECT

private slots:
  void selectionClickSelectsItemAndShowsHandles() {
    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    auto *rect = new QGraphicsRectItem(QRectF(0, 0, 120, 80));
    rect->setFlag(QGraphicsItem::ItemIsSelectable, true);
    rect->setFlag(QGraphicsItem::ItemIsMovable, true);
    rect->setPos(180, 140);
    canvas.scene()->addItem(rect);

    canvas.setShape("Selection");
    QApplication::processEvents();

    const QPoint viewPos =
        canvas.mapFromScene(rect->sceneBoundingRect().center());
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      viewPos);

    QVERIFY(rect->isSelected());
    QVERIFY(hasTransformHandle(canvas.scene()));
  }

  void selectionDragMovesItem() {
    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    auto *rect = new QGraphicsRectItem(QRectF(0, 0, 120, 80));
    rect->setFlag(QGraphicsItem::ItemIsSelectable, true);
    rect->setFlag(QGraphicsItem::ItemIsMovable, true);
    rect->setPos(220, 180);
    canvas.scene()->addItem(rect);

    canvas.setShape("Selection");
    QApplication::processEvents();

    const QPoint startPos =
        canvas.mapFromScene(rect->sceneBoundingRect().center());
    const QPoint endPos = startPos + QPoint(36, 24);
    const QPointF oldPos = rect->pos();

    QTest::mousePress(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      startPos);
    QTest::mouseMove(canvas.viewport(), endPos, 20);
    QTest::mouseRelease(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                        endPos);

    QVERIFY(rect->isSelected());
    QVERIFY(QLineF(oldPos, rect->pos()).length() > 1.0);
  }

  void mainWindowSelectionModeStillSelectsAndDrags() {
    auto *window = new MainWindow;
    window->resize(1200, 800);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *canvas = window->findChild<Canvas *>();
    QVERIFY(canvas);

    canvas->setShape("Selection");
    QApplication::processEvents();

    auto *rect = new QGraphicsRectItem(QRectF(0, 0, 120, 80));
    rect->setFlag(QGraphicsItem::ItemIsSelectable, true);
    rect->setFlag(QGraphicsItem::ItemIsMovable, true);
    rect->setPos(260, 210);
    canvas->scene()->addItem(rect);

    const QPoint startPos =
        canvas->mapFromScene(rect->sceneBoundingRect().center());
    const QPoint endPos = startPos + QPoint(28, 18);
    const QPointF oldPos = rect->pos();

    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier,
                      startPos);
    QVERIFY(rect->isSelected());

    QTest::mousePress(canvas->viewport(), Qt::LeftButton, Qt::NoModifier,
                      startPos);
    QTest::mouseMove(canvas->viewport(), endPos, 20);
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, Qt::NoModifier,
                        endPos);

    QVERIFY(rect->isSelected());
    QVERIFY(QLineF(oldPos, rect->pos()).length() > 1.0);
  }
};

QTEST_MAIN(TestCanvasInteraction)
#include "test_canvas_interaction.moc"
