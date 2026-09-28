/**
 * @file raster_surface.h
 * @brief Sparse, tiled pixel storage for editable raster layers.
 *
 * A RasterSurface owns the pixels of one raster layer in its own coordinate
 * space (one unit = one pixel), independent of any QGraphicsScene. Pixels
 * are kept in fixed-size tiles that are only allocated where something was
 * painted, so a small edit touches (and an undo step stores) only the tiles
 * it overlaps instead of a copy of the whole logical image.
 */
#ifndef RASTER_SURFACE_H
#define RASTER_SURFACE_H

#include <QHash>
#include <QImage>
#include <QJsonArray>
#include <QPainterPath>
#include <QPoint>
#include <QRect>
#include <QSet>
#include <QString>
#include <QtGlobal>
#include <cstddef>
#include <functional>

class QPainter;

/// Tile coordinates (tile column / row, not pixels).
using RasterTileKey = QPoint;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// Qt 6 ships qHash(QPoint); Qt 5 does not.
inline uint qHash(const QPoint &p, uint seed = 0) {
  return ::qHash((static_cast<quint64>(static_cast<quint32>(p.x())) << 32) |
                     static_cast<quint32>(p.y()),
                 seed);
}
#endif

/**
 * @brief Tiles touched by one edit, before and after it.
 *
 * A null image means "no tile" (the tile did not exist before, or became
 * fully transparent and was dropped). Only touched tiles are stored.
 */
struct RasterTileDiff {
  QHash<RasterTileKey, QImage> before;
  QHash<RasterTileKey, QImage> after;

  bool isEmpty() const { return before.isEmpty() && after.isEmpty(); }
  /// Approximate heap usage of the stored pixels.
  std::size_t byteSize() const;
};

class RasterSurface {
public:
  static constexpr int kTileSize = 256;
  /// Pixel format of every tile.
  static constexpr QImage::Format kFormat = QImage::Format_ARGB32_Premultiplied;
  /// Hard limits for loaded data, so a corrupt file cannot request absurd
  /// allocations.
  static constexpr int kMaxExtent = 1 << 20; ///< |pixel coordinate| bound
  static constexpr std::size_t kMaxLoadedTiles = 16384; ///< ~4 GiB at 256²

  RasterSurface() = default;

  /// Paint with @p painter clipped to @p area (surface pixels). The painter
  /// works in surface coordinates. Only tiles overlapping @p area are
  /// allocated/changed.
  void paint(const QRect &area, const std::function<void(QPainter &)> &fn);

  /**
   * @brief Remove alpha under @p shape (surface coordinates).
   * @param opacity 0..1 amount of alpha removed at the shape's core.
   * @param hardness 0..1; below 1 the edge of an ellipse dab fades out.
   * @param clip Optional clip (surface coordinates), e.g. a selection.
   */
  void erase(const QPainterPath &shape, qreal opacity = 1.0,
             qreal hardness = 1.0, const QPainterPath &clip = QPainterPath());

  /**
   * @brief Remove alpha under @p shape with @p painter (any QImage device).
   *
   * Shared by tiles and image items so both erasers behave the same.
   * The painter's composition mode and brush are changed.
   */
  static void erasePath(QPainter &painter, const QPainterPath &shape,
                        qreal opacity, qreal hardness);

  /// Start recording touched tiles; endEdit() returns the diff.
  void beginEdit();
  RasterTileDiff endEdit();
  bool isRecording() const { return recording_; }

  /// Apply one side of a diff (undo: @p forward = false).
  void applyDiff(const RasterTileDiff &diff, bool forward);

  /// Draw the tiles intersecting @p exposed (surface coordinates).
  void render(QPainter *painter, const QRectF &exposed) const;

  /// Replace the contents with @p image placed at @p offset.
  void setImage(const QImage &image, const QPoint &offset = QPoint());
  /// Composite of every tile, cropped to boundingRect(); @p origin receives
  /// the top-left surface coordinate of the returned image.
  QImage toImage(QPoint *origin = nullptr) const;

  void clear();
  bool isEmpty() const { return tiles_.isEmpty(); }
  int tileCount() const { return tiles_.size(); }
  QList<RasterTileKey> tileKeys() const { return tiles_.keys(); }
  QImage tile(const RasterTileKey &key) const { return tiles_.value(key); }
  /// Surface pixel bounds covered by allocated tiles.
  QRect boundingRect() const;
  std::size_t memoryBytes() const;
  QRgb pixel(const QPoint &p) const;

  static QRect tileRect(const RasterTileKey &key) {
    return QRect(key.x() * kTileSize, key.y() * kTileSize, kTileSize,
                 kTileSize);
  }
  static QList<RasterTileKey> tilesFor(const QRect &area);

  /// Serialize as an array of {tx, ty, png(base64)} objects.
  QJsonArray toJson() const;
  /// Replace the contents from toJson() output; validates tile sizes,
  /// coordinates and count. On failure the surface is left unchanged.
  bool fromJson(const QJsonArray &tiles, QString *error = nullptr);

private:
  QImage &tileForWrite(const RasterTileKey &key);
  void recordBefore(const RasterTileKey &key);
  void dropIfTransparent(const RasterTileKey &key);

  QHash<RasterTileKey, QImage> tiles_;
  bool recording_ = false;
  RasterTileDiff pending_;
  QSet<RasterTileKey> touched_;
};

#endif // RASTER_SURFACE_H
