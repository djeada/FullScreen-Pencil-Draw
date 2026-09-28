/**
 * @file test_document_integrity.cpp
 * @brief Regression gates for document integrity: lossless native round
 *        trips of mixed vector/raster documents, atomic saves, crash
 *        recovery snapshots and format-aware export.
 */
#include "../src/core/document_exporter.h"
#include "../src/core/item_store.h"
#include "../src/core/layer.h"
#include "../src/core/project_serializer.h"
#include "../src/core/recovery_store.h"
#include "../src/widgets/brush_stroke_item.h"
#include "../src/widgets/raster_layer_item.h"
#include <QFile>
#include <QGraphicsColorizeEffect>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

/// A document in memory: scene, item store and layers.
struct Doc {
  QGraphicsScene scene;
  ItemStore store{&scene};
  LayerManager layers{&scene};
  Doc() {
    layers.setItemStore(&store);
    scene.setSceneRect(0, 0, 400, 300);
  }
  ItemId add(QGraphicsItem *item, Layer *layer = nullptr) {
    const ItemId id = store.registerItem(item);
    (layer ? layer : layers.activeLayer())->addItem(id, &store);
    return id;
  }
};

/// Mixed document: vector paths/shapes/text, raster layer pixels, an
/// image, a custom brush stroke, a group, several layers with blend modes.
void buildMixedDocument(Doc &doc) {
  Layer *vector = doc.layers.layer(0);
  vector->setName("Vectors");

  QPainterPath curve;
  curve.moveTo(20, 20);
  curve.cubicTo(80, 0, 120, 120, 180, 60);
  auto *path = new QGraphicsPathItem(curve);
  QPen dashed(QColor(200, 30, 30), 4);
  dashed.setDashPattern({3, 2, 1, 2});
  dashed.setCapStyle(Qt::RoundCap);
  path->setPen(dashed);
  doc.add(path, vector);

  auto *rect = new QGraphicsRectItem(0, 0, 60, 40);
  rect->setPos(200, 30);
  rect->setBrush(QColor(20, 120, 220));
  rect->setRotation(15);
  rect->setOpacity(0.8);
  doc.add(rect, vector);

  auto *text = new QGraphicsTextItem("Hybrid");
  text->setPos(40, 200);
  QFont font("Sans Serif");
  font.setPixelSize(22);
  font.setUnderline(true);
  text->setFont(font);
  text->setDefaultTextColor(Qt::darkGreen);
  doc.add(text, vector);

  auto *group = new QGraphicsItemGroup();
  auto *line = new QGraphicsLineItem(0, 0, 50, 0);
  line->setPen(QPen(Qt::black, 3));
  auto *dot = new QGraphicsEllipseItem(45, -5, 10, 10);
  dot->setBrush(Qt::black);
  group->addToGroup(line);
  group->addToGroup(dot);
  group->setPos(250, 200);
  doc.add(group, vector);

  // Raster layer with painted pixels, partly erased.
  Layer *raster = doc.layers.createLayer("Paint", Layer::Type::Raster);
  raster->setBlendMode(Layer::BlendMode::Multiply);
  raster->setOpacity(0.9);
  auto *pixels = new RasterLayerItem();
  pixels->paintStroke(QPointF(30, 150), QPointF(330, 150),
                      QPen(QColor(250, 200, 0), 30));
  pixels->eraseDab(QPointF(180, 150), 40, 1.0, 1.0);
  doc.add(pixels, raster);

  // Mixed layer: image with a tint, brush stroke, hidden and locked items.
  Layer *mixed = doc.layers.createLayer("Mixed", Layer::Type::Mixed);
  mixed->setBlendMode(Layer::BlendMode::Screen);
  QImage img(40, 30, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::transparent);
  {
    QPainter p(&img);
    p.fillRect(QRect(5, 5, 30, 20), QColor(90, 0, 160));
  }
  auto *image = new QGraphicsPixmapItem(QPixmap::fromImage(img));
  image->setPos(320, 220);
  auto *tint = new QGraphicsColorizeEffect();
  tint->setColor(Qt::red);
  tint->setStrength(0.5);
  image->setGraphicsEffect(tint);
  doc.add(image, mixed);

  BrushTip tip;
  tip.setShape(BrushTipShape::Chisel);
  tip.setAngle(30);
  auto *stroke = new BrushStrokeItem(tip, 10, QColor(0, 140, 70), 0.9);
  doc.store.registerItem(stroke);
  stroke->addPoint(QPointF(60, 80));
  stroke->addPoint(QPointF(100, 95));
  stroke->addPoint(QPointF(140, 80));
  mixed->addItem(doc.store.idForItem(stroke), &doc.store);

  auto *hidden = new QGraphicsEllipseItem(0, 0, 20, 20);
  hidden->setVisible(false);
  doc.add(hidden, mixed);
  auto *locked = new QGraphicsRectItem(0, 0, 10, 10);
  locked->setData(0, "locked");
  doc.add(locked, mixed);

  Layer *hiddenLayer = doc.layers.createLayer("Hidden", Layer::Type::Vector);
  doc.add(new QGraphicsRectItem(0, 0, 30, 30), hiddenLayer);
  hiddenLayer->setVisible(false);
  hiddenLayer->setLocked(true);

  doc.layers.setActiveLayer(1);
  doc.layers.updateLayerZOrder();
}

