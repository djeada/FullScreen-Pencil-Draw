/**
 * @file test_canvas_interaction.cpp
 * @brief Interaction tests for Canvas selection and dragging.
 */
#include "../src/core/fill_utils.h"
#include "../src/core/project_serializer.h"
#include "../src/widgets/canvas.h"
#include "../src/widgets/latex_text_item.h"
#include "../src/widgets/layer_panel.h"
#include "../src/widgets/transform_handle_item.h"
#include "../src/windows/main_window.h"
#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QTest>

namespace {
template <typename T> int countItems(QGraphicsScene *scene) {
  int count = 0;
  if (!scene)
    return count;
  for (QGraphicsItem *item : scene->items()) {
    if (dynamic_cast<T *>(item))
      ++count;
  }
  return count;
}

// Visible-only variant: the canvas keeps a permanently hidden eraser-preview
// ellipse in the scene, which must not be mistaken for a leaked marker.
template <typename T> int countVisibleItems(QGraphicsScene *scene) {
  int count = 0;
  if (!scene)
    return count;
  for (QGraphicsItem *item : scene->items()) {
    if (dynamic_cast<T *>(item) && item->isVisible())
      ++count;
  }
  return count;
}

template <typename T> T *findItem(QGraphicsScene *scene) {
  if (!scene)
    return nullptr;
  for (QGraphicsItem *item : scene->items()) {
    if (auto *typed = dynamic_cast<T *>(item))
      return typed;
  }
  return nullptr;
}

TransformHandleItem *findTransformHandle(QGraphicsScene *scene) {
  if (!scene)
    return nullptr;
  for (QGraphicsItem *item : scene->items()) {
    if (item && item->type() == TransformHandleItem::Type)
      return static_cast<TransformHandleItem *>(item);
  }
  return nullptr;
}

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
    MainWindow window;
    window.resize(1200, 800);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *canvas = window.findChild<Canvas *>();
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

  // --- Bezier tool on the canvas -----------------------------------------

