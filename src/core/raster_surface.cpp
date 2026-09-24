/**
 * @file raster_surface.cpp
 * @brief RasterSurface implementation.
 */
#include "raster_surface.h"
#include <QBuffer>
#include <QJsonObject>
#include <QPainter>
#include <QRadialGradient>
#include <QtMath>
#include <algorithm>
#include <climits>

namespace {
int floorDiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

bool isFullyTransparent(const QImage &image) {
  for (int y = 0; y < image.height(); ++y) {
    const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
    for (int x = 0; x < image.width(); ++x) {
      if (qAlpha(line[x]) != 0)
        return false;
    }
  }
  return true;
}
} // namespace

std::size_t RasterTileDiff::byteSize() const {
  std::size_t bytes = 0;
  for (const QImage &img : before)
    bytes += static_cast<std::size_t>(img.sizeInBytes());
  for (const QImage &img : after)
    bytes += static_cast<std::size_t>(img.sizeInBytes());
  return bytes;
}

QList<RasterTileKey> RasterSurface::tilesFor(const QRect &area) {
  QList<RasterTileKey> keys;
  if (area.isEmpty())
    return keys;
  const int tx0 = floorDiv(area.left(), kTileSize);
  const int ty0 = floorDiv(area.top(), kTileSize);
  const int tx1 = floorDiv(area.right(), kTileSize);
  const int ty1 = floorDiv(area.bottom(), kTileSize);
  for (int ty = ty0; ty <= ty1; ++ty)
    for (int tx = tx0; tx <= tx1; ++tx)
      keys.append(RasterTileKey(tx, ty));
  return keys;
}

void RasterSurface::recordBefore(const RasterTileKey &key) {
  if (!recording_ || touched_.contains(key))
    return;
  touched_.insert(key);
  // A null image records "no tile here before".
  pending_.before.insert(key, tiles_.value(key));
}

QImage &RasterSurface::tileForWrite(const RasterTileKey &key) {
  recordBefore(key);
  auto it = tiles_.find(key);
  if (it == tiles_.end()) {
    QImage tile(kTileSize, kTileSize, kFormat);
    tile.fill(Qt::transparent);
    it = tiles_.insert(key, tile);
  }
  return it.value();
}

void RasterSurface::dropIfTransparent(const RasterTileKey &key) {
  auto it = tiles_.find(key);
  if (it != tiles_.end() && isFullyTransparent(it.value()))
    tiles_.erase(it);
}

void RasterSurface::paint(const QRect &area,
                          const std::function<void(QPainter &)> &fn) {
  if (!fn)
    return;
  for (const RasterTileKey &key : tilesFor(area)) {
    const QRect tr = tileRect(key);
    QImage &tile = tileForWrite(key);
    {
      QPainter p(&tile);
      p.setRenderHint(QPainter::Antialiasing);
      p.translate(-tr.topLeft());
      p.setClipRect(area.intersected(tr));
      fn(p);
    }
    dropIfTransparent(key);
  }
}

void RasterSurface::erase(const QPainterPath &shape, qreal opacity,
                          qreal hardness, const QPainterPath &clip) {
  if (shape.isEmpty() || opacity <= 0.0)
    return;
  QRect area = shape.boundingRect().toAlignedRect();
  if (!clip.isEmpty())
    area = area.intersected(clip.boundingRect().toAlignedRect());
  if (area.isEmpty())
    return;
  // Only tiles that exist can lose pixels; never allocate for an erase.
  for (const RasterTileKey &key : tilesFor(area)) {
    if (!tiles_.contains(key))
      continue;
    const QRect tr = tileRect(key);
    QImage &tile = tileForWrite(key);
    {
      QPainter p(&tile);
      p.setRenderHint(QPainter::Antialiasing);
      p.translate(-tr.topLeft());
      if (!clip.isEmpty())
        p.setClipPath(clip);
      erasePath(p, shape, opacity, hardness);
    }
    dropIfTransparent(key);
  }
}

void RasterSurface::erasePath(QPainter &p, const QPainterPath &shape,
                              qreal opacity, qreal hardness) {
  p.setRenderHint(QPainter::Antialiasing);
  p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
  p.setPen(Qt::NoPen);
  const QColor core(0, 0, 0, qBound(0, qRound(opacity * 255), 255));
  if (hardness < 1.0) {
    // Soft edge: full strength up to `hardness` of the radius, then fade.
    const QRectF b = shape.boundingRect();
    const qreal radius = qMax(b.width(), b.height()) / 2.0;
    QRadialGradient g(b.center(), qMax<qreal>(radius, 0.5));
    g.setColorAt(0.0, core);
    g.setColorAt(qBound<qreal>(0.0, hardness, 1.0), core);
    g.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.setBrush(g);
  } else {
    p.setBrush(core);
  }
  p.drawPath(shape);
}

void RasterSurface::beginEdit() {
  recording_ = true;
  pending_ = RasterTileDiff();
  touched_.clear();
}

RasterTileDiff RasterSurface::endEdit() {
  recording_ = false;
  RasterTileDiff diff = std::move(pending_);
  pending_ = RasterTileDiff();
  for (const RasterTileKey &key : touched_)
    diff.after.insert(key, tiles_.value(key));
  touched_.clear();
  // Drop tiles the edit did not actually change (e.g. a dab over an empty
  // area that allocated and then released a tile).
  for (auto it = diff.before.begin(); it != diff.before.end();) {
    const QImage &a = diff.after.value(it.key());
    if (it.value() == a) {
      diff.after.remove(it.key());
      it = diff.before.erase(it);
    } else {
      ++it;
    }
  }
  return diff;
}

