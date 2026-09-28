/**
 * @file test_raster_surface.cpp
 * @brief Tests for the tiled raster engine: tiles, alpha erase, diffs,
 *        undo, serialization and memory behaviour.
 */
#include "../src/core/item_store.h"
#include "../src/core/raster_surface.h"
#include "../src/widgets/brush_stroke_item.h"
#include "../src/widgets/raster_layer_item.h"
#include <QElapsedTimer>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QPainter>
#include <QtTest/QtTest>

namespace {
void fillRect(RasterSurface &surface, const QRect &rect, const QColor &color) {
  surface.paint(rect, [&](QPainter &p) { p.fillRect(rect, color); });
}
} // namespace

class TestRasterSurface : public QObject {
  Q_OBJECT

private slots:
  void paintAllocatesOnlyTouchedTiles() {
    RasterSurface surface;
    QVERIFY(surface.isEmpty());
    fillRect(surface, QRect(10, 10, 20, 20), Qt::red);
    QCOMPARE(surface.tileCount(), 1);
    QCOMPARE(qAlpha(surface.pixel(QPoint(15, 15))), 255);
    QCOMPARE(qAlpha(surface.pixel(QPoint(40, 40))), 0);
    QCOMPARE(surface.memoryBytes(), std::size_t(RasterSurface::kTileSize *
                                                RasterSurface::kTileSize * 4));
  }

  void paintSpanningTileBoundariesAndNegativeCoordinates() {
    RasterSurface surface;
    const int t = RasterSurface::kTileSize;
    // A rect straddling the origin covers four tiles, two of them negative.
    const QRect rect(-10, -10, 20, 20);
    fillRect(surface, rect, Qt::blue);
    QCOMPARE(surface.tileCount(), 4);
    for (const QPoint &p :
         {QPoint(-5, -5), QPoint(5, -5), QPoint(-5, 5), QPoint(5, 5)})
      QCOMPARE(qAlpha(surface.pixel(p)), 255);
    QCOMPARE(qAlpha(surface.pixel(QPoint(-11, 0))), 0);
    QCOMPARE(surface.boundingRect(), QRect(-t, -t, 2 * t, 2 * t));
  }

  void eraseRemovesAlphaPartiallyAndNeverAllocates() {
    RasterSurface surface;
    fillRect(surface, QRect(0, 0, 100, 100), Qt::green);
    QPainterPath dab;
    dab.addEllipse(QPointF(50, 50), 10, 10);
    surface.erase(dab, 0.5); // half strength
    const int a = qAlpha(surface.pixel(QPoint(50, 50)));
    QVERIFY2(a > 100 && a < 150, qPrintable(QString::number(a)));
    QCOMPARE(qAlpha(surface.pixel(QPoint(5, 5))), 255);

    surface.erase(dab, 1.0);
    QCOMPARE(qAlpha(surface.pixel(QPoint(50, 50))), 0);

    // Erasing where nothing was painted must not create tiles.
    QPainterPath far;
    far.addEllipse(QPointF(5000, 5000), 20, 20);
    surface.erase(far);
    QCOMPARE(surface.tileCount(), 1);
  }

  void softEraseFadesTowardsTheEdge() {
    RasterSurface surface;
    fillRect(surface, QRect(0, 0, 100, 100), Qt::black);
    QPainterPath dab;
    dab.addEllipse(QPointF(50, 50), 20, 20);
    surface.erase(dab, 1.0, 0.0); // fully soft
    const int centre = qAlpha(surface.pixel(QPoint(50, 50)));
    const int edge = qAlpha(surface.pixel(QPoint(50, 67)));
    QVERIFY2(centre < edge, qPrintable(QString("%1 %2").arg(centre).arg(edge)));
  }

  void erasingEverythingReleasesTiles() {
    RasterSurface surface;
    fillRect(surface, QRect(0, 0, 10, 10), Qt::red);
    QPainterPath all;
    all.addRect(QRectF(-1, -1, 20, 20));
    surface.erase(all);
    QCOMPARE(surface.tileCount(), 0);
  }

  void clipLimitsTheErase() {
    RasterSurface surface;
    fillRect(surface, QRect(0, 0, 100, 100), Qt::red);
    QPainterPath dab;
    dab.addRect(QRectF(0, 0, 100, 100));
    QPainterPath clip;
    clip.addRect(QRectF(0, 0, 50, 100)); // e.g. a selection
    surface.erase(dab, 1.0, 1.0, clip);
    QCOMPARE(qAlpha(surface.pixel(QPoint(25, 50))), 0);
    QCOMPARE(qAlpha(surface.pixel(QPoint(75, 50))), 255);
  }

