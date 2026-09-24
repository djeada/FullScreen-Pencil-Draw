/**
 * @file document_exporter.cpp
 * @brief DocumentExporter implementation.
 */
#include "document_exporter.h"
#include "../widgets/item_painting.h"
#include "../widgets/latex_text_item.h"
#include "../widgets/mermaid_text_item.h"
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QImageWriter>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSaveFile>
#include <algorithm>
#ifdef HAVE_QT_SVG
#include <QSvgGenerator>
#endif

namespace {
QTransform sourceToTarget(const QRectF &source, const QRectF &target) {
  const qreal sx = source.width() > 0 ? target.width() / source.width() : 1;
  const qreal sy = source.height() > 0 ? target.height() / source.height() : 1;
  return QTransform()
      .translate(target.x(), target.y())
      .scale(sx, sy)
      .translate(-source.x(), -source.y());
}

QString blendModeName(Layer::BlendMode mode) {
  static const char *names[] = {"Normal",      "Multiply",   "Screen",
                                "Overlay",     "Darken",     "Lighten",
                                "Color Dodge", "Color Burn", "Hard Light",
                                "Soft Light",  "Difference", "Exclusion"};
  const int i = static_cast<int>(mode);
  return i >= 0 && i < 12 ? QString::fromLatin1(names[i]) : QString();
}
} // namespace

DocumentExporter::DocumentExporter(LayerManager *layers,
                                   const QGraphicsPixmapItem *baseImage,
                                   const QColor &background)
    : layers_(layers), baseImage_(baseImage), background_(background) {}

void DocumentExporter::setItems(const QList<QGraphicsItem *> &items) {
  useItems_ = true;
  items_ = items;
}

ExportFormat DocumentExporter::formatForFileName(const QString &fileName) {
  const QString ext = QFileInfo(fileName).suffix().toLower();
  if (ext == QLatin1String("fspd"))
    return ExportFormat::Native;
  if (ext == QLatin1String("svg"))
    return ExportFormat::Svg;
  if (ext == QLatin1String("pdf"))
    return ExportFormat::Pdf;
  if (ext == QLatin1String("png"))
    return ExportFormat::Png;
  if (ext == QLatin1String("jpg") || ext == QLatin1String("jpeg"))
    return ExportFormat::Jpeg;
  if (ext == QLatin1String("webp"))
    return ExportFormat::WebP;
  if (ext == QLatin1String("tif") || ext == QLatin1String("tiff"))
    return ExportFormat::Tiff;
  if (ext == QLatin1String("bmp"))
    return ExportFormat::Bmp;
  return ExportFormat::Unknown;
}

ExportFormatInfo DocumentExporter::formatInfo(ExportFormat format) {
  ExportFormatInfo info;
  info.format = format;
  switch (format) {
  case ExportFormat::Native:
    info.name = QStringLiteral("FullScreen Pencil Draw project (.fspd)");
    info.kind = QStringLiteral("editable native");
    info.keepsVectors = info.keepsRaster = info.alpha = true;
    info.notes = QStringLiteral(
        "Everything stays editable: layers (order, visibility, lock, "
        "opacity, blend mode), vector objects, text, groups and raster "
        "pixels.");
    break;
  case ExportFormat::Svg:
    info.name = QStringLiteral("SVG");
    info.kind = QStringLiteral("hybrid vector+raster");
    info.keepsVectors = info.keepsRaster = info.alpha = true;
    info.notes = QStringLiteral(
        "Paths, shapes and lines are vector elements; images, brush strokes "
        "and raster layers are embedded PNG images. LaTeX text and Mermaid "
        "diagrams are embedded as rendered images. Layer blend modes are "
        "not expressible and are exported as Normal (reported).");
    break;
  case ExportFormat::Pdf:
    info.name = QStringLiteral("PDF");
    info.kind = QStringLiteral("hybrid vector+raster");
    info.keepsVectors = info.keepsRaster = true;
    info.notes = QStringLiteral(
        "Vector geometry stays vector; raster content is embedded at its "
        "own resolution. The drawing is fitted onto an A4 page filled with "
        "the canvas colour. LaTeX text and Mermaid diagrams are embedded as "
        "rendered images. Layer blend modes are not written and are "
        "exported as Normal (reported).");
    break;
  case ExportFormat::Png:
  case ExportFormat::WebP:
  case ExportFormat::Tiff:
    info.name = format == ExportFormat::Png    ? QStringLiteral("PNG")
                : format == ExportFormat::WebP ? QStringLiteral("WebP")
                                               : QStringLiteral("TIFF");
    info.kind = QStringLiteral("flattened");
    info.alpha = true;
    info.notes = QStringLiteral(
        "A composite rendered exactly like the canvas (blend modes "
        "included). A transparent canvas colour stays transparent.");
    break;
  case ExportFormat::Jpeg:
  case ExportFormat::Bmp:
    info.name = format == ExportFormat::Jpeg ? QStringLiteral("JPEG")
                                             : QStringLiteral("BMP");
    info.kind = QStringLiteral("flattened");
    info.notes = QStringLiteral(
        "A composite rendered like the canvas. No alpha channel: "
        "transparent areas are flattened onto white (reported).");
    break;
  case ExportFormat::Unknown:
    info.name = QStringLiteral("Unknown");
    break;
  }
  return info;
}

