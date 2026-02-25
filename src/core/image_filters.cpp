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

  // Separable box blur: horizontal pass then vertical pass.
  // Each pass uses a sliding-window running sum for O(w*h) total.
  QImage temp(w, h, QImage::Format_ARGB32);
  const int side = 2 * radius + 1;

  // --- Horizontal pass ---
  for (int y = 0; y < h; ++y) {
    const auto *srcRow = reinterpret_cast<const QRgb *>(img.constScanLine(y));
    auto *dstRow = reinterpret_cast<QRgb *>(temp.scanLine(y));

    int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
    // Initialize window for x=0
    for (int kx = -radius; kx <= radius; ++kx) {
      int sx = std::clamp(kx, 0, w - 1);
      QRgb px = srcRow[sx];
      rSum += qRed(px);
      gSum += qGreen(px);
      bSum += qBlue(px);
      aSum += qAlpha(px);
    }
    dstRow[0] = qRgba(rSum / side, gSum / side, bSum / side, aSum / side);

    for (int x = 1; x < w; ++x) {
      // Add the new right pixel, remove the old left pixel
      int addX = std::min(x + radius, w - 1);
      int remX = std::max(x - radius - 1, 0);
      QRgb addPx = srcRow[addX];
      QRgb remPx = srcRow[remX];
      rSum += qRed(addPx) - qRed(remPx);
      gSum += qGreen(addPx) - qGreen(remPx);
      bSum += qBlue(addPx) - qBlue(remPx);
      aSum += qAlpha(addPx) - qAlpha(remPx);
      dstRow[x] = qRgba(rSum / side, gSum / side, bSum / side, aSum / side);
    }
  }

  // --- Vertical pass ---
  QImage result(w, h, QImage::Format_ARGB32);
  for (int x = 0; x < w; ++x) {
    int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
    // Initialize window for y=0
    for (int ky = -radius; ky <= radius; ++ky) {
      int sy = std::clamp(ky, 0, h - 1);
      QRgb px = reinterpret_cast<const QRgb *>(temp.constScanLine(sy))[x];
      rSum += qRed(px);
      gSum += qGreen(px);
      bSum += qBlue(px);
      aSum += qAlpha(px);
    }
    reinterpret_cast<QRgb *>(result.scanLine(0))[x] =
        qRgba(rSum / side, gSum / side, bSum / side, aSum / side);

    for (int y = 1; y < h; ++y) {
      int addY = std::min(y + radius, h - 1);
      int remY = std::max(y - radius - 1, 0);
      QRgb addPx =
          reinterpret_cast<const QRgb *>(temp.constScanLine(addY))[x];
      QRgb remPx =
          reinterpret_cast<const QRgb *>(temp.constScanLine(remY))[x];
      rSum += qRed(addPx) - qRed(remPx);
      gSum += qGreen(addPx) - qGreen(remPx);
      bSum += qBlue(addPx) - qBlue(remPx);
      aSum += qAlpha(addPx) - qAlpha(remPx);
      reinterpret_cast<QRgb *>(result.scanLine(y))[x] =
          qRgba(rSum / side, gSum / side, bSum / side, aSum / side);
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
              static_cast<int>(strength * (qRed(srcRow[x]) - qRed(blurRow[x])));
      int g =
          qGreen(srcRow[x]) +
          static_cast<int>(strength * (qGreen(srcRow[x]) - qGreen(blurRow[x])));
      int b =
          qBlue(srcRow[x]) +
          static_cast<int>(strength * (qBlue(srcRow[x]) - qBlue(blurRow[x])));
      int a = qAlpha(srcRow[x]);
      dstRow[x] = qRgba(std::clamp(r, 0, 255), std::clamp(g, 0, 255),
                        std::clamp(b, 0, 255), std::clamp(a, 0, 255));
    }
  }

  return result;
}