  void diffRecordsOnlyTouchedTilesAndUndoes() {
    RasterSurface surface;
    const int t = RasterSurface::kTileSize;
    fillRect(surface, QRect(0, 0, 4 * t, 4 * t), Qt::red); // 16 tiles
    QCOMPARE(surface.tileCount(), 16);

    surface.beginEdit();
    QPainterPath dab;
    dab.addEllipse(QPointF(t + 20, t + 20), 8, 8); // inside one tile
    surface.erase(dab);
    const RasterTileDiff diff = surface.endEdit();
    QCOMPARE(diff.before.size(), 1);
    QCOMPARE(diff.after.size(), 1);
    QVERIFY(diff.before.contains(QPoint(1, 1)));
    // An undo step for a small edit stores two tiles, not the whole image.
    QCOMPARE(diff.byteSize(), std::size_t(2 * t * t * 4));

    QCOMPARE(qAlpha(surface.pixel(QPoint(t + 20, t + 20))), 0);
    surface.applyDiff(diff, false);
    QCOMPARE(qAlpha(surface.pixel(QPoint(t + 20, t + 20))), 255);
    surface.applyDiff(diff, true);
    QCOMPARE(qAlpha(surface.pixel(QPoint(t + 20, t + 20))), 0);
  }

  void diffOfNewTileUndoesToNoTile() {
    RasterSurface surface;
    surface.beginEdit();
    fillRect(surface, QRect(0, 0, 10, 10), Qt::red);
    const RasterTileDiff diff = surface.endEdit();
    QCOMPARE(surface.tileCount(), 1);
    surface.applyDiff(diff, false);
    QCOMPARE(surface.tileCount(), 0);
    surface.applyDiff(diff, true);
    QCOMPARE(surface.tileCount(), 1);
  }

  void jsonRoundTripKeepsPixels() {
    RasterSurface surface;
    fillRect(surface, QRect(-30, -30, 300, 60), QColor(10, 20, 30, 128));
    const QJsonArray json = surface.toJson();
    RasterSurface loaded;
    QString error;
    QVERIFY2(loaded.fromJson(json, &error), qPrintable(error));
    QCOMPARE(loaded.tileCount(), surface.tileCount());
    for (const RasterTileKey &key : surface.tileKeys())
      QCOMPARE(loaded.tile(key), surface.tile(key));
  }

