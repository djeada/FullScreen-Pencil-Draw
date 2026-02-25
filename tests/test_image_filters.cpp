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
    QVERIFY2(
        (lightSide - darkSide) >= (origLight - origDark),
        qPrintable(QString("Sharpened contrast %1 should be >= original %2")
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

  // --- adjustLevels tests ---

  void adjustLevelsReturnsNullImageUnchanged() {
    QImage null;
    QImage result = ImageFilters::adjustLevels(null);
    QVERIFY(result.isNull());
  }

  void adjustLevelsDefaultOptionsPreservesImage() {
    QImage img = createSolidImage(10, 10, QColor(100, 150, 200));
    QImage result = ImageFilters::adjustLevels(img);
    QCOMPARE(result.size(), img.size());
    QRgb px = result.pixel(5, 5);
    QCOMPARE(qRed(px), 100);
    QCOMPARE(qGreen(px), 150);
    QCOMPARE(qBlue(px), 200);
  }

  void adjustLevelsBlackPointClipsLow() {
    // With inputBlack = 100, values <= 100 should become 0
    QImage img = createSolidImage(4, 4, QColor(50, 50, 50));
    ImageFilters::LevelsOptions opts;
    opts.inputBlack = 100;
    QImage result = ImageFilters::adjustLevels(img, opts);
    QRgb px = result.pixel(2, 2);
    QCOMPARE(qRed(px), 0);
    QCOMPARE(qGreen(px), 0);
    QCOMPARE(qBlue(px), 0);
  }

  void adjustLevelsWhitePointClipsHigh() {
    // With inputWhite = 100, values >= 100 should become 255
    QImage img = createSolidImage(4, 4, QColor(150, 150, 150));
    ImageFilters::LevelsOptions opts;
    opts.inputWhite = 100;
    QImage result = ImageFilters::adjustLevels(img, opts);
    QRgb px = result.pixel(2, 2);
    QCOMPARE(qRed(px), 255);
    QCOMPARE(qGreen(px), 255);
    QCOMPARE(qBlue(px), 255);
  }

  void adjustLevelsGammaLightensMidtones() {
    // Gamma > 1.0 should lighten midtone values
    QImage img = createSolidImage(4, 4, QColor(128, 128, 128));
    ImageFilters::LevelsOptions opts;
    opts.gamma = 2.0;
    QImage result = ImageFilters::adjustLevels(img, opts);
    QRgb px = result.pixel(2, 2);
    // gamma 2.0 → pow(128/255, 0.5) ≈ 0.708 → ~181
    QVERIFY2(qRed(px) > 128,
             qPrintable(QString("Expected > 128, got %1").arg(qRed(px))));
  }

  void adjustLevelsGammaDarkensMidtones() {
    // Gamma < 1.0 should darken midtone values
    QImage img = createSolidImage(4, 4, QColor(128, 128, 128));
    ImageFilters::LevelsOptions opts;
    opts.gamma = 0.5;
    QImage result = ImageFilters::adjustLevels(img, opts);
    QRgb px = result.pixel(2, 2);
    QVERIFY2(qRed(px) < 128,
             qPrintable(QString("Expected < 128, got %1").arg(qRed(px))));
  }

  void adjustLevelsPerChannelWorks() {
    // Boost red gamma while leaving green/blue at defaults
    QImage img = createSolidImage(4, 4, QColor(128, 128, 128));
    ImageFilters::LevelsOptions opts;
    opts.redGamma = 2.0;
    QImage result = ImageFilters::adjustLevels(img, opts);
    QRgb px = result.pixel(2, 2);
    QVERIFY2(qRed(px) > qGreen(px),
             qPrintable(QString("Red %1 should be > Green %2")
                            .arg(qRed(px))
                            .arg(qGreen(px))));
    QCOMPARE(qGreen(px), 128);
    QCOMPARE(qBlue(px), 128);
  }

  void adjustLevelsBrightnessIncreasesValues() {
    QImage img = createSolidImage(4, 4, QColor(100, 100, 100));
    ImageFilters::LevelsOptions opts;
    opts.brightness = 50;
    QImage result = ImageFilters::adjustLevels(img, opts);
    QRgb px = result.pixel(2, 2);
    QVERIFY2(qRed(px) > 100,
             qPrintable(QString("Expected > 100, got %1").arg(qRed(px))));
  }

  void adjustLevelsContrastIncreasesRange() {
    // With positive contrast, darks get darker and lights get lighter
    QImage img(4, 1, QImage::Format_ARGB32);
    img.setPixel(0, 0, qRgb(50, 50, 50));
    img.setPixel(1, 0, qRgb(200, 200, 200));
    img.setPixel(2, 0, qRgb(50, 50, 50));
    img.setPixel(3, 0, qRgb(200, 200, 200));

    ImageFilters::LevelsOptions opts;
    opts.contrast = 50;
    QImage result = ImageFilters::adjustLevels(img, opts);
    int dark = qRed(result.pixel(0, 0));
    int light = qRed(result.pixel(1, 0));
    QVERIFY2(dark < 50,
             qPrintable(QString("Dark %1 should be < 50").arg(dark)));
    QVERIFY2(light > 200,
             qPrintable(QString("Light %1 should be > 200").arg(light)));
  }

  void adjustLevelsPreservesAlpha() {
    QImage img(4, 4, QImage::Format_ARGB32);
    img.fill(qRgba(100, 100, 100, 128));
    ImageFilters::LevelsOptions opts;
    opts.gamma = 2.0;
    QImage result = ImageFilters::adjustLevels(img, opts);
    QRgb px = result.pixel(2, 2);
    QCOMPARE(qAlpha(px), 128);
  }

  void adjustLevelsOutputSameSize() {
    QImage img = createSolidImage(50, 30, Qt::cyan);
    QImage result = ImageFilters::adjustLevels(img);
    QCOMPARE(result.width(), 50);
    QCOMPARE(result.height(), 30);
  }
};

QTEST_MAIN(TestImageFilters)
#include "test_image_filters.moc"