QImage scanDocument(const QImage &source, const ScanDocumentOptions &opts) {
  if (source.isNull())
    return source;

  QImage img = source.convertToFormat(QImage::Format_ARGB32);
  const int w = img.width();
  const int h = img.height();
  if (w == 0 || h == 0)
    return source;

  // --- Step 1: Convert to grayscale luminance ---
  QImage gray(w, h, QImage::Format_ARGB32);
  for (int y = 0; y < h; ++y) {
    const auto *src = reinterpret_cast<const QRgb *>(img.constScanLine(y));
    auto *dst = reinterpret_cast<QRgb *>(gray.scanLine(y));
    for (int x = 0; x < w; ++x) {
      int lum =
          (qRed(src[x]) * 299 + qGreen(src[x]) * 587 + qBlue(src[x]) * 114) /
          1000;
      dst[x] = qRgba(lum, lum, lum, qAlpha(src[x]));
    }
  }

  // --- Step 2: Background estimation via large-radius blur ---
  const int blockRadius = std::clamp(std::min(w, h) / 20, 8, 60);
  QImage background = blur(gray, blockRadius);

  QImage processed(w, h, QImage::Format_ARGB32);

  if (opts.hardBinarize) {
    // ---- Hard binarization mode (artistic "scan look") ----
    const int bias = static_cast<int>((opts.threshold - 0.5) * 60);
    for (int y = 0; y < h; ++y) {
      const auto *grayRow =
          reinterpret_cast<const QRgb *>(gray.constScanLine(y));
      const auto *bgRow =
          reinterpret_cast<const QRgb *>(background.constScanLine(y));
      auto *dst = reinterpret_cast<QRgb *>(processed.scanLine(y));
      for (int x = 0; x < w; ++x) {
        int px = qRed(grayRow[x]);
        int localMean = qRed(bgRow[x]) - bias;
        int val = (px < localMean) ? 0 : 255;
        dst[x] = qRgba(val, val, val, qAlpha(grayRow[x]));
      }
    }
  } else {
    // ---- Document enhancement mode (default) ----
    // Like real scanner apps: normalize uneven lighting by dividing by the
    // local background, then apply a levels stretch to push background to
    // pure white and text to dark black.
    //
    // threshold controls where the black-point sits after normalization:
    //   0.0 → very light (only darkest ink survives)
    //   0.5 → balanced (default)
    //   1.0 → aggressive (more of the image becomes dark)
    double blackPoint = 0.55 - opts.threshold * 0.35; // range 0.20–0.55
    // White-point: how far below 1.0 we start pushing to white
    double whiteClip = 0.85 + (1.0 - opts.whitePoint) * 0.14; // ~0.85–0.99

    for (int y = 0; y < h; ++y) {
      const auto *grayRow =
          reinterpret_cast<const QRgb *>(gray.constScanLine(y));
      const auto *bgRow =
          reinterpret_cast<const QRgb *>(background.constScanLine(y));
      auto *dst = reinterpret_cast<QRgb *>(processed.scanLine(y));
      for (int x = 0; x < w; ++x) {
        double px = qRed(grayRow[x]) / 255.0;
        double bg = std::max(qRed(bgRow[x]) / 255.0, 0.01);

        // Normalize: divide by local background to remove uneven lighting
        double normalized = std::min(px / bg, 1.0);

        // Levels stretch: remap [blackPoint, whiteClip] → [0, 1]
        // Everything above whiteClip becomes pure white (background)
        // Everything below blackPoint becomes pure black (text)
        double stretched;
        if (normalized >= whiteClip) {
          stretched = 1.0;
        } else if (normalized <= blackPoint) {
          stretched = 0.0;
        } else {
          stretched = (normalized - blackPoint) / (whiteClip - blackPoint);
        }

        // Apply a gentle gamma curve to keep midtones readable
        // (gamma < 1 lightens midtones → cleaner paper look)
        stretched = std::pow(stretched, 0.7);

        int val = std::clamp(static_cast<int>(stretched * 255.0 + 0.5), 0, 255);
        dst[x] = qRgba(val, val, val, qAlpha(grayRow[x]));
      }
    }
  }

  // --- Step 3: Sharpen text edges ---
  if (opts.sharpenStrength > 0.01) {
    processed = sharpen(processed, 2, opts.sharpenStrength);
  }

  // --- Step 4: Noise (optional, 0 = clean) ---
  if (opts.noiseLevel > 0) {
    const int noiseMag = opts.noiseLevel;
    for (int y = 0; y < h; ++y) {
      auto *row = reinterpret_cast<QRgb *>(processed.scanLine(y));
      for (int x = 0; x < w; ++x) {
        unsigned int seed =
            static_cast<unsigned int>(x * 374761393u + y * 668265263u);
        seed = (seed ^ (seed >> 13)) * 1274126177u;
        int noise = static_cast<int>((seed & 0xFF)) - 128;
        noise = noise * noiseMag / 128;
        int v = std::clamp(qRed(row[x]) + noise, 0, 255);
        row[x] = qRgba(v, v, v, qAlpha(row[x]));
      }
    }
  }

  // --- Step 5: Sepia tint (optional) ---
  QImage result(w, h, QImage::Format_ARGB32);
  if (opts.sepiaEnabled && opts.sepiaStrength > 0.001) {
    double s = opts.sepiaStrength;
    for (int y = 0; y < h; ++y) {
      const auto *src =
          reinterpret_cast<const QRgb *>(processed.constScanLine(y));
      auto *dst = reinterpret_cast<QRgb *>(result.scanLine(y));
      for (int x = 0; x < w; ++x) {
        double t = qRed(src[x]) / 255.0;
        int gr = qRed(src[x]);
        int sepR = static_cast<int>(30 + t * (252 - 30));
        int sepG = static_cast<int>(25 + t * (248 - 25));
        int sepB = static_cast<int>(20 + t * (240 - 20));
        int r = static_cast<int>(gr + s * (sepR - gr));
        int g = static_cast<int>(gr + s * (sepG - gr));
        int b = static_cast<int>(gr + s * (sepB - gr));
        dst[x] = qRgba(std::clamp(r, 0, 255), std::clamp(g, 0, 255),
                        std::clamp(b, 0, 255), qAlpha(src[x]));
      }
    }
  } else {
    result = processed;
  }

  // --- Step 6: Vignette (optional) ---
  if (opts.vignetteEnabled && opts.vignetteStrength > 0.001) {
    const double cx = w / 2.0;
    const double cy = h / 2.0;
    const double maxDist = std::sqrt(cx * cx + cy * cy);
    const double darkMax = opts.vignetteStrength * 0.30;
    for (int y = 0; y < h; ++y) {
      auto *row = reinterpret_cast<QRgb *>(result.scanLine(y));
      for (int x = 0; x < w; ++x) {
        double dx = x - cx;
        double dy = y - cy;
        double dist = std::sqrt(dx * dx + dy * dy) / maxDist;
        double factor = 1.0 - std::max(0.0, (dist - 0.6) / 0.4) * darkMax;
        int r = static_cast<int>(qRed(row[x]) * factor);
        int g = static_cast<int>(qGreen(row[x]) * factor);
        int b = static_cast<int>(qBlue(row[x]) * factor);
        row[x] = qRgba(std::clamp(r, 0, 255), std::clamp(g, 0, 255),
                        std::clamp(b, 0, 255), qAlpha(row[x]));
      }
    }
  }

  return result;
}