QImage renderDoc(Doc &doc) {
  DocumentExporter exporter(&doc.layers, nullptr, Qt::white);
  return exporter.renderImage(QRectF(0, 0, 400, 300), QSize(400, 300));
}

/// Largest per-channel difference between two renders.
int maxDifference(const QImage &a, const QImage &b) {
  if (a.size() != b.size())
    return 255;
  const QImage x = a.convertToFormat(QImage::Format_ARGB32);
  const QImage y = b.convertToFormat(QImage::Format_ARGB32);
  int worst = 0;
  for (int row = 0; row < x.height(); ++row) {
    const auto *l1 = reinterpret_cast<const QRgb *>(x.constScanLine(row));
    const auto *l2 = reinterpret_cast<const QRgb *>(y.constScanLine(row));
    for (int col = 0; col < x.width(); ++col) {
      worst = qMax(worst, qAbs(qRed(l1[col]) - qRed(l2[col])));
      worst = qMax(worst, qAbs(qGreen(l1[col]) - qGreen(l2[col])));
      worst = qMax(worst, qAbs(qBlue(l1[col]) - qBlue(l2[col])));
      worst = qMax(worst, qAbs(qAlpha(l1[col]) - qAlpha(l2[col])));
    }
  }
  return worst;
}

QByteArray readFile(const QString &path) {
  QFile f(path);
  return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

bool save(Doc &doc, const QString &path,
          ProjectSerializer::SaveStatus *status = nullptr) {
  return ProjectSerializer::saveProject(path, &doc.scene, &doc.store,
                                        &doc.layers, doc.scene.sceneRect(),
                                        Qt::white, {}, status);
}

bool load(Doc &doc, const QString &path, QString *error = nullptr,
          ProjectSerializer::LoadExtras *extras = nullptr) {
  QRectF rect;
  QColor bg;
  return ProjectSerializer::loadProject(path, &doc.scene, &doc.store,
                                        &doc.layers, rect, bg, extras, error);
}

template <typename T> QList<T *> itemsOfType(Doc &doc) {
  QList<T *> result;
  for (QGraphicsItem *item : doc.scene.items())
    if (auto *t = dynamic_cast<T *>(item))
      result.append(t);
  return result;
}
} // namespace

class TestDocumentIntegrity : public QObject {
  Q_OBJECT

private slots:
  void cleanup() {
    ProjectSerializer::setWriteFaultForTesting(
        ProjectSerializer::WriteFault::None);
  }

