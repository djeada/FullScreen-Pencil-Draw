/**
 * @file test_project_serializer.cpp
 * @brief Unit tests for ProjectSerializer class.
 *
 * Tests cover:
 * - Round-trip save/load of projects with various item types
 * - Preservation of layer properties (name, visibility, locked, opacity)
 * - Preservation of canvas properties (scene rect, background color)
 * - Pen and brush serialization
 * - Transform serialization
 * - Error handling for invalid files
 */
#include "../src/core/item_id.h"
#include "../src/core/item_store.h"
#include "../src/core/layer.h"
#include "../src/core/project_serializer.h"
#include "../src/core/scene_controller.h"
#include <QDir>
#include <QFile>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestProjectSerializer : public QObject {
  Q_OBJECT

private slots:
  void testSaveAndLoadEmptyProject() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    // Save
    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);
    QVERIFY(QFile::exists(filePath));

    // Load into fresh state
    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);
    QCOMPARE(loadedRect, sceneRect);
    QCOMPARE(loadedBg, bgColor);
    QCOMPARE(manager2.layerCount(), 1);
  }

  void testSaveAndLoadWithRectItem() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_rect.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    // Add a rect item
    auto *rect = new QGraphicsRectItem(10, 20, 100, 50);
    rect->setPen(QPen(QColor(255, 0, 0), 3));
    rect->setBrush(QBrush(QColor(0, 255, 0)));
    rect->setPos(30, 40);
    ItemId id = store.registerItem(rect);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor("#ff00ff00");

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    // Load
    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    // Verify the layer has 1 item
    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 1);

    // Verify it's a rect item
    QGraphicsItem *item = store2.item(layer->itemIds().first());
    QVERIFY(item);
    auto *loadedRect2 = dynamic_cast<QGraphicsRectItem *>(item);
    QVERIFY(loadedRect2);
    QCOMPARE(loadedRect2->rect(), QRectF(10, 20, 100, 50));
    QCOMPARE(loadedRect2->pos(), QPointF(30, 40));
    QCOMPARE(loadedRect2->pen().color(), QColor(255, 0, 0));
    QVERIFY(qFuzzyCompare(loadedRect2->pen().widthF(), 3.0));
  }

  void testSaveAndLoadWithPathItem() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_path.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    // Create a path
    QPainterPath path;
    path.moveTo(0, 0);
    path.lineTo(100, 50);
    path.lineTo(50, 100);
    auto *pathItem = new QGraphicsPathItem(path);
    pathItem->setPen(QPen(Qt::blue, 2));
    ItemId id = store.registerItem(pathItem);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    // Load
    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 1);

    QGraphicsItem *item = store2.item(layer->itemIds().first());
    QVERIFY(dynamic_cast<QGraphicsPathItem *>(item));
  }

  void testSaveAndLoadWithMultipleLayers() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_layers.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    // Layer 0 (default "Background") - add a rect
    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    ItemId id1 = store.registerItem(rect);
    manager.activeLayer()->addItem(id1, &store);

    // Create Layer 1
    Layer *layer2 = manager.createLayer("Drawing", Layer::Type::Vector);
    layer2->setOpacity(0.5);
    layer2->setVisible(false);

    // Add line to layer 1
    manager.setActiveLayer(1);
    auto *line = new QGraphicsLineItem(0, 0, 200, 200);
    ItemId id2 = store.registerItem(line);
    layer2->addItem(id2, &store);

    QRectF sceneRect(0, 0, 1920, 1080);
    QColor bgColor(Qt::black);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    // Load
    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    QCOMPARE(manager2.layerCount(), 2);
    QCOMPARE(manager2.activeLayerIndex(), 1);

    // Check layer 0
    Layer *l0 = manager2.layer(0);
    QVERIFY(l0);
    QCOMPARE(l0->name(), "Background");
    QCOMPARE(l0->itemCount(), 1);
    QVERIFY(l0->isVisible());

    // Check layer 1
    Layer *l1 = manager2.layer(1);
    QVERIFY(l1);
    QCOMPARE(l1->name(), "Drawing");
    QCOMPARE(l1->itemCount(), 1);
    QVERIFY(!l1->isVisible());
    QVERIFY(qFuzzyCompare(l1->opacity(), 0.5));

    // Verify item types
    QVERIFY(
        dynamic_cast<QGraphicsRectItem *>(store2.item(l0->itemIds().first())));
    QVERIFY(
        dynamic_cast<QGraphicsLineItem *>(store2.item(l1->itemIds().first())));
  }

  void testSaveAndLoadEllipseItem() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_ellipse.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    auto *ellipse = new QGraphicsEllipseItem(10, 20, 80, 60);
    ellipse->setPen(QPen(Qt::green, 1));
    ItemId id = store.registerItem(ellipse);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 1);

    auto *loadedEllipse = dynamic_cast<QGraphicsEllipseItem *>(
        store2.item(layer->itemIds().first()));
    QVERIFY(loadedEllipse);
    QCOMPARE(loadedEllipse->rect(), QRectF(10, 20, 80, 60));
  }

  void testSaveAndLoadTextItem() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_text.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    auto *text = new QGraphicsTextItem("Hello World");
    text->setDefaultTextColor(Qt::red);
    text->setPos(100, 200);
    ItemId id = store.registerItem(text);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 1);

    auto *loadedText = dynamic_cast<QGraphicsTextItem *>(
        store2.item(layer->itemIds().first()));
    QVERIFY(loadedText);
    QCOMPARE(loadedText->pos(), QPointF(100, 200));
    QCOMPARE(loadedText->defaultTextColor(), QColor(Qt::red));
  }

  void testLoadInvalidFile() {
    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);
    QRectF loadedRect;
    QColor loadedBg;

    // Non-existent file
    bool loaded =
        ProjectSerializer::loadProject("/nonexistent/path.fspd", &scene, &store,
                                       &manager, loadedRect, loadedBg);
    QVERIFY(!loaded);
  }

  void testLoadCorruptedFile() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/corrupt.fspd";

    // Write invalid JSON
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("not valid json");
    file.close();

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene, &store, &manager, loadedRect, loadedBg);
    QVERIFY(!loaded);
  }

  void testSaveWithNullParameters() {
    bool saved = ProjectSerializer::saveProject(
        "/tmp/test.fspd", nullptr, nullptr, nullptr, QRectF(), QColor());
    QVERIFY(!saved);
  }

  void testLoadWithNullParameters() {
    QRectF r;
    QColor c;
    bool loaded = ProjectSerializer::loadProject("/tmp/test.fspd", nullptr,
                                                 nullptr, nullptr, r, c);
    QVERIFY(!loaded);
  }

  void testFileFilter() {
    QString filter = ProjectSerializer::fileFilter();
    QVERIFY(filter.contains("fspd"));
  }

  void testSaveAndLoadPreservesTransform() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_transform.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    QTransform t;
    t.scale(2.0, 3.0);
    rect->setTransform(t);
    ItemId id = store.registerItem(rect);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 1);

    QGraphicsItem *item = store2.item(layer->itemIds().first());
    QVERIFY(item);
    QTransform loaded_t = item->transform();
    QVERIFY(qFuzzyCompare(loaded_t.m11(), 2.0));
    QVERIFY(qFuzzyCompare(loaded_t.m22(), 3.0));
  }

  void testSaveAndLoadLinearGradientBrush() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_lg.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    // Create rect with linear gradient brush
    auto *rect = new QGraphicsRectItem(0, 0, 100, 100);
    QLinearGradient lg(0, 0, 1, 1);
    lg.setCoordinateMode(QGradient::ObjectBoundingMode);
    lg.setColorAt(0, QColor(Qt::red));
    lg.setColorAt(1, QColor(Qt::blue));
    rect->setBrush(QBrush(lg));
    rect->setPen(QPen(Qt::black));
    ItemId id = store.registerItem(rect);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    // Load
    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    QCOMPARE(layer->itemCount(), 1);

    auto *loadedRect2 = dynamic_cast<QGraphicsRectItem *>(
        store2.item(layer->itemIds().first()));
    QVERIFY(loadedRect2);

    const QBrush &loadedBrush = loadedRect2->brush();
    QVERIFY(loadedBrush.gradient() != nullptr);
    QCOMPARE(loadedBrush.gradient()->type(), QGradient::LinearGradient);
    QCOMPARE(loadedBrush.gradient()->stops().size(), 2);
    QCOMPARE(loadedBrush.gradient()->coordinateMode(),
             QGradient::ObjectBoundingMode);
  }

  void testSaveAndLoadRadialGradientBrush() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_rg.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    auto *ellipse = new QGraphicsEllipseItem(0, 0, 80, 80);
    QRadialGradient rg(0.5, 0.5, 0.5);
    rg.setCoordinateMode(QGradient::ObjectBoundingMode);
    rg.setColorAt(0, QColor(Qt::yellow));
    rg.setColorAt(1, QColor(Qt::green));
    ellipse->setBrush(QBrush(rg));
    ItemId id = store.registerItem(ellipse);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    auto *loadedEllipse = dynamic_cast<QGraphicsEllipseItem *>(
        store2.item(layer->itemIds().first()));
    QVERIFY(loadedEllipse);

    const QBrush &loadedBrush = loadedEllipse->brush();
    QVERIFY(loadedBrush.gradient() != nullptr);
    QCOMPARE(loadedBrush.gradient()->type(), QGradient::RadialGradient);
    QCOMPARE(loadedBrush.gradient()->stops().size(), 2);
  }

  void testSaveAndLoadPatternBrush() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_pattern.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    auto *rect = new QGraphicsRectItem(0, 0, 50, 50);
    rect->setBrush(QBrush(QColor(Qt::cyan), Qt::CrossPattern));
    ItemId id = store.registerItem(rect);
    manager.activeLayer()->addItem(id, &store);

    QRectF sceneRect(0, 0, 800, 600);
    QColor bgColor(Qt::white);

    bool saved = ProjectSerializer::saveProject(filePath, &scene, &store,
                                                &manager, sceneRect, bgColor);
    QVERIFY(saved);

    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF loadedRect;
    QColor loadedBg;

    bool loaded = ProjectSerializer::loadProject(
        filePath, &scene2, &store2, &manager2, loadedRect, loadedBg);
    QVERIFY(loaded);

    Layer *layer = manager2.layer(0);
    QVERIFY(layer);
    auto *loadedRect2 = dynamic_cast<QGraphicsRectItem *>(
        store2.item(layer->itemIds().first()));
    QVERIFY(loadedRect2);
    QCOMPARE(loadedRect2->brush().style(), Qt::CrossPattern);
    QCOMPARE(loadedRect2->brush().color(), QColor(Qt::cyan));
  }
};

QTEST_MAIN(TestProjectSerializer)
#include "test_project_serializer.moc"
