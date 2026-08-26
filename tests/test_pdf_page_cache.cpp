/**
 * @file test_pdf_page_cache.cpp
 * @brief Tests for PdfPageCache memory bounding and LRU eviction.
 */
#include <QtTest/QtTest>

#include "../src/core/pdf_document.h"

namespace {
QImage makeImage(int side) {
  QImage image(side, side, QImage::Format_ARGB32);
  image.fill(Qt::red);
  return image;
}
} // namespace

class TestPdfPageCache : public QObject {
  Q_OBJECT

private slots:
  void storesAndReturnsPages() {
    PdfPageCache cache(4);
    const QImage page = makeImage(16);

    QVERIFY(!cache.hasPage(0, 96));
    cache.setPage(0, 96, page);
    QVERIFY(cache.hasPage(0, 96));
    QCOMPARE(cache.getPage(0, 96).size(), page.size());

    // Same page at a different DPI is a distinct entry.
    QVERIFY(!cache.hasPage(0, 300));
  }

  void nullImagesAreNotCached() {
    PdfPageCache cache(4);
    cache.setPage(1, 96, QImage());
    QVERIFY(!cache.hasPage(1, 96));
  }

  void reinsertingSamePageDoesNotDoubleCountBytes() {
    PdfPageCache cache(64);
    const QImage page = makeImage(32);

    // Re-inserting the same key repeatedly must not accumulate bytes, or the
    // budget would evict everything else for no reason.
    for (int i = 0; i < 50; ++i) {
      cache.setPage(0, 96, page);
    }
    QVERIFY(cache.hasPage(0, 96));

    cache.setPage(1, 96, page);
    cache.setPage(2, 96, page);
    QVERIFY(cache.hasPage(0, 96));
    QVERIFY(cache.hasPage(1, 96));
    QVERIFY(cache.hasPage(2, 96));
  }

  void entryCountCapEvictsLeastRecentlyUsed() {
    PdfPageCache cache(2);
    const QImage page = makeImage(8);

    cache.setPage(0, 96, page);
    cache.setPage(1, 96, page);

    // Touch page 0 so page 1 becomes the least recently used entry.
    QVERIFY(!cache.getPage(0, 96).isNull());

    cache.setPage(2, 96, page);

    QVERIFY(cache.hasPage(0, 96));
    QVERIFY(cache.hasPage(2, 96));
    QVERIFY(!cache.hasPage(1, 96));
  }

  void byteBudgetEvictsLeastRecentlyUsedPages() {
    PdfPageCache cache(100); // entry cap high enough to isolate the byte cap
    const QImage page = makeImage(64);
    const std::size_t pageBytes = static_cast<std::size_t>(page.sizeInBytes());

    // Room for exactly two pages.
    cache.setMaxBytes(pageBytes * 2 + 8);

    cache.setPage(0, 96, page);
    cache.setPage(1, 96, page);
    QCOMPARE(cache.currentBytes(), pageBytes * 2);

    // Touch page 0 so page 1 is the least recently used.
    QVERIFY(!cache.getPage(0, 96).isNull());

    cache.setPage(2, 96, page);
    QVERIFY(cache.currentBytes() <= cache.maxBytes());
    QVERIFY(cache.hasPage(0, 96));
    QVERIFY(cache.hasPage(2, 96));
    QVERIFY2(!cache.hasPage(1, 96),
             "the least recently used page should be evicted first");
  }

  void pageLargerThanTheBudgetIsNotCached() {
    PdfPageCache cache(10);
    const QImage small = makeImage(8);
    const QImage huge = makeImage(256);

    cache.setMaxBytes(static_cast<std::size_t>(small.sizeInBytes()) * 2);
    cache.setPage(0, 96, small);
    cache.setPage(1, 96, huge);

    // The oversized page must be skipped without flushing the cache.
    QVERIFY(!cache.hasPage(1, 96));
    QVERIFY(cache.hasPage(0, 96));
  }

  void shrinkingTheBudgetEvictsImmediately() {
    PdfPageCache cache(10);
    const QImage page = makeImage(32);
    const std::size_t pageBytes = static_cast<std::size_t>(page.sizeInBytes());

    cache.setPage(0, 96, page);
    cache.setPage(1, 96, page);
    cache.setPage(2, 96, page);

    cache.setMaxBytes(pageBytes);
    QVERIFY(cache.currentBytes() <= pageBytes);
  }

  void removePageDropsEveryDpiVariant() {
    PdfPageCache cache(8);
    const QImage page = makeImage(8);

    cache.setPage(3, 96, page);
    cache.setPage(3, 300, page);
    cache.setPage(4, 96, page);

    cache.removePage(3);

    QVERIFY(!cache.hasPage(3, 96));
    QVERIFY(!cache.hasPage(3, 300));
    QVERIFY(cache.hasPage(4, 96));
  }

  void clearEmptiesTheCache() {
    PdfPageCache cache(4);
    cache.setPage(0, 96, makeImage(8));
    cache.clear();
    QVERIFY(!cache.hasPage(0, 96));

    // The byte accounting must be reset too: the cache still accepts pages.
    cache.setPage(0, 96, makeImage(8));
    QVERIFY(cache.hasPage(0, 96));
  }
};

QTEST_MAIN(TestPdfPageCache)
#include "test_pdf_page_cache.moc"