  // ---------------- #163 lossless native round trips ----------------

  void mixedDocumentRoundTripIsLossless() {
    QTemporaryDir dir;
    const QString path = dir.filePath("mixed.fspd");
    Doc original;
    buildMixedDocument(original);
    QVERIFY(ProjectSerializer::findUnsupportedItems(&original.store,
                                                    &original.layers)
                .isEmpty());
    ProjectSerializer::SaveStatus status;
    QVERIFY2(save(original, path, &status), qPrintable(status.message));
    QVERIFY(status.rasterizedItems.isEmpty());

    Doc reopened;
    QString error;
    QVERIFY2(load(reopened, path, &error), qPrintable(error));

    // Structure: layers with every property, identity and order.
    QCOMPARE(reopened.layers.layerCount(), original.layers.layerCount());
    QCOMPARE(reopened.layers.activeLayerIndex(),
             original.layers.activeLayerIndex());
    for (int i = 0; i < original.layers.layerCount(); ++i) {
      Layer *a = original.layers.layer(i);
      Layer *b = reopened.layers.layer(i);
      QCOMPARE(b->id(), a->id());
      QCOMPARE(b->name(), a->name());
      QCOMPARE(b->type(), a->type());
      QCOMPARE(b->blendMode(), a->blendMode());
      QCOMPARE(b->isVisible(), a->isVisible());
      QCOMPARE(b->isLocked(), a->isLocked());
      QCOMPARE(b->opacity(), a->opacity());
      QCOMPARE(b->itemIds(), a->itemIds()); // identity + stacking order
    }

    // Editability: items come back as their own types.
    QCOMPARE(itemsOfType<BrushStrokeItem>(reopened).size(), 1);
    QCOMPARE(itemsOfType<RasterLayerItem>(reopened).size(), 1);
    QCOMPARE(itemsOfType<QGraphicsItemGroup>(reopened).size(), 1);
    QCOMPARE(itemsOfType<QGraphicsTextItem>(reopened).size(), 1);
    auto *stroke = itemsOfType<BrushStrokeItem>(reopened).first();
    auto *origStroke = itemsOfType<BrushStrokeItem>(original).first();
    QCOMPARE(stroke->tip().shape(), BrushTipShape::Chisel);
    QCOMPARE(stroke->points(), origStroke->points());
    QCOMPARE(stroke->image(), origStroke->image());
    auto *raster = itemsOfType<RasterLayerItem>(reopened).first();
    auto *origRaster = itemsOfType<RasterLayerItem>(original).first();
    QCOMPARE(raster->surface().tileKeys().size(),
             origRaster->surface().tileKeys().size());
    for (const RasterTileKey &key : origRaster->surface().tileKeys())
      QCOMPARE(raster->surface().tile(key), origRaster->surface().tile(key));
    auto *pathItem = itemsOfType<QGraphicsPathItem>(reopened).first();
    QCOMPARE(
        pathItem->pen().dashPattern(),
        itemsOfType<QGraphicsPathItem>(original).first()->pen().dashPattern());
    auto *text = itemsOfType<QGraphicsTextItem>(reopened).first();
    QVERIFY(text->font().underline());
    QCOMPARE(text->font().pixelSize(), 22);
    bool foundRotated = false;
    for (auto *r : itemsOfType<QGraphicsRectItem>(reopened))
      foundRotated |= qFuzzyCompare(r->rotation(), 15.0);
    QVERIFY(foundRotated);
    auto *image = itemsOfType<QGraphicsPixmapItem>(reopened).first();
    QVERIFY(qobject_cast<QGraphicsColorizeEffect *>(image->graphicsEffect()));

    // Appearance: the reopened document renders like the original.
    const int diff = maxDifference(renderDoc(original), renderDoc(reopened));
    QVERIFY2(diff <= 2, qPrintable(QString("max channel diff %1").arg(diff)));

    // Saving is stable: after one round trip (Qt normalises rich-text HTML
    // once, without visual change) further save/reopen cycles are
    // byte-identical, so nothing drifts or decays over repeated edits.
    const QString again = dir.filePath("again.fspd");
    QVERIFY(save(reopened, again));
    Doc third;
    QVERIFY(load(third, again));
    const QString thirdPath = dir.filePath("third.fspd");
    QVERIFY(save(third, thirdPath));
    QCOMPARE(readFile(thirdPath), readFile(again));
  }

