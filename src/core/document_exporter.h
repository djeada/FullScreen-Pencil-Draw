/**
 * @file document_exporter.h
 * @brief Format-aware export of hybrid vector + raster documents.
 *
 * One exporter renders the document for every target so the screen
 * preview, bitmap exports and vector exports agree on layer order,
 * visibility, opacity and blend modes. What each format can keep is
 * spelled out in capabilityMatrix() (and docs/EXPORT.md):
 *
 * - Native (.fspd): everything, editable (see ProjectSerializer).
 * - SVG / PDF (hybrid): paths, shapes, lines and plain text stay vectors;
 *   raster content (images, brush strokes, raster-layer tiles) is embedded
 *   as images at its own resolution. Items that only exist as rendered
 *   images (LaTeX text, Mermaid diagrams) are embedded as images and
 *   reported. Blend modes a format cannot express are reported, never
 *   silently approximated by flattening vectors.
 * - PNG / WebP / TIFF / BMP / JPEG (flattened): a deliberate composite
 *   rendered exactly like the canvas. JPEG and BMP have no alpha channel,
 *   so transparency is flattened onto white (reported).
 */
#ifndef DOCUMENT_EXPORTER_H
#define DOCUMENT_EXPORTER_H

#include "layer.h"
#include <QColor>
#include <QImage>
#include <QList>
#include <QRectF>
#include <QString>
#include <QStringList>

class QGraphicsItem;
class QGraphicsPixmapItem;
class QPainter;

enum class ExportFormat {
  Unknown,
  Native,
  Svg,
  Pdf,
  Png,
  Jpeg,
  WebP,
  Tiff,
  Bmp
};

struct ExportFormatInfo {
  ExportFormat format = ExportFormat::Unknown;
  QString name;
  QString kind; ///< "editable native", "hybrid vector+raster", "flattened"
  bool keepsVectors = false;
  bool keepsRaster = false;
  bool alpha = false;
  QString notes;
};

struct ExportResult {
  bool ok = false;
  QString error;
  QStringList warnings; ///< Content the format could not keep as-is
};

class DocumentExporter {
public:
  /// A document: its layers (bottom to top) and the optional base image
  /// that sits underneath every layer.
  DocumentExporter(LayerManager *layers, const QGraphicsPixmapItem *baseImage,
                   const QColor &background);

  /// Export only @p items (e.g. a selection) as one normal layer.
  void setItems(const QList<QGraphicsItem *> &items);
  /// Used as the export area when the document has no visible content.
  void setFallbackRect(const QRectF &rect) { fallbackRect_ = rect; }
  /// Fill behind everything; Qt::transparent for a transparent export.
  void setBackground(const QColor &color) { background_ = color; }

  static ExportFormat formatForFileName(const QString &fileName);
  static ExportFormatInfo formatInfo(ExportFormat format);
  static QList<ExportFormatInfo> capabilityMatrix();

  /// Visible content bounds (scene coordinates) plus @p margin.
  QRectF contentRect(qreal margin = 10) const;

  /**
   * @brief Flattened composite of @p source (scene coordinates) into an
   *        image of @p size, compositing layers with their blend modes
   *        exactly like the canvas does.
   */
  QImage renderImage(const QRectF &source, const QSize &size,
                     QImage::Format format = QImage::Format_ARGB32) const;

  /**
   * @brief Paint for a vector device (SVG, PDF): items are painted as
   *        themselves, so vector geometry stays vector and images are
   *        embedded. Unsupported appearance is appended to @p warnings.
   */
  void paintVector(QPainter *painter, const QRectF &target,
                   const QRectF &source, QStringList *warnings) const;

  /// Export to @p fileName (format from the extension unless given).
  ExportResult exportToFile(const QString &fileName,
                            ExportFormat format = ExportFormat::Unknown) const;

private:
  struct ExportLayer {
    QString name;
    Layer::BlendMode blendMode = Layer::BlendMode::Normal;
    QList<QGraphicsItem *> items; ///< Stacking order, visible only
  };
  QList<ExportLayer> exportLayers() const;
  static QStringList imageOnlyItems(const QList<ExportLayer> &layers);

  ExportResult exportRaster(const QString &fileName, ExportFormat format) const;
  ExportResult exportSvg(const QString &fileName) const;
  ExportResult exportPdf(const QString &fileName) const;

  LayerManager *layers_;
  const QGraphicsPixmapItem *baseImage_;
  QColor background_;
  bool useItems_ = false;
  QList<QGraphicsItem *> items_;
  QRectF fallbackRect_;
};

#endif // DOCUMENT_EXPORTER_H
