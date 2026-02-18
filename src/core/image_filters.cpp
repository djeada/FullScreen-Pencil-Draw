/**
 * @file image_filters.cpp
 * @brief Implementation of basic image filters (blur, sharpen).
 */
#include "image_filters.h"

#include <algorithm>
#include <cmath>

namespace ImageFilters {

QImage blur(const QImage &source, int radius) {
  if (source.isNull() || radius < 1)
    return source;

  QImage img = source.convertToFormat(QImage::Format_ARGB32);
  const int w = img.width();
  const int h = img.height();
  if (w == 0 || h == 0)
    return source;

  QImage result(w, h, QImage::Format_ARGB32);
  const int side = 2 * radius + 1;
  const int area = side * side;

  for (int y = 0; y < h; ++y) {
    auto *dst = reinterpret_cast<QRgb *>(result.scanLine(y));
    for (int x = 0; x < w; ++x) {
      int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
      for (int ky = -radius; ky <= radius; ++ky) {
        int sy = std::clamp(y + ky, 0, h - 1);
        const auto *row = reinterpret_cast<const QRgb *>(img.constScanLine(sy));
        for (int kx = -radius; kx <= radius; ++kx) {
          int sx = std::clamp(x + kx, 0, w - 1);
          QRgb px = row[sx];
          aSum += qAlpha(px);
          rSum += qRed(px);
          gSum += qGreen(px);
          bSum += qBlue(px);
        }
      }
      dst[x] =
          qRgba(rSum / area, gSum / area, bSum / area, aSum / area);
    }
  }

  return result;
}

QImage sharpen(const QImage &source, int radius, double strength) {
  if (source.isNull() || radius < 1)
    return source;

  QImage blurred = blur(source, radius);
  QImage img = source.convertToFormat(QImage::Format_ARGB32);
  const int w = img.width();
  const int h = img.height();
  if (w == 0 || h == 0)
    return source;

  QImage result(w, h, QImage::Format_ARGB32);

  for (int y = 0; y < h; ++y) {
    const auto *srcRow = reinterpret_cast<const QRgb *>(img.constScanLine(y));
    const auto *blurRow =
        reinterpret_cast<const QRgb *>(blurred.constScanLine(y));
    auto *dstRow = reinterpret_cast<QRgb *>(result.scanLine(y));
    for (int x = 0; x < w; ++x) {
      int r = qRed(srcRow[x]) +
              static_cast<int>(strength *
                               (qRed(srcRow[x]) - qRed(blurRow[x])));
      int g = qGreen(srcRow[x]) +
              static_cast<int>(strength *
                               (qGreen(srcRow[x]) - qGreen(blurRow[x])));
      int b = qBlue(srcRow[x]) +
              static_cast<int>(strength *
                               (qBlue(srcRow[x]) - qBlue(blurRow[x])));
      int a = qAlpha(srcRow[x]);
      dstRow[x] = qRgba(std::clamp(r, 0, 255), std::clamp(g, 0, 255),
                         std::clamp(b, 0, 255), std::clamp(a, 0, 255));
    }
  }

  return result;
}

} // namespace ImageFilters
