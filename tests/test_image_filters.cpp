/**
 * @file test_image_filters.cpp
 * @brief Unit tests for ImageFilters (blur, sharpen).
 */
#include "../src/core/image_filters.h"
#include <QColor>
#include <QImage>
#include <QTest>

class TestImageFilters : public QObject {
  Q_OBJECT

private:
  QImage createSolidImage(int w, int h, QColor color) {
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(color);
    return img;
  }

private slots:
  void blurReturnsNullImageUnchanged() {
    QImage null;
    QImage result = ImageFilters::blur(null);
    QVERIFY(result.isNull());
  }

  void blurReturnsOriginalForBadRadius() {
    QImage img = createSolidImage(4, 4, Qt::red);
    QImage result = ImageFilters::blur(img, 0);
    QCOMPARE(result.size(), img.size());
    // With radius 0, should return original unchanged
    QImage a = result.convertToFormat(QImage::Format_ARGB32);
    QImage b = img.convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(a, b);
  }

  void blurPreservesSolidColor() {
    QImage img = createSolidImage(10, 10, Qt::blue);
    QImage result = ImageFilters::blur(img, 2);
    QCOMPARE(result.size(), img.size());
    // A solid-color image should remain the same after blur
    QRgb px = result.pixel(5, 5);
    QCOMPARE(qRed(px), 0);
    QCOMPARE(qGreen(px), 0);
    QCOMPARE(qBlue(px), 255);
  }

  void blurSmoothsSharpEdge() {
    // Create an image with a sharp black/white edge
    QImage img(20, 20, QImage::Format_ARGB32);
    for (int y = 0; y < 20; ++y)
      for (int x = 0; x < 20; ++x)
        img.setPixel(x, y, x < 10 ? qRgb(0, 0, 0) : qRgb(255, 255, 255));

    QImage result = ImageFilters::blur(img, 2);

    // At the boundary (x=10) the blurred pixel should be a mid-gray
    QRgb edge = result.pixel(10, 10);
    int r = qRed(edge);
    QVERIFY2(r > 50 && r < 200,
             qPrintable(QString("Expected mid-gray, got r=%1").arg(r)));
  }

  void sharpenReturnsNullImageUnchanged() {
    QImage null;
    QImage result = ImageFilters::sharpen(null);
    QVERIFY(result.isNull());
  }

  void sharpenPreservesSolidColor() {
    QImage img = createSolidImage(10, 10, Qt::green);
    QImage result = ImageFilters::sharpen(img, 2, 1.0);
    QCOMPARE(result.size(), img.size());
    QRgb px = result.pixel(5, 5);
    QCOMPARE(qRed(px), 0);
    QCOMPARE(qGreen(px), 255); // Qt::green is (0,255,0)
    QCOMPARE(qBlue(px), 0);
  }

  void sharpenEnhancesEdge() {
    // Create an image with a smooth gradient transition and sharpen it
    QImage img(20, 1, QImage::Format_ARGB32);
    for (int x = 0; x < 20; ++x) {
      int v = x < 10 ? 80 : 180;
      img.setPixel(x, 0, qRgb(v, v, v));
    }

    QImage result = ImageFilters::sharpen(img, 1, 1.5);
    // After sharpening, the dark side near the edge should be darker
    // and the light side should be lighter (more contrast at the edge)
    int darkSide = qRed(result.pixel(9, 0));
    int lightSide = qRed(result.pixel(10, 0));
    int origDark = qRed(img.pixel(9, 0));
    int origLight = qRed(img.pixel(10, 0));
    // The difference should be at least as large as original
    QVERIFY2((lightSide - darkSide) >= (origLight - origDark),
             qPrintable(
                 QString("Sharpened contrast %1 should be >= original %2")
                     .arg(lightSide - darkSide)
                     .arg(origLight - origDark)));
  }

  void blurOutputSameSize() {
    QImage img = createSolidImage(50, 30, Qt::cyan);
    QImage result = ImageFilters::blur(img, 5);
    QCOMPARE(result.width(), 50);
    QCOMPARE(result.height(), 30);
  }

  void sharpenOutputSameSize() {
    QImage img = createSolidImage(50, 30, Qt::cyan);
    QImage result = ImageFilters::sharpen(img, 5, 1.0);
    QCOMPARE(result.width(), 50);
    QCOMPARE(result.height(), 30);
  }
};

QTEST_MAIN(TestImageFilters)
#include "test_image_filters.moc"
