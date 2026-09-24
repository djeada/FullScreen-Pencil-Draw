/**
 * @file test_canvas_interaction.cpp
 * @brief Interaction tests for Canvas selection and dragging.
 */
#include "../src/core/auto_save_manager.h"
#include "../src/core/fill_utils.h"
#include "../src/core/item_store.h"
#include "../src/core/project_serializer.h"
#include "../src/core/raster_surface.h"
#include "../src/widgets/canvas.h"
#include "../src/widgets/latex_text_item.h"
#include "../src/widgets/layer_panel.h"
#include "../src/widgets/raster_layer_item.h"
#include "../src/widgets/transform_handle_item.h"
#include "../src/windows/main_window.h"
#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QScrollArea>
#include <QSignalSpy>
#include <QStandardPaths>
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
// Press, move (with the button held) and release in scene coordinates.
void dragScene(Canvas &canvas, const QPointF &from, const QPointF &to,
               int steps = 10) {
  const QPoint a = canvas.mapFromScene(from);
  const QPoint b = canvas.mapFromScene(to);
  QTest::mousePress(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, a);
  for (int i = 1; i <= steps; ++i) {
    const QPoint p = a + (b - a) * i / steps;
    QMouseEvent move(QEvent::MouseMove, QPointF(p),
                     QPointF(canvas.viewport()->mapToGlobal(p)), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(canvas.viewport(), &move);
  }
  QTest::mouseRelease(canvas.viewport(), Qt::LeftButton, Qt::NoModifier, b);
  QApplication::processEvents();
}

QGraphicsPixmapItem *addOpaqueImage(Canvas &canvas, const QRectF &rect,
                                    const QColor &color = Qt::red) {
  QImage image(rect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
  image.fill(color);
  auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
  item->setPos(rect.topLeft());
  canvas.registerItem(item);
  return item;
}

int alphaAt(QGraphicsPixmapItem *item, const QPointF &scenePos) {
  const QPointF local = item->mapFromScene(scenePos) - item->offset();
  return qAlpha(item->pixmap().toImage().pixel(local.toPoint()));
}

bool inScene(Canvas &canvas, QGraphicsItem *item) {
  return canvas.scene()->items().contains(item);
}

void showCanvas(Canvas &canvas) {
  canvas.resize(900, 700);
  canvas.show();
  QVERIFY(QTest::qWaitForWindowExposed(&canvas));
  canvas.scene()->setSceneRect(0, 0, 800, 600);
  canvas.centerOn(400, 300);
}
} // namespace

class TestCanvasInteraction : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    // Keep settings and recovery snapshots away from the user's real ones
    // (a leftover recovery copy would otherwise open a modal prompt when a
    // test creates a MainWindow).
    QStandardPaths::setTestModeEnabled(true);
  }

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

  // ---- Object Eraser vs Pixel Eraser (#167) ----

  void objectEraserRemovesOnlyTheTouchedObjectAndUndoRestoresIt() {
    Canvas canvas;
    showCanvas(canvas);
    auto *a = new QGraphicsRectItem(0, 0, 60, 60);
    a->setBrush(Qt::blue);
    a->setPos(100, 100);
    auto *b = new QGraphicsRectItem(0, 0, 60, 60);
    b->setBrush(Qt::green);
    b->setPos(200, 100);
    auto *c = new QGraphicsRectItem(0, 0, 60, 60);
    c->setPos(300, 100);
    canvas.registerItem(a);
    canvas.registerItem(b);
    canvas.registerItem(c);
    Layer *layer = canvas.layerManager()->activeLayer();
    const QList<ItemId> order = layer->itemIds();

    canvas.setEraserTool();
    QVERIFY(canvas.isObjectEraserActive());
    // One drag across b's interior (and nothing else).
    dragScene(canvas, QPointF(215, 130), QPointF(245, 130));
    QVERIFY(!inScene(canvas, b));
    QVERIFY(inScene(canvas, a));
    QVERIFY(inScene(canvas, c));

    // The sweep is one undo step, restoring b with its layer and order.
    canvas.undoLastAction();
    QVERIFY(inScene(canvas, b));
    QCOMPARE(canvas.layerManager()->findLayerForItem(b), layer);
    QCOMPARE(layer->itemIds(), order);
  }

  void objectEraserSkipsLockedAndHiddenLayers() {
    Canvas canvas;
    showCanvas(canvas);
    LayerManager *layers = canvas.layerManager();
    Layer *locked = layers->createLayer("Locked");
    layers->setActiveLayer(1);
    auto *onLocked = new QGraphicsRectItem(0, 0, 80, 80);
    onLocked->setBrush(Qt::black);
    onLocked->setPos(100, 100);
    canvas.registerItem(onLocked);
    locked->setLocked(true);

    Layer *hidden = layers->createLayer("Hidden");
    layers->setActiveLayer(2);
    auto *onHidden = new QGraphicsRectItem(0, 0, 80, 80);
    onHidden->setBrush(Qt::black);
    onHidden->setPos(100, 100);
    canvas.registerItem(onHidden);
    hidden->setVisible(false);

    canvas.setEraserTool();
    dragScene(canvas, QPointF(110, 140), QPointF(170, 140));
    QVERIFY(inScene(canvas, onLocked));
    QVERIFY(inScene(canvas, onHidden));
  }

  void objectEraserNeedsVisiblePixelsOfAnImage() {
    Canvas canvas;
    showCanvas(canvas);
    QImage image(200, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
      QPainter p(&image);
      p.fillRect(QRect(0, 0, 20, 20), Qt::black);
    }
    auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    item->setPos(100, 100);
    canvas.registerItem(item);
    canvas.setEraserTool();
    // Inside the bounding box but over transparent pixels only.
    dragScene(canvas, QPointF(180, 200), QPointF(220, 200));
    QVERIFY(inScene(canvas, item));
    // Over the visible corner.
    dragScene(canvas, QPointF(105, 110), QPointF(115, 110));
    QVERIFY(!inScene(canvas, item));
  }

  void pixelEraserMakesPartialTransparencyAndIsOneUndoStep() {
    Canvas canvas;
    showCanvas(canvas);
    QGraphicsPixmapItem *image =
        addOpaqueImage(canvas, QRectF(100, 100, 200, 200));
    auto *vector = new QGraphicsRectItem(0, 0, 200, 40);
    vector->setBrush(Qt::blue);
    vector->setPos(100, 180);
    canvas.registerItem(vector);

    canvas.setPixelEraserTool();
    QVERIFY(canvas.isPixelEraserActive());
    QVERIFY(!canvas.isObjectEraserActive());
    dragScene(canvas, QPointF(140, 200), QPointF(260, 200));

    // The image keeps existing; only the touched pixels became transparent.
    QVERIFY(inScene(canvas, image));
    QVERIFY(inScene(canvas, vector)); // vector objects are never touched
    QCOMPARE(alphaAt(image, QPointF(200, 200)), 0);
    QCOMPARE(alphaAt(image, QPointF(200, 120)), 255);

    canvas.undoLastAction();
    QCOMPARE(alphaAt(image, QPointF(200, 200)), 255);
    canvas.redoLastAction();
    QCOMPARE(alphaAt(image, QPointF(200, 200)), 0);
  }

  void pixelEraserRespectsTheColourSelection() {
    Canvas canvas;
    showCanvas(canvas);
    QImage image(200, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::red);
    {
      QPainter p(&image);
      p.fillRect(QRect(100, 0, 100, 100), Qt::blue);
    }
    auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    item->setPos(100, 100);
    canvas.registerItem(item);
    // Select the red half by colour, then erase across both halves.
    canvas.setShape("ColorSelect");
    QTest::mouseClick(canvas.viewport(), Qt::LeftButton, Qt::NoModifier,
                      canvas.mapFromScene(QPointF(120, 150)));
    QVERIFY(canvas.hasActiveColorSelection());
    canvas.setPixelEraserTool();
    // Let the double-click interval pass so the next press is a press.
    QTest::qWait(QApplication::doubleClickInterval() + 50);
    dragScene(canvas, QPointF(130, 150), QPointF(270, 150));
    QCOMPARE(alphaAt(item, QPointF(150, 150)), 0);   // selected: erased
    QCOMPARE(alphaAt(item, QPointF(250, 150)), 255); // unselected: kept
  }

  void pixelEraserLeavesVectorOnlyLayersAloneAndSaysWhy() {
    Canvas canvas;
    showCanvas(canvas);
    auto *vector = new QGraphicsRectItem(0, 0, 100, 100);
    vector->setBrush(Qt::blue);
    vector->setPos(100, 100);
    canvas.registerItem(vector);
    QSignalSpy hint(&canvas, &Canvas::statusMessage);
    canvas.setPixelEraserTool();
    dragScene(canvas, QPointF(120, 150), QPointF(180, 150));
    QVERIFY(inScene(canvas, vector));
    QCOMPARE(hint.count(), 1);
  }

  void rasterLayerPaintAndEraseSurviveSaveAndReopen() {
    QTemporaryDir dir;
    const QString path = dir.filePath("hybrid.fspd");
    {
      Canvas canvas;
      showCanvas(canvas);
      auto *vector = new QGraphicsRectItem(0, 0, 50, 50);
      vector->setPos(500, 100);
      canvas.registerItem(vector);
      LayerManager *layers = canvas.layerManager();
      layers->createLayer("Paint", Layer::Type::Raster);
      layers->setActiveLayer(1);

      canvas.setPenTool();
      dragScene(canvas, QPointF(100, 300), QPointF(400, 300));
      RasterLayerItem *raster = findItem<RasterLayerItem>(canvas.scene());
      QVERIFY(raster);
      QCOMPARE(qAlpha(raster->surface().pixel(QPoint(250, 300))), 255);
      QCOMPARE(countItems<QGraphicsPathItem>(canvas.scene()), 0);

      canvas.setPixelEraserTool();
      dragScene(canvas, QPointF(240, 290), QPointF(260, 310), 4);
      QCOMPARE(qAlpha(raster->surface().pixel(QPoint(250, 300))), 0);
      QCOMPARE(qAlpha(raster->surface().pixel(QPoint(120, 300))), 255);
      QVERIFY(inScene(canvas, raster)); // the layer content stays

      // Object Eraser never deletes raster layer content.
      canvas.setEraserTool();
      dragScene(canvas, QPointF(110, 300), QPointF(130, 300));
      QVERIFY(inScene(canvas, raster));

      // Undo the pixel erase, redo it: one step each.
      canvas.undoLastAction();
      QCOMPARE(qAlpha(raster->surface().pixel(QPoint(250, 300))), 255);
      canvas.redoLastAction();
      QCOMPARE(qAlpha(raster->surface().pixel(QPoint(250, 300))), 0);

      QSignalSpy saved(&canvas, &Canvas::documentSaved);
      QVERIFY(canvas.saveProjectTo(path, /*interactive=*/false));
      QCOMPARE(saved.count(), 1);
      QCOMPARE(canvas.currentFilePath(), path);
    }
    Canvas reopened;
    QSignalSpy loaded(&reopened, &Canvas::documentLoaded);
    QVERIFY(reopened.loadProjectFile(path, false));
    QCOMPARE(loaded.count(), 1);
    QCOMPARE(reopened.layerManager()->layer(1)->type(), Layer::Type::Raster);
    RasterLayerItem *raster = findItem<RasterLayerItem>(reopened.scene());
    QVERIFY(raster);
    QCOMPARE(qAlpha(raster->surface().pixel(QPoint(250, 300))), 0);
    QCOMPARE(qAlpha(raster->surface().pixel(QPoint(120, 300))), 255);
    QCOMPARE(countItems<QGraphicsRectItem>(reopened.scene()), 1);
  }

  void recoveredDocumentsOpenAsUnsavedWithoutAFileName() {
    QTemporaryDir dir;
    const QString path = dir.filePath("snapshot.fspd");
    {
      Canvas source;
      source.registerItem(new QGraphicsRectItem(0, 0, 10, 10));
      QVERIFY(source.saveProjectTo(path, false));
    }
    Canvas canvas;
    QSignalSpy recovered(&canvas, &Canvas::documentRecovered);
    QSignalSpy loaded(&canvas, &Canvas::documentLoaded);
    QVERIFY(canvas.loadProjectFile(path, false, /*recovered=*/true));
    QCOMPARE(recovered.count(), 1);
    QCOMPARE(loaded.count(), 0);
    // Saving must ask where instead of overwriting the snapshot/original.
    QVERIFY(canvas.currentFilePath().isEmpty());
  }

  void failedSaveKeepsTheDocumentModified() {
    Canvas canvas;
    canvas.registerItem(new QGraphicsRectItem(0, 0, 10, 10));
    QSignalSpy saved(&canvas, &Canvas::documentSaved);
    QSignalSpy failed(&canvas, &Canvas::saveFailed);
    QVERIFY(!canvas.saveProjectTo("/nonexistent-dir-for-fspd/x.fspd", false));
    QCOMPARE(saved.count(), 0);
    QCOMPARE(failed.count(), 1);
    QVERIFY(canvas.currentFilePath().isEmpty());
  }

  // ---- Autosave / recovery lifecycle (#165) ----

  void autosaveRecoversAnEditableDocumentAfterACrash() {
    QTemporaryDir recoveryDir;
    QString crashedSession;
    {
      Canvas canvas;
      auto *rect = new QGraphicsRectItem(0, 0, 40, 40);
      canvas.registerItem(rect);
      canvas.layerManager()->createLayer("Paint", Layer::Type::Raster);
      AutoSaveManager autosave(&canvas, nullptr, recoveryDir.path());
      autosave.setShouldSaveCheck([] { return true; });
      QVERIFY(autosave.performAutoSave());
      QVERIFY(autosave.hasAutoSave());
      crashedSession = autosave.documentId();
      // Another document session never overwrites this one's snapshot.
      Canvas other;
      AutoSaveManager otherAutosave(&other, nullptr, recoveryDir.path());
      otherAutosave.setShouldSaveCheck([] { return true; });
      QVERIFY(otherAutosave.performAutoSave());
      QVERIFY(otherAutosave.autoSavePath() != autosave.autoSavePath());
      otherAutosave.clearAutoSave(); // this one closes cleanly
      QVERIFY(!otherAutosave.hasAutoSave());
    } // the first "crashes": its snapshot stays on disk

    // Next launch.
    Canvas canvas;
    AutoSaveManager autosave(&canvas, nullptr, recoveryDir.path());
    const QList<RecoverySnapshot> found = autosave.recoverableSnapshots();
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().documentId, crashedSession);
    QCOMPARE(found.first().layerCount, 2);
    QVERIFY(autosave.recoverSnapshot(found.first()));
    QCOMPARE(canvas.layerManager()->layerCount(), 2);
    QCOMPARE(canvas.layerManager()->layer(1)->type(), Layer::Type::Raster);
    QCOMPARE(countItems<QGraphicsRectItem>(canvas.scene()), 1);
    // Recovered work is unsaved: no file name, and its snapshot is kept and
    // continued until the user saves or discards.
    QVERIFY(canvas.currentFilePath().isEmpty());
    QCOMPARE(autosave.documentId(), crashedSession);
    QVERIFY(autosave.hasAutoSave());
    // A clean close (after saving) removes it.
    autosave.clearAutoSave();
    QVERIFY(autosave.recoverableSnapshots().isEmpty());
  }
};

QTEST_MAIN(TestCanvasInteraction)
#include "test_canvas_interaction.moc"
