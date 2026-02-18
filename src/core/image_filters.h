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

} // namespace ImageFilters

#endif // IMAGE_FILTERS_H