  void bezierToolCommitsPathOnDoubleClick() {
    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setBezierTool();
    QApplication::processEvents();

    const QPoint a(150, 150);
    const QPoint b(300, 220);
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, a);
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, b);
    QTest::mouseDClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, b);
    QApplication::processEvents();

    auto *path = findItem<QGraphicsPathItem>(canvas.scene());
    QVERIFY2(path, "double-click should commit the bezier path");
    QVERIFY(path->path().elementCount() >= 2);
    // Anchor markers are preview-only and must not survive finalization.
    QCOMPARE(countVisibleItems<QGraphicsEllipseItem>(canvas.scene()), 0);
  }

  void bezierToolEscapeDiscardsPath() {
    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setBezierTool();
    QApplication::processEvents();

    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(160, 160));
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(280, 240));
    QApplication::processEvents();

    QVERIFY(canvas.cancelActiveGesture());
    QApplication::processEvents();

    QCOMPARE(countItems<QGraphicsPathItem>(canvas.scene()), 0);
    QCOMPARE(countVisibleItems<QGraphicsEllipseItem>(canvas.scene()), 0);
  }

  void bezierToolSwitchingToolLeavesNoPreviewItems() {
    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setBezierTool();
    QApplication::processEvents();

    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(170, 170));
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(290, 250));
    QApplication::processEvents();

    // Switching tools finalizes the path; the markers must be cleaned up.
    canvas.setPenTool();
    QApplication::processEvents();

    QCOMPARE(countVisibleItems<QGraphicsEllipseItem>(canvas.scene()), 0);
  }

  void loadingADocumentMidGestureDropsPreviewItems() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString projectPath = dir.filePath("doc.fspd");

    // A minimal saved document to load over the in-progress gesture.
    {
      Canvas source;
      auto *rect = new QGraphicsRectItem(QRectF(0, 0, 40, 40));
      source.scene()->addItem(rect);
      QVERIFY(ProjectSerializer::saveProject(
          projectPath, source.scene(), source.itemStore(),
          source.layerManager(), source.scene()->sceneRect(), Qt::white));
    }

    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setBezierTool();
    QApplication::processEvents();
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(180, 180));
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(300, 260));
    QApplication::processEvents();
    QCOMPARE(countVisibleItems<QGraphicsEllipseItem>(canvas.scene()), 2);

    // Replacing the document must not leave the tool holding pointers to
    // items the load is about to delete.
    canvas.openRecentFile(projectPath);
    QApplication::processEvents();

    QCOMPARE(countVisibleItems<QGraphicsEllipseItem>(canvas.scene()), 0);

    // The tool must be usable again afterwards, without touching stale state.
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(200, 200));
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(320, 280));
    QTest::mouseDClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                       QPoint(320, 280));
    QApplication::processEvents();
    QVERIFY(findItem<QGraphicsPathItem>(canvas.scene()));
  }

  // --- Inline text editing ------------------------------------------------

  void escapeCancelRemovesEmptyTextItem() {
    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    canvas.setTextTool();
    QApplication::processEvents();

    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(240, 200));
    QApplication::processEvents();

    auto *textItem = findItem<LatexTextItem>(canvas.scene());
    QVERIFY2(textItem, "clicking with the text tool should create an item");
    QVERIFY(textItem->isEditing());

    auto *proxy = findItem<QGraphicsProxyWidget>(canvas.scene());
    QVERIFY(proxy && proxy->widget());
    QTest::keyClick(proxy->widget(), Qt::Key_Escape);

    QApplication::processEvents();
    QTest::qWait(50);
    QApplication::processEvents();

    QCOMPARE(countItems<LatexTextItem>(canvas.scene()), 0);
  }

  // --- Transform handles --------------------------------------------------

  void transformHandlesShrinkInSceneUnitsWhenZoomingIn() {
    Canvas canvas;
    canvas.resize(800, 600);
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    auto *rect = new QGraphicsRectItem(QRectF(0, 0, 120, 80));
    rect->setFlag(QGraphicsItem::ItemIsSelectable, true);
    rect->setFlag(QGraphicsItem::ItemIsMovable, true);
    rect->setPos(200, 160);
    canvas.scene()->addItem(rect);

    canvas.setShape("Selection");
    QApplication::processEvents();

    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      canvas.mapFromScene(rect->sceneBoundingRect().center()));
    QApplication::processEvents();

    TransformHandleItem *handle = findTransformHandle(canvas.scene());
    QVERIFY(handle);
    const QRectF before = handle->boundingRect();

    canvas.zoomIn();
    canvas.zoomIn();
    QApplication::processEvents();

    // Handles keep a constant on-screen size, so their scene-space footprint
    // must shrink as the view scales up.
    const QRectF after = handle->boundingRect();
    QVERIFY2(after.width() < before.width(),
             "handle bounds should shrink in scene units after zooming in");
    QVERIFY(after.height() < before.height());
  }

  // --- Layer panel --------------------------------------------------------

  void layerPanelContentStaysReachableWhenTheWindowIsShort() {
    MainWindow window;
    window.resize(1000, 400); // too short for the full layer panel content
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *layerPanel = window.findChild<LayerPanel *>();
    QVERIFY(layerPanel);
    auto *scrollArea = layerPanel->findChild<QScrollArea *>();
    QVERIFY2(scrollArea, "layer panel contents must live in a scroll area");
    QVERIFY(scrollArea->widgetResizable());
    QVERIFY(scrollArea->widget());

    QApplication::processEvents();
    QTest::qWait(50);
    QApplication::processEvents();

    // The controls below the tree (opacity, blend mode, buttons) are taller
    // than the dock here, so they must be reachable by scrolling.
    QVERIFY2(scrollArea->widget()->sizeHint().height() > 0,
             "scrolled contents should report a size hint");
    QVERIFY2(scrollArea->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff,
             "vertical scrolling must not be disabled");
  }

  // --- Fill ---------------------------------------------------------------

  void fillRecolorsPressureStrokeBrush() {
    QGraphicsScene scene;

    // Pressure strokes are filled outlines drawn with Qt::NoPen.
    QPainterPath outline;
    outline.addRect(QRectF(0, 0, 100, 60));
    auto *stroke = new QGraphicsPathItem(outline);
    stroke->setPen(Qt::NoPen);
    stroke->setBrush(QBrush(Qt::red));
    scene.addItem(stroke);

    const bool filled =
        fillTopItemAtPoint(&scene, QPointF(50, 30), QBrush(Qt::blue), nullptr,
                           nullptr, nullptr, [](std::unique_ptr<Action>) {});

    QVERIFY(filled);
    QCOMPARE(stroke->brush().color(), QColor(Qt::blue));
    // The invisible pen must stay invisible.
    QCOMPARE(stroke->pen().style(), Qt::NoPen);
  }

  void fillRecolorsStrokedPathPen() {
    QGraphicsScene scene;

    QPainterPath line;
    line.moveTo(0, 0);
    line.lineTo(100, 0);
    auto *stroked = new QGraphicsPathItem(line);
    stroked->setPen(QPen(Qt::red, 8));
    stroked->setBrush(Qt::NoBrush);
    scene.addItem(stroked);

    const bool filled =
        fillTopItemAtPoint(&scene, QPointF(50, 0), QBrush(Qt::blue), nullptr,
                           nullptr, nullptr, [](std::unique_ptr<Action>) {});

    QVERIFY(filled);
    QCOMPARE(stroked->pen().color(), QColor(Qt::blue));
  }
};

QTEST_MAIN(TestCanvasInteraction)
#include "test_canvas_interaction.moc"
