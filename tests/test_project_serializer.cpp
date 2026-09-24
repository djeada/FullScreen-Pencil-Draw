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
#include "../src/widgets/electronics_elements.h"
#include "../src/widgets/element_factory.h"
#include "../src/widgets/latex_text_item.h"
#include "../src/widgets/text_on_path_item.h"
#include "../src/widgets/wire_item.h"
#include <QDir>
#include <QFile>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QLinearGradient>
#include <QPainter>
#include <QRadialGradient>
#include <QTemporaryDir>
#include <QtTest/QtTest>

// Item type without a dedicated project format (like BrushStrokeItem):
// saved through the raster fallback.
class CustomPaintedItem : public QGraphicsItem {
public:
  QRectF boundingRect() const override { return QRectF(-10, -5, 40, 20); }
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
             QWidget *) override {
    painter->fillRect(boundingRect(), Qt::red);
  }
};

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

  void testSaveAndLoadLatexTextItem() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_latex.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    auto *latexItem = new LatexTextItem();
    latexItem->setText("Hello $x^2$");
    latexItem->setTextColor(QColor(Qt::blue));
    latexItem->setFont(QFont("Arial", 16));
    latexItem->setPos(50, 75);
    ItemId id = store.registerItem(latexItem);
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

    auto *loadedLatex =
        dynamic_cast<LatexTextItem *>(store2.item(layer->itemIds().first()));
    QVERIFY(loadedLatex);
    QCOMPARE(loadedLatex->text(), "Hello $x^2$");
    QCOMPARE(loadedLatex->textColor(), QColor(Qt::blue));
    QCOMPARE(loadedLatex->pos(), QPointF(50, 75));
    QCOMPARE(loadedLatex->font().pointSize(), 16);
  }

  void testSaveAndLoadTextOnPathItem() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test_pathtext.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);

    auto *pathTextItem = new TextOnPathItem();
    pathTextItem->setText("Curved Text");
    pathTextItem->setTextColor(QColor(Qt::red));
    pathTextItem->setFont(QFont("Helvetica", 20));
    QPainterPath path;
    path.moveTo(0, 100);
    path.cubicTo(50, 0, 150, 0, 200, 100);
    pathTextItem->setPath(path);
    pathTextItem->setPos(10, 20);
    ItemId id = store.registerItem(pathTextItem);
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

    auto *loadedPathText =
        dynamic_cast<TextOnPathItem *>(store2.item(layer->itemIds().first()));
    QVERIFY(loadedPathText);
    QCOMPARE(loadedPathText->text(), "Curved Text");
    QCOMPARE(loadedPathText->textColor(), QColor(Qt::red));
    QCOMPARE(loadedPathText->pos(), QPointF(10, 20));
    QCOMPARE(loadedPathText->font().pointSize(), 20);
    QVERIFY(loadedPathText->path().elementCount() > 0);
  }
  // Arrows are groups (line + polygon head); they, locked items, element
  // items with their wires and layer blend modes must all survive a save.
  void testSaveAndLoadGroupsElementsWiresAndLocks() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString filePath = tmpDir.path() + "/test_mixed.fspd";

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);
    Layer *layer = manager.activeLayer();
    layer->setBlendMode(Layer::BlendMode::Multiply);

    auto *line = new QGraphicsLineItem(0, 0, 100, 0);
    auto *head = new QGraphicsPolygonItem(
        QPolygonF({QPointF(100, 0), QPointF(90, -5), QPointF(90, 5)}));
    head->setBrush(Qt::red);
    auto *arrow = new QGraphicsItemGroup();
    arrow->addToGroup(line);
    arrow->addToGroup(head);
    arrow->setPos(50, 60);
    layer->addItem(store.registerItem(arrow), &store);

    auto *locked = new QGraphicsRectItem(0, 0, 10, 10);
    locked->setData(0, "locked");
    layer->addItem(store.registerItem(locked), &store);

    QGraphicsItem *r1 = createDiagramElement("resistor");
    QGraphicsItem *r2 = createDiagramElement("resistor");
    QVERIFY(r1 && r2);
    r2->setPos(200, 0);
    layer->addItem(store.registerItem(r1), &store);
    layer->addItem(store.registerItem(r2), &store);
    auto *wire = new WireItem(static_cast<ElectronicsElementItem *>(r1), 1,
                              static_cast<ElectronicsElementItem *>(r2), 0);
    // Save the wire before its elements to exercise deferred resolution.
    layer->addItem(store.registerItem(wire), &store);
    layer->moveItemToBottom(store.idForItem(wire));

    QVERIFY(ProjectSerializer::saveProject(filePath, &scene, &store, &manager,
                                           QRectF(0, 0, 800, 600), Qt::white));

    QGraphicsScene scene2;
    ItemStore store2(&scene2);
    LayerManager manager2(&scene2);
    manager2.setItemStore(&store2);
    QRectF rect;
    QColor bg;
    QVERIFY(ProjectSerializer::loadProject(filePath, &scene2, &store2,
                                           &manager2, rect, bg));

    Layer *loaded = manager2.layer(0);
    QCOMPARE(loaded->blendMode(), Layer::BlendMode::Multiply);
    QCOMPARE(loaded->itemCount(), 5);

    int groups = 0, elements = 0, wires = 0, lockedCount = 0;
    for (QGraphicsItem *item : loaded->items()) {
      if (auto *g = dynamic_cast<QGraphicsItemGroup *>(item)) {
        ++groups;
        QCOMPARE(g->childItems().size(), 2);
        QCOMPARE(g->pos(), QPointF(50, 60));
      } else if (diagramElementId(item) == "resistor") {
        ++elements;
      } else if (auto *w = dynamic_cast<WireItem *>(item)) {
        ++wires;
        QVERIFY(w->sourceElement());
        QVERIFY(w->destElement());
        QCOMPARE(w->sourcePin(), 1);
        QCOMPARE(w->destPin(), 0);
      } else if (item->data(0).toString() == "locked") {
        ++lockedCount;
        QVERIFY(!(item->flags() & QGraphicsItem::ItemIsMovable));
      }
    }
    QCOMPARE(groups, 1);
    QCOMPARE(elements, 2);
    QCOMPARE(wires, 1);
    QCOMPARE(lockedCount, 1);
  }
  // The raster fallback must survive repeated save/load cycles without the
  // bitmap changing size or position.
  void testRasterFallbackIsStableAcrossSaves() {
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QGraphicsScene scene;
    ItemStore store(&scene);
    LayerManager manager(&scene);
    manager.setItemStore(&store);
    auto *custom = new CustomPaintedItem();
    custom->setPos(100, 50);
    manager.activeLayer()->addItem(store.registerItem(custom), &store);
    const QRectF expected = custom->sceneBoundingRect();

    QGraphicsScene *current = &scene;
    ItemStore *currentStore = &store;
    LayerManager *currentManager = &manager;
    std::vector<std::unique_ptr<QGraphicsScene>> scenes;
    std::vector<std::unique_ptr<ItemStore>> stores;
    std::vector<std::unique_ptr<LayerManager>> managers;
    // Without the user's consent an item with no native format is never
    // flattened silently: the save fails and names the item.
    {
      const QString refused = tmpDir.path() + "/refused.fspd";
      ProjectSerializer::SaveStatus status;
      QVERIFY(!ProjectSerializer::saveProject(
          refused, &scene, &store, &manager, QRectF(0, 0, 800, 600), Qt::white,
          ProjectSerializer::SaveOptions(), &status));
      QCOMPARE(status.error, ProjectSerializer::SaveError::UnsupportedContent);
      QCOMPARE(status.unsupportedItems.size(), 1);
      QVERIFY(!QFile::exists(refused));
      QCOMPARE(ProjectSerializer::findUnsupportedItems(&store, &manager).size(),
               1);
    }
    ProjectSerializer::SaveOptions approved;
    approved.allowRasterFallback = true;
    for (int round = 0; round < 2; ++round) {
      const QString path = tmpDir.path() + QString("/raster%1.fspd").arg(round);
      ProjectSerializer::SaveStatus status;
      QVERIFY(ProjectSerializer::saveProject(
          path, current, currentStore, currentManager, QRectF(0, 0, 800, 600),
          Qt::white, approved, &status));
      // Only the first round has a custom item; afterwards it is an image.
      QCOMPARE(status.rasterizedItems.size(), round == 0 ? 1 : 0);
      scenes.push_back(std::make_unique<QGraphicsScene>());
      stores.push_back(std::make_unique<ItemStore>(scenes.back().get()));
      managers.push_back(std::make_unique<LayerManager>(scenes.back().get()));
      managers.back()->setItemStore(stores.back().get());
      QRectF rect;
      QColor bg;
      QVERIFY(ProjectSerializer::loadProject(path, scenes.back().get(),
                                             stores.back().get(),
                                             managers.back().get(), rect, bg));
      current = scenes.back().get();
      currentStore = stores.back().get();
      currentManager = managers.back().get();
      QCOMPARE(currentManager->layer(0)->itemCount(), 1);
      const QRectF loaded =
          currentManager->layer(0)->items().first()->sceneBoundingRect();
      // QGraphicsPixmapItem pads its bounds by half a pixel on each side.
      QVERIFY2(qAbs(loaded.width() - expected.width()) <= 1.0 &&
                   qAbs(loaded.height() - expected.height()) <= 1.0 &&
                   (loaded.topLeft() - expected.topLeft()).manhattanLength() <=
                       1.0,
               qPrintable(QString("round %1: %2,%3 %4x%5")
                              .arg(round)
                              .arg(loaded.x())
                              .arg(loaded.y())
                              .arg(loaded.width())
                              .arg(loaded.height())));
    }
  }
};

QTEST_MAIN(TestProjectSerializer)
#include "test_project_serializer.moc"