QList<ExportFormatInfo> DocumentExporter::capabilityMatrix() {
  QList<ExportFormatInfo> matrix;
  for (ExportFormat f :
       {ExportFormat::Native, ExportFormat::Svg, ExportFormat::Pdf,
        ExportFormat::Png, ExportFormat::WebP, ExportFormat::Tiff,
        ExportFormat::Jpeg, ExportFormat::Bmp})
    matrix.append(formatInfo(f));
  return matrix;
}

QList<DocumentExporter::ExportLayer> DocumentExporter::exportLayers() const {
  QList<ExportLayer> result;
  auto byZ = [](QGraphicsItem *a, QGraphicsItem *b) {
    return a->zValue() < b->zValue();
  };
  if (useItems_) {
    ExportLayer layer;
    for (QGraphicsItem *item : items_)
      if (item && item->isVisible())
        layer.items.append(item);
    std::stable_sort(layer.items.begin(), layer.items.end(), byZ);
    result.append(layer);
    return result;
  }
  if (baseImage_ && baseImage_->isVisible()) {
    ExportLayer base;
    base.name = QStringLiteral("Base image");
    base.items.append(const_cast<QGraphicsPixmapItem *>(baseImage_));
    result.append(base);
  }
  if (!layers_)
    return result;
  for (int i = 0; i < layers_->layerCount(); ++i) {
    Layer *layer = layers_->layer(i);
    if (!layer || !layer->isVisible())
      continue;
    ExportLayer el;
    el.name = layer->name();
    el.blendMode = layer->blendMode();
    for (QGraphicsItem *item : layer->items())
      if (item && item->isVisible())
        el.items.append(item);
    std::stable_sort(el.items.begin(), el.items.end(), byZ);
    if (!el.items.isEmpty())
      result.append(el);
  }
  return result;
}

QRectF DocumentExporter::contentRect(qreal margin) const {
  QRectF bounds;
  for (const ExportLayer &layer : exportLayers())
    for (QGraphicsItem *item : layer.items)
      bounds = bounds.isNull() ? item->sceneBoundingRect()
                               : bounds.united(item->sceneBoundingRect());
  if (bounds.isEmpty())
    bounds = fallbackRect_.isEmpty() ? QRectF(0, 0, 1, 1) : fallbackRect_;
  return bounds.adjusted(-margin, -margin, margin, margin);
}

QImage DocumentExporter::renderImage(const QRectF &source, const QSize &size,
                                     QImage::Format format) const {
  if (size.isEmpty())
    return QImage();
  const QRectF target(QPointF(0, 0), QSizeF(size));
  const QTransform base = sourceToTarget(source, target);

  // Same scheme as Canvas::paintEvent: background, then every layer in
  // its own buffer composited with the layer's blend mode.
  QImage result(size, QImage::Format_ARGB32_Premultiplied);
  result.fill(background_);
  QPainter painter(&result);
  for (const ExportLayer &layer : exportLayers()) {
    QImage layerImage(size, QImage::Format_ARGB32_Premultiplied);
    layerImage.fill(Qt::transparent);
    {
      QPainter lp(&layerImage);
      lp.setRenderHint(QPainter::Antialiasing);
      lp.setRenderHint(QPainter::TextAntialiasing);
      lp.setRenderHint(QPainter::SmoothPixmapTransform);
      for (QGraphicsItem *item : layer.items)
        paintItemTree(&lp, item, base, /*paintSelection=*/false);
    }
    painter.setCompositionMode(Layer::toCompositionMode(layer.blendMode));
    painter.drawImage(0, 0, layerImage);
  }
  painter.end();
  return result.convertToFormat(format);
}