void RasterSurface::applyDiff(const RasterTileDiff &diff, bool forward) {
  const auto &state = forward ? diff.after : diff.before;
  for (auto it = state.constBegin(); it != state.constEnd(); ++it) {
    if (it.value().isNull())
      tiles_.remove(it.key());
    else
      tiles_.insert(it.key(), it.value());
  }
}

void RasterSurface::render(QPainter *painter, const QRectF &exposed) const {
  if (!painter)
    return;
  const QRect area = exposed.isNull()
                         ? boundingRect()
                         : exposed.toAlignedRect().adjusted(-1, -1, 1, 1);
  // Iterate the smaller set: allocated tiles or tiles in the exposed area.
  const QList<RasterTileKey> candidates = tilesFor(area);
  if (candidates.size() < tiles_.size()) {
    for (const RasterTileKey &key : candidates) {
      const auto it = tiles_.constFind(key);
      if (it != tiles_.constEnd())
        painter->drawImage(tileRect(key).topLeft(), it.value());
    }
  } else {
    for (auto it = tiles_.constBegin(); it != tiles_.constEnd(); ++it) {
      const QRect tr = tileRect(it.key());
      if (tr.intersects(area))
        painter->drawImage(tr.topLeft(), it.value());
    }
  }
}

void RasterSurface::setImage(const QImage &image, const QPoint &offset) {
  for (const RasterTileKey &key : tiles_.keys())
    recordBefore(key);
  tiles_.clear();
  if (image.isNull())
    return;
  const QImage src = image.convertToFormat(kFormat);
  const QRect area(offset, src.size());
  paint(area, [&](QPainter &p) {
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(offset, src);
  });
}

QImage RasterSurface::toImage(QPoint *origin) const {
  const QRect bounds = boundingRect();
  if (origin)
    *origin = bounds.topLeft();
  if (bounds.isEmpty())
    return QImage();
  QImage image(bounds.size(), kFormat);
  image.fill(Qt::transparent);
  QPainter p(&image);
  p.translate(-bounds.topLeft());
  render(&p, bounds);
  return image;
}

void RasterSurface::clear() {
  for (const RasterTileKey &key : tiles_.keys())
    recordBefore(key);
  tiles_.clear();
}

QRect RasterSurface::boundingRect() const {
  QRect bounds;
  for (auto it = tiles_.constBegin(); it != tiles_.constEnd(); ++it)
    bounds = bounds.united(tileRect(it.key()));
  return bounds;
}

std::size_t RasterSurface::memoryBytes() const {
  std::size_t bytes = 0;
  for (const QImage &tile : tiles_)
    bytes += static_cast<std::size_t>(tile.sizeInBytes());
  return bytes;
}

QRgb RasterSurface::pixel(const QPoint &p) const {
  const RasterTileKey key(floorDiv(p.x(), kTileSize),
                          floorDiv(p.y(), kTileSize));
  const auto it = tiles_.constFind(key);
  if (it == tiles_.constEnd())
    return qRgba(0, 0, 0, 0);
  const QPoint local = p - tileRect(key).topLeft();
  return it.value().pixel(local);
}

QJsonArray RasterSurface::toJson() const {
  QJsonArray array;
  // Stable order keeps saved files diffable and deterministic.
  QList<RasterTileKey> keys = tiles_.keys();
  std::sort(keys.begin(), keys.end(),
            [](const RasterTileKey &a, const RasterTileKey &b) {
              return a.y() != b.y() ? a.y() < b.y() : a.x() < b.x();
            });
  for (const RasterTileKey &key : keys) {
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    tiles_.value(key).save(&buf, "PNG");
    QJsonObject tile;
    tile["tx"] = key.x();
    tile["ty"] = key.y();
    tile["png"] = QString::fromLatin1(ba.toBase64());
    array.append(tile);
  }
  return array;
}

bool RasterSurface::fromJson(const QJsonArray &array, QString *error) {
  auto fail = [error](const QString &message) {
    if (error)
      *error = message;
    return false;
  };
  if (static_cast<std::size_t>(array.size()) > kMaxLoadedTiles)
    return fail(QStringLiteral("raster layer has too many tiles (%1)")
                    .arg(array.size()));
  constexpr int kMaxTile = kMaxExtent / kTileSize;
  QHash<RasterTileKey, QImage> loaded;
  for (const QJsonValue &value : array) {
    const QJsonObject obj = value.toObject();
    const int tx = obj["tx"].toInt(INT_MIN);
    const int ty = obj["ty"].toInt(INT_MIN);
    if (tx < -kMaxTile || tx > kMaxTile || ty < -kMaxTile || ty > kMaxTile)
      return fail(QStringLiteral("raster tile coordinates out of range"));
    QImage image;
    if (!image.loadFromData(
            QByteArray::fromBase64(obj["png"].toString().toLatin1()), "PNG"))
      return fail(QStringLiteral("raster tile (%1, %2) is not a valid PNG")
                      .arg(tx)
                      .arg(ty));
    if (image.size() != QSize(kTileSize, kTileSize))
      return fail(QStringLiteral("raster tile (%1, %2) has size %3x%4")
                      .arg(tx)
                      .arg(ty)
                      .arg(image.width())
                      .arg(image.height()));
    const RasterTileKey key(tx, ty);
    if (loaded.contains(key))
      return fail(
          QStringLiteral("duplicate raster tile (%1, %2)").arg(tx).arg(ty));
    loaded.insert(key, image.convertToFormat(kFormat));
  }
  tiles_ = std::move(loaded);
  return true;
}