  void baseImageRoundTrips() {
    QTemporaryDir dir;
    const QString path = dir.filePath("base.fspd");
    Doc doc;
    QImage img(30, 20, QImage::Format_ARGB32);
    img.fill(QColor(1, 2, 3));
    QGraphicsPixmapItem base(QPixmap::fromImage(img));
    base.setPos(5, 6);
    ProjectSerializer::SaveOptions options;
    options.backgroundImage = &base;
    QVERIFY(ProjectSerializer::saveProject(path, &doc.scene, &doc.store,
                                           &doc.layers, doc.scene.sceneRect(),
                                           Qt::white, options));
    Doc reopened;
    ProjectSerializer::LoadExtras extras;
    QVERIFY(load(reopened, path, nullptr, &extras));
    QVERIFY(extras.hasBackgroundImage);
    QCOMPARE(extras.backgroundImagePos, QPointF(5, 6));
    QCOMPARE(
        extras.backgroundImage.toImage().convertToFormat(QImage::Format_ARGB32),
        img);
    QCOMPARE(extras.formatVersion, ProjectSerializer::FORMAT_VERSION);
  }

  void version1FilesStillLoad() {
    QTemporaryDir dir;
    const QString path = dir.filePath("v1.fspd");
    const QByteArray v1 = R"({
      "formatVersion": 1,
      "application": "FullScreenPencilDraw",
      "canvas": {"x": 0, "y": 0, "width": 640, "height": 480,
                 "backgroundColor": "#ff202020"},
      "activeLayer": 0,
      "layers": [{
        "name": "Background", "visible": true, "locked": false,
        "opacity": 1, "type": 0, "blendMode": 2,
        "items": [{
          "type": "rect", "x": 10, "y": 10, "z": 0, "visible": true,
          "opacity": 1, "rx": 0, "ry": 0, "rw": 50, "rh": 20,
          "pen": {"color": "#ff000000", "width": 2, "style": 1,
                  "capStyle": 16, "joinStyle": 64},
          "brush": {"color": "#ffff0000", "style": 1},
          "transform": {"m11": 1, "m12": 0, "m13": 0, "m21": 0, "m22": 1,
                        "m23": 0, "m31": 0, "m32": 0, "m33": 1}
        }, {
          "type": "text", "x": 0, "y": 0, "z": 1, "visible": true,
          "opacity": 1, "html": "hi", "defaultColor": "#ff00ff00",
          "fontFamily": "Sans", "fontSize": 14, "fontBold": true,
          "fontItalic": false
        }]
      }]
    })";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(v1);
    f.close();
    Doc doc;
    QString error;
    QVERIFY2(load(doc, path, &error), qPrintable(error));
    QCOMPARE(doc.layers.layer(0)->itemCount(), 2);
    QCOMPARE(doc.layers.layer(0)->blendMode(), Layer::BlendMode::Screen);
    auto *text = itemsOfType<QGraphicsTextItem>(doc).first();
    QCOMPARE(text->font().pointSize(), 14);
    QVERIFY(text->font().bold());
  }

  void newerFormatAndCorruptFilesLeaveTheDocumentAlone() {
    QTemporaryDir dir;
    Doc doc;
    doc.add(new QGraphicsRectItem(0, 0, 10, 10));
    const QList<ItemId> before = doc.layers.layer(0)->itemIds();

    const QString newer = dir.filePath("newer.fspd");
    QFile f(newer);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"({"formatVersion": 99, "layers": []})");
    f.close();
    QString error;
    QVERIFY(!load(doc, newer, &error));
    QVERIFY(error.contains("99"));

    // A damaged image must not half-replace the open document.
    Doc source;
    buildMixedDocument(source);
    const QString good = dir.filePath("good.fspd");
    QVERIFY(save(source, good));
    QJsonObject root = QJsonDocument::fromJson(readFile(good)).object();
    QJsonArray layers = root["layers"].toArray();
    QJsonObject mixed = layers[2].toObject();
    QJsonArray items = mixed["items"].toArray();
    QJsonObject image = items[0].toObject();
    image["data"] = "@@@@";
    items[0] = image;
    mixed["items"] = items;
    layers[2] = mixed;
    root["layers"] = layers;
    const QString corrupt = dir.filePath("corrupt.fspd");
    QFile c(corrupt);
    QVERIFY(c.open(QIODevice::WriteOnly));
    c.write(QJsonDocument(root).toJson());
    c.close();
    QVERIFY(!load(doc, corrupt, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(doc.layers.layerCount(), 1);
    QCOMPARE(doc.layers.layer(0)->itemIds(), before);
  }

  // ---------------- #164 atomic, verified saves ----------------

  void failedSavesKeepThePreviousFile() {
    QTemporaryDir dir;
    const QString path = dir.filePath("doc.fspd");
    Doc doc;
    doc.add(new QGraphicsRectItem(0, 0, 10, 10));
    QVERIFY(save(doc, path));
    const QByteArray good = readFile(path);

    doc.add(new QGraphicsEllipseItem(0, 0, 50, 50)); // unsaved change
    using Fault = ProjectSerializer::WriteFault;
    using Error = ProjectSerializer::SaveError;
    const QList<QPair<Fault, Error>> faults = {
        {Fault::Open, Error::OpenFailed},
        {Fault::ShortWrite, Error::WriteFailed},
        {Fault::Commit, Error::CommitFailed}};
    for (const auto &fault : faults) {
      ProjectSerializer::setWriteFaultForTesting(fault.first);
      ProjectSerializer::SaveStatus status;
      QVERIFY(!save(doc, path, &status));
      QCOMPARE(status.error, fault.second);
      QVERIFY(status.message.contains("doc.fspd"));
      QCOMPARE(readFile(path), good); // last good file intact
    }
    ProjectSerializer::setWriteFaultForTesting(Fault::None);
    // No temporary files are left next to the document.
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 1);
    QVERIFY(save(doc, path));
    QVERIFY(readFile(path) != good);
  }

  void unwritableLocationReportsThePath() {
    Doc doc;
    ProjectSerializer::SaveStatus status;
    const QString path = "/nonexistent-dir-for-fspd-test/doc.fspd";
    QVERIFY(!save(doc, path, &status));
    QCOMPARE(status.error, ProjectSerializer::SaveError::OpenFailed);
    QVERIFY(status.message.contains("nonexistent-dir-for-fspd-test"));
  }

  // ---------------- #165 editable crash recovery ----------------

  void recoverySnapshotIsAnEditableDocument() {
    QTemporaryDir dir;
    Doc doc;
    buildMixedDocument(doc);
    const QImage expected = renderDoc(doc);

    QString sessionId;
    {
      RecoveryStore store(dir.path());
      sessionId = store.createSession();
      ProjectSerializer::SaveOptions options;
      options.allowRasterFallback = true;
      const QByteArray data = ProjectSerializer::serializeProject(
          &doc.store, &doc.layers, doc.scene.sceneRect(), Qt::white, options);
      RecoverySnapshot info;
      info.displayName = "Untitled";
      info.layerCount = doc.layers.layerCount();
      QVERIFY(store.writeSnapshot(sessionId, data, info));
      // A live session is never offered for recovery (e.g. to a second
      // running instance).
      RecoveryStore other(dir.path());
      QVERIFY(other.recoverableSnapshots().isEmpty());
    } // "crash": the session ends without discarding its snapshot

    RecoveryStore relaunch(dir.path());
    const QList<RecoverySnapshot> found = relaunch.recoverableSnapshots();
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().documentId, sessionId);
    QCOMPARE(found.first().layerCount, doc.layers.layerCount());

    Doc recovered;
    QString error;
    QVERIFY2(load(recovered, found.first().snapshotPath, &error),
             qPrintable(error));
    QCOMPARE(recovered.layers.layerCount(), doc.layers.layerCount());
    QCOMPARE(itemsOfType<RasterLayerItem>(recovered).size(), 1);
    QCOMPARE(itemsOfType<BrushStrokeItem>(recovered).size(), 1);
    QVERIFY(maxDifference(expected, renderDoc(recovered)) <= 2);

    relaunch.discard(sessionId);
    QVERIFY(relaunch.recoverableSnapshots().isEmpty());
  }

  void documentsNeverOverwriteEachOthersSnapshots() {
    QTemporaryDir dir;
    RecoveryStore store(dir.path());
    const QString a = store.createSession();
    const QString b = store.createSession();
    QVERIFY(a != b);
    QVERIFY(store.writeSnapshot(a, R"({"formatVersion":2,"a":1})", {}));
    QVERIFY(store.writeSnapshot(b, R"({"formatVersion":2,"b":1})", {}));
    QVERIFY(readFile(store.snapshotPath(a)).contains("\"a\""));
    QVERIFY(readFile(store.snapshotPath(b)).contains("\"b\""));
    store.releaseSession(a);
    store.releaseSession(b);
    QCOMPARE(RecoveryStore(dir.path()).recoverableSnapshots().size(), 2);
  }

  void failedAutosaveKeepsTheLastValidSnapshot() {
    QTemporaryDir dir;
    RecoveryStore store(dir.path());
    const QString id = store.createSession();
    const QByteArray first = R"({"formatVersion":2,"version":"first"})";
    QVERIFY(store.writeSnapshot(id, first, {}));
    ProjectSerializer::setWriteFaultForTesting(
        ProjectSerializer::WriteFault::ShortWrite);
    QString error;
    QVERIFY(!store.writeSnapshot(id, R"({"formatVersion":2,"v":"second"})", {},
                                 &error));
    QVERIFY(!error.isEmpty());
    ProjectSerializer::setWriteFaultForTesting(
        ProjectSerializer::WriteFault::None);
    QCOMPARE(readFile(store.snapshotPath(id)), first);
    store.releaseSession(id);
    QCOMPARE(RecoveryStore(dir.path()).recoverableSnapshots().size(), 1);
  }

  void incompleteSnapshotsAreNotOffered() {
    QTemporaryDir dir;
    // Metadata without a readable project (e.g. interrupted first write).
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QFile meta(dir.filePath(id + ".json"));
    QVERIFY(meta.open(QIODevice::WriteOnly));
    meta.write("{}");
    meta.close();
    QFile snap(dir.filePath(id + ".fspd"));
    QVERIFY(snap.open(QIODevice::WriteOnly));
    snap.write("{\"formatVer");
    snap.close();
    QVERIFY(RecoveryStore(dir.path()).recoverableSnapshots().isEmpty());
  }

  // ---------------- #170 hybrid export ----------------

  void svgKeepsVectorsAndEmbedsRaster() {
#ifndef HAVE_QT_SVG
    QSKIP("Qt SVG not available");
#else
    QTemporaryDir dir;
    Doc doc;
    buildMixedDocument(doc);
    DocumentExporter exporter(&doc.layers, nullptr, Qt::white);
    const QString path = dir.filePath("out.svg");
    const ExportResult result = exporter.exportToFile(path);
    QVERIFY2(result.ok, qPrintable(result.error));
    const QByteArray svg = readFile(path);
    QVERIFY(svg.contains("<path"));  // vector geometry
    QVERIFY(svg.contains("<image")); // raster pixels embedded
    QVERIFY(svg.contains("Hybrid") || svg.contains("<text")); // vector text
    // Blend modes cannot be expressed: reported, never silent.
    QVERIFY(result.warnings.join(' ').contains("Multiply"));
#endif
  }

  void pdfExportWritesAPdf() {
    QTemporaryDir dir;
    Doc doc;
    buildMixedDocument(doc);
    DocumentExporter exporter(&doc.layers, nullptr, Qt::white);
    const QString path = dir.filePath("out.pdf");
    const ExportResult result = exporter.exportToFile(path);
    QVERIFY2(result.ok, qPrintable(result.error));
    const QByteArray pdf = readFile(path);
    QVERIFY(pdf.startsWith("%PDF"));
    QVERIFY(pdf.contains("/Image")); // raster content embedded
  }

  void bitmapExportMatchesTheComposite() {
    QTemporaryDir dir;
    Doc doc;
    buildMixedDocument(doc);
    DocumentExporter exporter(&doc.layers, nullptr, Qt::white);
    const QRectF area = exporter.contentRect();
    const QImage composite = exporter.renderImage(area, area.size().toSize());
    const QString path = dir.filePath("out.png");
    const ExportResult result = exporter.exportToFile(path);
    QVERIFY2(result.ok, qPrintable(result.error));
    QVERIFY(result.warnings.isEmpty());
    QImage png(path);
    QCOMPARE(png.size(), composite.size());
    QVERIFY(maxDifference(png, composite) <= 1);

    // The composite honours blend modes like the canvas does: Multiply
    // over white leaves the paint colour, not white.
    const QPoint paint = (QPointF(60, 150) - area.topLeft()).toPoint();
    QVERIFY(qBlue(composite.pixel(paint)) < 100);
    // Erased raster pixels are really gone.
    const QPoint erased = (QPointF(180, 150) - area.topLeft()).toPoint();
    QCOMPARE(QColor(composite.pixel(erased)), QColor(Qt::white));
  }

  void jpegFlattensTransparencyWithAWarning() {
    QTemporaryDir dir;
    Doc doc;
    doc.add(new QGraphicsRectItem(0, 0, 20, 20));
    DocumentExporter exporter(&doc.layers, nullptr, Qt::transparent);
    const QString path = dir.filePath("out.jpg");
    const ExportResult result = exporter.exportToFile(path);
    QVERIFY2(result.ok, qPrintable(result.error));
    QVERIFY(!result.warnings.isEmpty());
    const QImage jpg(path);
    QVERIFY(qGray(jpg.pixel(1, 1)) > 240); // white, not black

    // PNG keeps the transparency.
    const QString png = dir.filePath("out.png");
    QVERIFY(exporter.exportToFile(png).ok);
    QCOMPARE(qAlpha(QImage(png).pixel(1, 1)), 0);
  }

  void capabilityMatrixCoversEveryFormat() {
    const auto matrix = DocumentExporter::capabilityMatrix();
    QCOMPARE(matrix.size(), 8);
    for (const ExportFormatInfo &info : matrix) {
      QVERIFY(!info.name.isEmpty());
      QVERIFY(!info.notes.isEmpty());
    }
    QVERIFY(DocumentExporter::formatInfo(ExportFormat::Svg).keepsVectors);
    QVERIFY(!DocumentExporter::formatInfo(ExportFormat::Jpeg).alpha);
    QCOMPARE(DocumentExporter::formatForFileName("a.TIF"), ExportFormat::Tiff);
    QVERIFY(!DocumentExporter(nullptr, nullptr, Qt::white)
                 .exportToFile("x.fspd")
                 .ok);
  }
};

QTEST_MAIN(TestDocumentIntegrity)
#include "test_document_integrity.moc"