QStringList DocumentExporter::imageOnlyItems(const QList<ExportLayer> &layers) {
  int latex = 0;
  int mermaid = 0;
  for (const ExportLayer &layer : layers) {
    QList<QGraphicsItem *> stack = layer.items;
    while (!stack.isEmpty()) {
      QGraphicsItem *item = stack.takeLast();
      if (!item->isVisible())
        continue;
      if (dynamic_cast<LatexTextItem *>(item))
        ++latex;
      else if (dynamic_cast<MermaidTextItem *>(item))
        ++mermaid;
      stack.append(item->childItems());
    }
  }
  QStringList warnings;
  if (latex > 0)
    warnings << QStringLiteral("%1 text object(s) were embedded as rendered "
                               "images (LaTeX-capable text is rendered, not "
                               "outlined).")
                    .arg(latex);
  if (mermaid > 0)
    warnings << QStringLiteral("%1 Mermaid diagram(s) were embedded as "
                               "rendered images.")
                    .arg(mermaid);
  return warnings;
}

void DocumentExporter::paintVector(QPainter *painter, const QRectF &target,
                                   const QRectF &source,
                                   QStringList *warnings) const {
  if (!painter)
    return;
  const QList<ExportLayer> layers = exportLayers();
  const QTransform base =
      sourceToTarget(source, target) * painter->worldTransform();
  // Qt's SVG and PDF engines advertise blend-mode support but write
  // nothing for it, so non-Normal layers are always reported rather than
  // trusted (or silently flattened to pixels).
  QStringList unblended;
  for (const ExportLayer &layer : layers) {
    if (layer.blendMode != Layer::BlendMode::Normal)
      unblended << QStringLiteral("%1 (%2)").arg(
          layer.name, blendModeName(layer.blendMode));
    for (QGraphicsItem *item : layer.items)
      paintItemTree(painter, item, base, /*paintSelection=*/false);
  }
  if (!warnings)
    return;
  if (!unblended.isEmpty())
    warnings->append(QStringLiteral("This format cannot express layer blend "
                                    "modes; exported as Normal: %1.")
                         .arg(unblended.join(QStringLiteral(", "))));
  warnings->append(imageOnlyItems(layers));
}

ExportResult DocumentExporter::exportToFile(const QString &fileName,
                                            ExportFormat format) const {
  if (format == ExportFormat::Unknown)
    format = formatForFileName(fileName);
  switch (format) {
  case ExportFormat::Svg:
    return exportSvg(fileName);
  case ExportFormat::Pdf:
    return exportPdf(fileName);
  case ExportFormat::Png:
  case ExportFormat::Jpeg:
  case ExportFormat::WebP:
  case ExportFormat::Tiff:
  case ExportFormat::Bmp:
    return exportRaster(fileName, format);
  case ExportFormat::Native:
  case ExportFormat::Unknown:
    break;
  }
  ExportResult result;
  result.error = QStringLiteral("\"%1\" is not an export format; use the "
                                "project save for .fspd files.")
                     .arg(QFileInfo(fileName).suffix());
  return result;
}

