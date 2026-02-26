/**
 * @file image_filters.h
 * @brief Basic image filter functions (blur, sharpen).
 *
 * Pure functions that operate on QImage. No scene or item dependencies.
 */
#ifndef IMAGE_FILTERS_H
#define IMAGE_FILTERS_H

#include <QImage>

namespace ImageFilters {

/**
 * @brief Apply a box blur to an image.
 * @param source The source image
 * @param radius The blur radius (must be >= 1)
 * @return The blurred image, or the original if radius < 1 or image is null
 */
QImage blur(const QImage &source, int radius = 3);

/**
 * @brief Apply an unsharp-mask sharpen to an image.
 * @param source The source image
 * @param radius The blur radius used for the unsharp mask (must be >= 1)
 * @param strength Sharpening strength factor (typically 0.5 – 2.0)
 * @return The sharpened image, or the original if radius < 1 or image is null
 */
QImage sharpen(const QImage &source, int radius = 3, double strength = 1.0);

/**
 * @brief Options for the scanned document filter.
 */
struct ScanDocumentOptions {
  // Mode: false = enhance readability (default), true = artistic scan look
  bool hardBinarize = false;

  double threshold = 0.5; // 0.0–1.0 binarization / contrast strength
  int noiseLevel = 0;     // 0–10 noise intensity (0 = clean)
  bool sepiaEnabled = false;
  double sepiaStrength = 0.0; // 0.0–1.0
  bool vignetteEnabled = false;
  double vignetteStrength = 0.0; // 0.0–1.0
  double sharpenStrength = 1.5;  // text sharpening (0 = off, up to 3.0)
  double whitePoint = 0.9;       // background whitening aggressiveness 0–1
};

/**
 * @brief Apply a "scanned document" look to an image.
 *
 * Simulates the look of a flatbed/phone scanner: adaptive binarization,
 * subtle noise, warm paper tint, and edge vignette.
 *
 * @param source The source image
 * @param opts Configuration options
 * @return The processed image, or the original if image is null
 */
QImage scanDocument(const QImage &source,
                    const ScanDocumentOptions &opts = ScanDocumentOptions{});

/**
 * @brief Options for per-channel levels / curves adjustment.
 */
struct LevelsOptions {
  // Master channel (applied to R, G, B uniformly)
  int inputBlack = 0;   // 0–255
  int inputWhite = 255; // 0–255
  double gamma = 1.0;   // 0.1–10.0 (1.0 = linear)

  // Per-channel overrides (applied after master)
  int redInputBlack = 0;
  int redInputWhite = 255;
  double redGamma = 1.0;

  int greenInputBlack = 0;
  int greenInputWhite = 255;
  double greenGamma = 1.0;

  int blueInputBlack = 0;
  int blueInputWhite = 255;
  double blueGamma = 1.0;

  // Simple brightness / contrast on top
  int brightness = 0; // -100 – +100
  int contrast = 0;   // -100 – +100
};

/**
 * @brief Apply levels / curves adjustment to an image.
 *
 * Performs an input-levels remap (black point, white point, gamma) on
 * a master channel and optionally per R/G/B channel, then applies
 * brightness and contrast.
 *
 * @param source The source image
 * @param opts   Configuration options
 * @return The adjusted image, or the original if image is null
 */
QImage adjustLevels(const QImage &source,
                    const LevelsOptions &opts = LevelsOptions{});

/**
 * @brief High-quality Lanczos-3 image resize.
 *
 * Separable sinc-windowed resampling that preserves sharpness far better
 * than bilinear (Qt::SmoothTransformation) when down- or up-scaling.
 *
 * @param source The source image
 * @param newWidth Target width (must be >= 1)
 * @param newHeight Target height (must be >= 1)
 * @return The resized image, or the original on invalid parameters
 */
QImage lanczosResize(const QImage &source, int newWidth, int newHeight);

} // namespace ImageFilters

#endif // IMAGE_FILTERS_H