// --- Lanczos-3 resize ---

static double lanczosKernel(double x) {
  constexpr int A = 3; // Lanczos window size
  if (x == 0.0)
    return 1.0;
  if (x < -A || x > A)
    return 0.0;
  double px = M_PI * x;
  return (A * std::sin(px) * std::sin(px / A)) / (px * px);
}

QImage lanczosResize(const QImage &source, int newWidth, int newHeight) {
  if (source.isNull() || newWidth < 1 || newHeight < 1)
    return source;

  QImage img = source.convertToFormat(QImage::Format_ARGB32);
  const int srcW = img.width();
  const int srcH = img.height();
  if (srcW == 0 || srcH == 0)
    return source;
  if (srcW == newWidth && srcH == newHeight)
    return img;

  constexpr int A = 3; // Lanczos window radius

  // --- Horizontal pass: srcW×srcH → newWidth×srcH ---
  QImage hPass(newWidth, srcH, QImage::Format_ARGB32);
  {
    const double xRatio = static_cast<double>(srcW) / newWidth;
    const double xFilterRadius = std::max(1.0, xRatio) * A;
    for (int y = 0; y < srcH; ++y) {
      const auto *srcRow =
          reinterpret_cast<const QRgb *>(img.constScanLine(y));
      auto *dstRow = reinterpret_cast<QRgb *>(hPass.scanLine(y));
      for (int x = 0; x < newWidth; ++x) {
        double center = (x + 0.5) * xRatio - 0.5;
        int left = static_cast<int>(std::floor(center - xFilterRadius));
        int right = static_cast<int>(std::ceil(center + xFilterRadius));
        left = std::max(left, 0);
        right = std::min(right, srcW - 1);

        double rSum = 0, gSum = 0, bSum = 0, aSum = 0, wSum = 0;
        for (int sx = left; sx <= right; ++sx) {
          double dist = (sx - center) / std::max(1.0, xRatio);
          double w = lanczosKernel(dist);
          QRgb px = srcRow[sx];
          rSum += qRed(px) * w;
          gSum += qGreen(px) * w;
          bSum += qBlue(px) * w;
          aSum += qAlpha(px) * w;
          wSum += w;
        }
        if (wSum > 0) {
          rSum /= wSum;
          gSum /= wSum;
          bSum /= wSum;
          aSum /= wSum;
        }
        dstRow[x] =
            qRgba(std::clamp(static_cast<int>(rSum + 0.5), 0, 255),
                   std::clamp(static_cast<int>(gSum + 0.5), 0, 255),
                   std::clamp(static_cast<int>(bSum + 0.5), 0, 255),
                   std::clamp(static_cast<int>(aSum + 0.5), 0, 255));
      }
    }
  }

  // --- Vertical pass: newWidth×srcH → newWidth×newHeight ---
  QImage result(newWidth, newHeight, QImage::Format_ARGB32);
  {
    const double yRatio = static_cast<double>(srcH) / newHeight;
    const double yFilterRadius = std::max(1.0, yRatio) * A;
    for (int x = 0; x < newWidth; ++x) {
      for (int y = 0; y < newHeight; ++y) {
        double center = (y + 0.5) * yRatio - 0.5;
        int top = static_cast<int>(std::floor(center - yFilterRadius));
        int bottom = static_cast<int>(std::ceil(center + yFilterRadius));
        top = std::max(top, 0);
        bottom = std::min(bottom, srcH - 1);

        double rSum = 0, gSum = 0, bSum = 0, aSum = 0, wSum = 0;
        for (int sy = top; sy <= bottom; ++sy) {
          double dist = (sy - center) / std::max(1.0, yRatio);
          double w = lanczosKernel(dist);
          QRgb px =
              reinterpret_cast<const QRgb *>(hPass.constScanLine(sy))[x];
          rSum += qRed(px) * w;
          gSum += qGreen(px) * w;
          bSum += qBlue(px) * w;
          aSum += qAlpha(px) * w;
          wSum += w;
        }
        if (wSum > 0) {
          rSum /= wSum;
          gSum /= wSum;
          bSum /= wSum;
          aSum /= wSum;
        }
        reinterpret_cast<QRgb *>(result.scanLine(y))[x] =
            qRgba(std::clamp(static_cast<int>(rSum + 0.5), 0, 255),
                   std::clamp(static_cast<int>(gSum + 0.5), 0, 255),
                   std::clamp(static_cast<int>(bSum + 0.5), 0, 255),
                   std::clamp(static_cast<int>(aSum + 0.5), 0, 255));
      }
    }
  }

  return result;
}

} // namespace ImageFilters