ExportResult DocumentExporter::exportRaster(const QString &fileName,
                                            ExportFormat format) const {
  ExportResult result;
  const QRectF source = contentRect();
  const QSize size = source.size().toSize();
  const ExportFormatInfo info = formatInfo(format);
  QImage image = renderImage(source, size);
  if (!info.alpha && image.hasAlphaChannel()) {
    // Formats without alpha would otherwise turn transparency black.
    bool transparent = background_.alpha() < 255;
    QImage flat(image.size(), QImage::Format_RGB32);
    flat.fill(Qt::white);
    QPainter p(&flat);
    p.drawImage(0, 0, image);
    p.end();
    image = flat;
    if (transparent)
      result.warnings
          << QStringLiteral(
                 "%1 has no transparency; transparent areas were filled with "
                 "white.")
                 .arg(info.name);
  }
  const char *writerFormat = format == ExportFormat::Jpeg   ? "jpg"
                             : format == ExportFormat::WebP ? "webp"
                             : format == ExportFormat::Tiff ? "tiff"
                             : format == ExportFormat::Bmp  ? "bmp"
                                                            : "png";
  // Write through QSaveFile so a failed export never truncates an
  // existing file.
  QSaveFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) {
    result.error =
        QStringLiteral("Could not open \"%1\" for writing: %2")
            .arg(QDir::toNativeSeparators(fileName), file.errorString());
    return result;
  }
  QImageWriter writer(&file, writerFormat);
  if (!writer.write(image)) {
    file.cancelWriting();
    result.error =
        QStringLiteral("Could not write \"%1\": %2")
            .arg(QDir::toNativeSeparators(fileName), writer.errorString());
    return result;
  }
  if (!file.commit()) {
    result.error =
        QStringLiteral("Could not finish writing \"%1\": %2")
            .arg(QDir::toNativeSeparators(fileName), file.errorString());
    return result;
  }
  result.ok = true;
  return result;
}

ExportResult DocumentExporter::exportSvg(const QString &fileName) const {
  ExportResult result;
#ifndef HAVE_QT_SVG
  Q_UNUSED(fileName);
  result.error = QStringLiteral("SVG export requires the Qt SVG module, "
                                "which was not found at build time.");
  return result;
#else
  const QRectF source = contentRect();
  QByteArray svg;
  QBuffer buffer(&svg);
  buffer.open(QIODevice::WriteOnly);
  QSvgGenerator generator;
  generator.setOutputDevice(&buffer);
  generator.setSize(source.size().toSize());
  generator.setViewBox(QRectF(QPointF(), source.size()));
  generator.setTitle(QStringLiteral("FullScreen Pencil Draw Export"));
  generator.setDescription(
      QStringLiteral("Exported from FullScreen Pencil Draw"));
  QPainter painter;
  if (!painter.begin(&generator)) {
    result.error = QStringLiteral("Could not start the SVG export.");
    return result;
  }
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  if (background_.alpha() > 0)
    painter.fillRect(QRectF(QPointF(), source.size()), background_);
  paintVector(&painter, QRectF(QPointF(), source.size()), source,
              &result.warnings);
  painter.end();

  QSaveFile file(fileName);
  if (!file.open(QIODevice::WriteOnly) || file.write(svg) != svg.size() ||
      !file.commit()) {
    result.error =
        QStringLiteral("Could not write \"%1\": %2")
            .arg(QDir::toNativeSeparators(fileName), file.errorString());
    return result;
  }
  result.ok = true;
  return result;
#endif
}

ExportResult DocumentExporter::exportPdf(const QString &fileName) const {
  ExportResult result;
  const QRectF source = contentRect();
  QByteArray pdf;
  QBuffer buffer(&pdf);
  buffer.open(QIODevice::WriteOnly);
  {
    QPdfWriter pdfWriter(&buffer);
    // A4 with 10mm margins; the drawing is scaled to fit and centred.
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setPageMargins(QMarginsF(10, 10, 10, 10),
                             QPageLayout::Millimeter);
    pdfWriter.setTitle(QStringLiteral("FullScreen Pencil Draw Export"));
    pdfWriter.setCreator(QStringLiteral("FullScreen Pencil Draw"));
    pdfWriter.setResolution(300);

    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
      result.error = QStringLiteral("Could not start the PDF export.");
      return result;
    }
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRectF pageRect = painter.viewport();
    painter.fillRect(pageRect, background_);
    const double scale = qMin(pageRect.width() / source.width(),
                              pageRect.height() / source.height());
    const QSizeF fitted(source.width() * scale, source.height() * scale);
    const QRectF target(QPointF((pageRect.width() - fitted.width()) / 2.0,
                                (pageRect.height() - fitted.height()) / 2.0),
                        fitted);
    paintVector(&painter, target, source, &result.warnings);
    painter.end();
  }
  QSaveFile file(fileName);
  if (!file.open(QIODevice::WriteOnly) || file.write(pdf) != pdf.size() ||
      !file.commit()) {
    result.error =
        QStringLiteral("Could not write \"%1\": %2")
            .arg(QDir::toNativeSeparators(fileName), file.errorString());
    return result;
  }
  result.ok = true;
  return result;
}