  void jsonRejectsCorruptTiles() {
    RasterSurface surface;
    fillRect(surface, QRect(0, 0, 10, 10), Qt::red);
    QString error;

    QJsonArray badPng;
    badPng.append(QJsonObject{{"tx", 0}, {"ty", 0}, {"png", "not base64 png"}});
    QVERIFY(!surface.fromJson(badPng, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(surface.tileCount(), 1); // unchanged on failure

    QJsonArray outOfRange;
    outOfRange.append(QJsonObject{{"tx", 1 << 30}, {"ty", 0}, {"png", ""}});
    QVERIFY(!surface.fromJson(outOfRange, &error));

    // Wrong tile size.
    QImage small(8, 8, QImage::Format_ARGB32);
    small.fill(Qt::red);
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    small.save(&buf, "PNG");
    QJsonArray wrongSize;
    wrongSize.append(QJsonObject{
        {"tx", 0}, {"ty", 0}, {"png", QString::fromLatin1(ba.toBase64())}});
    QVERIFY(!surface.fromJson(wrongSize, &error));
  }

  void rasterItemBoundsFollowTheSurface() {
    RasterLayerItem item;
    QVERIFY(item.boundingRect().isEmpty());
    item.paintStroke(QPointF(10, 10), QPointF(40, 10), QPen(Qt::black, 4));
    QVERIFY(!item.boundingRect().isEmpty());
    QVERIFY(!(item.flags() & QGraphicsItem::ItemIsMovable));
    // Nothing (e.g. unlocking its layer) can make the content draggable.
    item.setFlag(QGraphicsItem::ItemIsMovable, true);
    item.setFlag(QGraphicsItem::ItemIsSelectable, true);
    QVERIFY(!(item.flags() & QGraphicsItem::ItemIsMovable));
    QVERIFY(!(item.flags() & QGraphicsItem::ItemIsSelectable));
    QCOMPARE(qAlpha(item.surface().pixel(QPoint(25, 10))), 255);
  }

  void pixelEditActionUndoRedoForTilesAndImages() {
    QGraphicsScene scene;
    ItemStore store(&scene);

    // Tiled raster layer content.
    auto *raster = new RasterLayerItem();
    const ItemId rasterId = store.registerItem(raster);
    raster->paintStroke(QPointF(0, 0), QPointF(50, 0), QPen(Qt::red, 10));
    raster->surface().beginEdit();
    raster->eraseDab(QPointF(25, 0), 12, 1.0, 1.0);
    PixelEditAction tiled(rasterId, &store, raster->surface().endEdit());
    QCOMPARE(qAlpha(raster->surface().pixel(QPoint(25, 0))), 0);
    tiled.undo();
    QCOMPARE(qAlpha(raster->surface().pixel(QPoint(25, 0))), 255);
    tiled.redo();
    QCOMPARE(qAlpha(raster->surface().pixel(QPoint(25, 0))), 0);
    QVERIFY(tiled.memoryCost() > Action::kBaseActionCost);

    // Image (pixmap) content.
    QImage before(20, 20, QImage::Format_ARGB32_Premultiplied);
    before.fill(Qt::blue);
    auto *pixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(before));
    const ItemId pixId = store.registerItem(pixmapItem);
    QImage after = before;
    after.setPixel(5, 5, qRgba(0, 0, 0, 0));
    PixelEditAction image(pixId, &store, before, after);
    image.redo();
    QCOMPARE(qAlpha(pixmapItem->pixmap().toImage().pixel(5, 5)), 0);
    image.undo();
    QCOMPARE(qAlpha(pixmapItem->pixmap().toImage().pixel(5, 5)), 255);

    // A deleted item turns the step into a no-op instead of a crash.
    store.unregisterItem(pixId);
    image.redo();
    delete pixmapItem;
  }

  void opaquePixelHitTestIgnoresTransparentAreas() {
    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter(&image).fillRect(QRect(0, 0, 10, 10), Qt::black);
    QGraphicsPixmapItem item(QPixmap::fromImage(image));
    item.setPos(200, 200);
    QPainterPath centre;
    centre.addEllipse(QPointF(250, 250), 5, 5);
    QVERIFY(!PixelEditAction::hitsOpaquePixels(&item, centre));
    QPainterPath corner;
    corner.addEllipse(QPointF(204, 204), 5, 5);
    QVERIFY(PixelEditAction::hitsOpaquePixels(&item, corner));
  }

  // Measured, not assumed: a small edit on a large canvas stores and
  // touches a bounded amount of memory, unlike replacing the whole image.
  void smallEditOnLargeCanvasIsBounded() {
    const int w = 8192, h = 8192;
    RasterSurface surface;
    fillRect(surface, QRect(0, 0, w, h), Qt::white);
    const int tiles = surface.tileCount();
    QCOMPARE(tiles,
             (w / RasterSurface::kTileSize) * (h / RasterSurface::kTileSize));

    QElapsedTimer timer;
    timer.start();
    surface.beginEdit();
    QPainterPath dab;
    dab.addEllipse(QPointF(4000, 4000), 6, 6);
    surface.erase(dab);
    const RasterTileDiff diff = surface.endEdit();
    const qint64 tiledNs = timer.nsecsElapsed();

    // Whole-image alternative: copy the image before and after an edit.
    QImage full(w, h, QImage::Format_ARGB32_Premultiplied);
    full.fill(Qt::white);
    timer.restart();
    QImage beforeCopy = full.copy();
    {
      QPainter p(&full);
      RasterSurface::erasePath(p, dab, 1.0, 1.0);
    }
    QImage afterCopy = full.copy();
    const qint64 fullNs = timer.nsecsElapsed();
    const std::size_t fullBytes =
        std::size_t(beforeCopy.sizeInBytes() + afterCopy.sizeInBytes());

    qInfo("small edit on %dx%d: tiled diff %zu bytes in %lld us; full-image "
          "undo %zu bytes in %lld us",
          w, h, diff.byteSize(), tiledNs / 1000, fullBytes, fullNs / 1000);
    QVERIFY(diff.byteSize() <= std::size_t(4 * RasterSurface::kTileSize *
                                           RasterSurface::kTileSize * 4));
    QVERIFY(diff.byteSize() * 100 < fullBytes);
  }

  void brushStrokeRestoreKeepsPixels() {
    BrushTip tip;
    tip.setShape(BrushTipShape::Chisel);
    BrushStrokeItem stroke(tip, 8, Qt::red);
    stroke.addPoint(QPointF(0, 0));
    stroke.addPoint(QPointF(40, 10));
    BrushStrokeItem copy(tip, 8, Qt::red);
    copy.restore(stroke.points(), stroke.image(), stroke.imageRect());
    QCOMPARE(copy.image(), stroke.image());
    QCOMPARE(copy.boundingRect(), stroke.boundingRect());
    QCOMPARE(copy.points(), stroke.points());
  }
};

QTEST_MAIN(TestRasterSurface)
#include "test_raster_surface.moc"
