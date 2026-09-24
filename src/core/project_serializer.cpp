/**
 * @file project_serializer.cpp
 * @brief Implementation of the native project file serializer.
 */
#include "project_serializer.h"
#include "../widgets/brush_stroke_item.h"
#include "../widgets/electronics_elements.h"
#include "../widgets/element_factory.h"
#include "../widgets/latex_text_item.h"
#include "../widgets/mermaid_text_item.h"
#include "../widgets/raster_layer_item.h"
#include "../widgets/text_on_path_item.h"
#include "../widgets/wire_item.h"
#include "item_store.h"
#include "layer.h"
#include "raster_surface.h"
#include <QAbstractGraphicsShapeItem>
#include <QBuffer>
#include <QConicalGradient>
#include <QDir>
#include <QFile>
#include <QGraphicsColorizeEffect>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QSaveFile>
#include <QStyleOptionGraphicsItem>
#include <QtMath>

const QString ProjectSerializer::fileFilter() {
  return QStringLiteral("Project Files (*.fspd)");
}

// ==================== Pen / Brush / Transform helpers ====================

QJsonObject ProjectSerializer::serializePen(const QPen &pen) {
  QJsonObject obj;
  obj["color"] = pen.color().name(QColor::HexArgb);
  obj["width"] = pen.widthF();
  obj["style"] = static_cast<int>(pen.style());
  obj["capStyle"] = static_cast<int>(pen.capStyle());
  obj["joinStyle"] = static_cast<int>(pen.joinStyle());
  obj["miterLimit"] = pen.miterLimit();
  obj["cosmetic"] = pen.isCosmetic();
  if (pen.style() == Qt::CustomDashLine) {
    QJsonArray dashes;
    for (qreal d : pen.dashPattern())
      dashes.append(d);
    obj["dashPattern"] = dashes;
  }
  if (!qFuzzyIsNull(pen.dashOffset()))
    obj["dashOffset"] = pen.dashOffset();
  return obj;
}

QPen ProjectSerializer::deserializePen(const QJsonObject &obj) {
  QPen pen;
  pen.setColor(QColor(obj["color"].toString()));
  pen.setWidthF(obj["width"].toDouble(1.0));
  pen.setStyle(static_cast<Qt::PenStyle>(obj["style"].toInt(1)));
  pen.setCapStyle(static_cast<Qt::PenCapStyle>(obj["capStyle"].toInt(0x00)));
  pen.setJoinStyle(static_cast<Qt::PenJoinStyle>(obj["joinStyle"].toInt(0x00)));
  pen.setMiterLimit(obj["miterLimit"].toDouble(2.0));
  pen.setCosmetic(obj["cosmetic"].toBool(false));
  if (obj.contains("dashPattern")) {
    QVector<qreal> dashes;
    for (const QJsonValue &d : obj["dashPattern"].toArray())
      dashes.append(d.toDouble());
    if (!dashes.isEmpty() && dashes.size() % 2 == 0)
      pen.setDashPattern(dashes); // also sets Qt::CustomDashLine
  }
  if (obj.contains("dashOffset"))
    pen.setDashOffset(obj["dashOffset"].toDouble());
  return pen;
}

QJsonObject ProjectSerializer::serializeBrush(const QBrush &brush) {
  QJsonObject obj;
  obj["color"] = brush.color().name(QColor::HexArgb);
  obj["style"] = static_cast<int>(brush.style());

  const QGradient *gradient = brush.gradient();
  if (gradient) {
    QJsonObject gradObj;
    gradObj["type"] = static_cast<int>(gradient->type());
    gradObj["spread"] = static_cast<int>(gradient->spread());
    gradObj["coordinateMode"] = static_cast<int>(gradient->coordinateMode());

    QJsonArray stops;
    for (const QGradientStop &stop : gradient->stops()) {
      QJsonObject s;
      s["pos"] = stop.first;
      s["color"] = stop.second.name(QColor::HexArgb);
      stops.append(s);
    }
    gradObj["stops"] = stops;

    if (gradient->type() == QGradient::LinearGradient) {
      auto *lg = static_cast<const QLinearGradient *>(gradient);
      gradObj["x1"] = lg->start().x();
      gradObj["y1"] = lg->start().y();
      gradObj["x2"] = lg->finalStop().x();
      gradObj["y2"] = lg->finalStop().y();
    } else if (gradient->type() == QGradient::RadialGradient) {
      auto *rg = static_cast<const QRadialGradient *>(gradient);
      gradObj["cx"] = rg->center().x();
      gradObj["cy"] = rg->center().y();
      gradObj["fx"] = rg->focalPoint().x();
      gradObj["fy"] = rg->focalPoint().y();
      gradObj["radius"] = rg->radius();
    } else if (gradient->type() == QGradient::ConicalGradient) {
      auto *cg = static_cast<const QConicalGradient *>(gradient);
      gradObj["cx"] = cg->center().x();
      gradObj["cy"] = cg->center().y();
      gradObj["angle"] = cg->angle();
    }

    obj["gradient"] = gradObj;
  }

  return obj;
}

QBrush ProjectSerializer::deserializeBrush(const QJsonObject &obj) {
  int style = obj["style"].toInt(0);

  if (obj.contains("gradient")) {
    QJsonObject gradObj = obj["gradient"].toObject();
    int gradType = gradObj["type"].toInt(0);

    QGradientStops stops;
    QJsonArray stopsArr = gradObj["stops"].toArray();
    for (const QJsonValue &sv : stopsArr) {
      QJsonObject s = sv.toObject();
      stops.append({s["pos"].toDouble(), QColor(s["color"].toString())});
    }

    QGradient::Spread spread =
        static_cast<QGradient::Spread>(gradObj["spread"].toInt(0));
    QGradient::CoordinateMode coordMode =
        static_cast<QGradient::CoordinateMode>(
            gradObj["coordinateMode"].toInt(0));

    if (gradType == QGradient::LinearGradient) {
      QLinearGradient lg(gradObj["x1"].toDouble(), gradObj["y1"].toDouble(),
                         gradObj["x2"].toDouble(), gradObj["y2"].toDouble());
      lg.setStops(stops);
      lg.setSpread(spread);
      lg.setCoordinateMode(coordMode);
      return QBrush(lg);
    } else if (gradType == QGradient::RadialGradient) {
      QRadialGradient rg(gradObj["cx"].toDouble(), gradObj["cy"].toDouble(),
                         gradObj["radius"].toDouble(50.0),
                         gradObj["fx"].toDouble(), gradObj["fy"].toDouble());
      rg.setStops(stops);
      rg.setSpread(spread);
      rg.setCoordinateMode(coordMode);
      return QBrush(rg);
    } else if (gradType == QGradient::ConicalGradient) {
      QConicalGradient cg(gradObj["cx"].toDouble(), gradObj["cy"].toDouble(),
                          gradObj["angle"].toDouble());
      cg.setStops(stops);
      cg.setSpread(spread);
      cg.setCoordinateMode(coordMode);
      return QBrush(cg);
    }
  }

  QBrush brush;
  brush.setColor(QColor(obj["color"].toString()));
  brush.setStyle(static_cast<Qt::BrushStyle>(style));
  return brush;
}

QJsonObject ProjectSerializer::serializeTransform(const QTransform &t) {
  QJsonObject obj;
  obj["m11"] = t.m11();
  obj["m12"] = t.m12();
  obj["m13"] = t.m13();
  obj["m21"] = t.m21();
  obj["m22"] = t.m22();
  obj["m23"] = t.m23();
  obj["m31"] = t.m31();
  obj["m32"] = t.m32();
  obj["m33"] = t.m33();
  return obj;
}

QTransform ProjectSerializer::deserializeTransform(const QJsonObject &obj) {
  return QTransform(
      obj["m11"].toDouble(1), obj["m12"].toDouble(0), obj["m13"].toDouble(0),
      obj["m21"].toDouble(0), obj["m22"].toDouble(1), obj["m23"].toDouble(0),
      obj["m31"].toDouble(0), obj["m32"].toDouble(0), obj["m33"].toDouble(1));
}

// ==================== Item serialization ====================

namespace {
// QGraphicsItem::data() key that temporarily holds a loaded element's saved
// key until wires have been resolved against it.
constexpr int kLoadedElementKeyData = 0x4b4559; // "KEY"
// Holds a loaded top-level item's saved ItemId until it is registered.
constexpr int kLoadedItemIdData = 0x49444b; // "IDK"

ProjectSerializer::WriteFault g_writeFault =
    ProjectSerializer::WriteFault::None;

// Per-save identity for items that others refer to (wire endpoints).
QString itemKey(const QGraphicsItem *item) {
  return QString::number(reinterpret_cast<quintptr>(item), 16);
}

QJsonArray serializePath(const QPainterPath &path) {
  QJsonArray elements;
  for (int i = 0; i < path.elementCount(); ++i) {
    const QPainterPath::Element e = path.elementAt(i);
    QJsonObject el;
    el["type"] = static_cast<int>(e.type);
    el["x"] = e.x;
    el["y"] = e.y;
    elements.append(el);
  }
  return elements;
}

QPainterPath deserializePath(const QJsonArray &elements) {
  QPainterPath path;
  for (int i = 0; i < elements.size(); ++i) {
    const QJsonObject el = elements[i].toObject();
    const qreal ex = el["x"].toDouble();
    const qreal ey = el["y"].toDouble();
    switch (el["type"].toInt()) {
    case QPainterPath::MoveToElement:
      path.moveTo(ex, ey);
      break;
    case QPainterPath::LineToElement:
      path.lineTo(ex, ey);
      break;
    case QPainterPath::CurveToElement: {
      // CurveTo is followed by two CurveToDataElements
      qreal c2x = ex, c2y = ey, epx = ex, epy = ey;
      if (i + 1 < elements.size()) {
        const QJsonObject d1 = elements[i + 1].toObject();
        c2x = d1["x"].toDouble();
        c2y = d1["y"].toDouble();
      }
      if (i + 2 < elements.size()) {
        const QJsonObject d2 = elements[i + 2].toObject();
        epx = d2["x"].toDouble();
        epy = d2["y"].toDouble();
      }
      path.cubicTo(ex, ey, c2x, c2y, epx, epy);
      i += 2; // Skip the two CurveToDataElements
      break;
    }
    default: // CurveToDataElement is consumed above
      break;
    }
  }
  return path;
}

QString encodePng(const QImage &image) {
  QByteArray ba;
  QBuffer buf(&ba);
  buf.open(QIODevice::WriteOnly);
  image.save(&buf, "PNG");
  return QString::fromLatin1(ba.toBase64());
}

QImage decodePng(const QJsonValue &value) {
  QImage image;
  image.loadFromData(QByteArray::fromBase64(value.toString().toLatin1()),
                     "PNG");
  return image;
}

void serializeFont(QJsonObject &obj, const QFont &f) {
  // Individual fields stay for older readers; "font" is authoritative.
  obj["font"] = f.toString();
  obj["fontFamily"] = f.family();
  obj["fontSize"] = f.pointSize();
  obj["fontBold"] = f.bold();
  obj["fontItalic"] = f.italic();
}

QFont deserializeFont(const QJsonObject &obj) {
  QFont f;
  if (obj.contains("font") && f.fromString(obj["font"].toString()))
    return f;
  f.setFamily(obj["fontFamily"].toString());
  f.setPointSize(obj["fontSize"].toInt(12));
  f.setBold(obj["fontBold"].toBool());
  f.setItalic(obj["fontItalic"].toBool());
  return f;
}

// Fallback for item types without a dedicated format, only used with the
// caller's (user's) explicit consent: keep their appearance as a bitmap.
QJsonObject rasterizeItem(QGraphicsItem *item) {
  const QRectF bounds = item->boundingRect();
  if (bounds.isEmpty())
    return QJsonObject();
  constexpr qreal kScale = 2.0; // keep edges crisp when zoomed in
  const QSize size(qCeil(bounds.width() * kScale),
                   qCeil(bounds.height() * kScale));
  if (size.width() > 16384 || size.height() > 16384)
    return QJsonObject();
  QImage image(size, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(kScale, kScale);
    painter.translate(-bounds.topLeft());
    QStyleOptionGraphicsItem option;
    option.exposedRect = bounds;
    item->paint(&painter, &option, nullptr);
  }
  QJsonObject obj;
  obj["type"] = "pixmap";
  obj["data"] = encodePng(image);
  obj["offsetX"] = bounds.x();
  obj["offsetY"] = bounds.y();
  obj["dpr"] = kScale;
  obj["rasterizedFrom"] = ProjectSerializer::describeItem(item);
  return obj;
}
} // namespace

void ProjectSerializer::setWriteFaultForTesting(WriteFault fault) {
  g_writeFault = fault;
}

QString ProjectSerializer::describeItem(const QGraphicsItem *item) {
  if (!item)
    return QStringLiteral("(null item)");
  QString kind;
  if (dynamic_cast<const BrushStrokeItem *>(item))
    kind = QStringLiteral("brush stroke");
  else if (dynamic_cast<const RasterLayerItem *>(item))
    kind = QStringLiteral("raster pixels");
  else if (dynamic_cast<const QGraphicsItemGroup *>(item))
    kind = QStringLiteral("group");
  else if (dynamic_cast<const QGraphicsPixmapItem *>(item))
    kind = QStringLiteral("image");
  else if (dynamic_cast<const QGraphicsTextItem *>(item) ||
           dynamic_cast<const LatexTextItem *>(item))
    kind = QStringLiteral("text");
  else if (dynamic_cast<const QAbstractGraphicsShapeItem *>(item) ||
           dynamic_cast<const QGraphicsLineItem *>(item))
    kind = QStringLiteral("shape");
  else
    kind = QStringLiteral("object (type %1)").arg(item->type());
  const QPointF p = item->sceneBoundingRect().topLeft();
  return QStringLiteral("%1 at (%2, %3)")
      .arg(kind)
      .arg(qRound(p.x()))
      .arg(qRound(p.y()));
}

QJsonObject ProjectSerializer::serializeItem(QGraphicsItem *item,
                                             bool allowRasterFallback,
                                             QStringList *unsupported) {
  QJsonObject obj;
  if (!item)
    return obj;

  // Common properties
  obj["x"] = item->pos().x();
  obj["y"] = item->pos().y();
  obj["z"] = item->zValue();
  obj["visible"] = item->isVisible();
  obj["opacity"] = item->opacity();
  obj["transform"] = serializeTransform(item->transform());
  // QGraphicsItem keeps rotation/scale separately from transform().
  if (!qFuzzyIsNull(item->rotation()))
    obj["rotation"] = item->rotation();
  if (!qFuzzyCompare(item->scale(), 1.0))
    obj["scale"] = item->scale();
  if (!item->transformOriginPoint().isNull()) {
    obj["originX"] = item->transformOriginPoint().x();
    obj["originY"] = item->transformOriginPoint().y();
  }

  if (item->data(0).toString() == QLatin1String("locked"))
    obj["locked"] = true;

  auto *wire = dynamic_cast<WireItem *>(item);
  const QString elementId = diagramElementId(item);

  // Determine type and serialize type-specific data
  if (!elementId.isEmpty()) {
    obj["type"] = "element";
    obj["elementId"] = elementId;
    obj["key"] = itemKey(item);
  } else if (wire && wire->sourceElement() && wire->destElement() &&
             !wire->parentItem()) {
    // Resolved against the saved elements' keys once everything is loaded.
    obj["type"] = "wire";
    obj["srcKey"] = itemKey(wire->sourceElement());
    obj["srcPin"] = wire->sourcePin();
    obj["dstKey"] = itemKey(wire->destElement());
    obj["dstPin"] = wire->destPin();
    obj["pen"] = serializePen(wire->pen());
  } else if (auto *group = dynamic_cast<QGraphicsItemGroup *>(item)) {
    // Arrows and user groups; children keep group-local coordinates.
    obj["type"] = "group";
    QJsonArray children;
    for (QGraphicsItem *child : group->childItems()) {
      QJsonObject childObj =
          serializeItem(child, allowRasterFallback, unsupported);
      if (childObj.isEmpty()) {
        auto *latex = dynamic_cast<LatexTextItem *>(child);
        if (latex && latex->text().trimmed().isEmpty())
          continue;           // blank text leftovers carry nothing
        return QJsonObject(); // never save a group with missing members
      }
      children.append(childObj);
    }
    obj["children"] = children;
  } else if (auto *raster = dynamic_cast<RasterLayerItem *>(item)) {
    obj["type"] = "raster";
    obj["tileSize"] = RasterSurface::kTileSize;
    obj["tiles"] = raster->surface().toJson();
  } else if (auto *stroke = dynamic_cast<BrushStrokeItem *>(item)) {
    // Keep the exact pixels plus everything needed to know how they were
    // made (tip, size, colour, recorded points).
    obj["type"] = "brushStroke";
    const BrushTip &tip = stroke->tip();
    QJsonObject tipObj;
    tipObj["shape"] = static_cast<int>(tip.shape());
    tipObj["angle"] = tip.angle();
    tipObj["spacing"] = tip.stampSpacing();
    if (!tip.tipImage().isNull())
      tipObj["image"] = encodePng(tip.tipImage());
    obj["tip"] = tipObj;
    obj["size"] = stroke->brushSize();
    obj["color"] = stroke->color().name(QColor::HexArgb);
    obj["strokeOpacity"] = stroke->strokeOpacity();
    QJsonArray points;
    for (const QPointF &pt : stroke->points())
      points.append(QJsonArray{pt.x(), pt.y()});
    obj["points"] = points;
    const QRectF r = stroke->imageRect();
    obj["imageX"] = r.x();
    obj["imageY"] = r.y();
    if (!stroke->image().isNull())
      obj["image"] = encodePng(stroke->image());
  } else if (auto *polygonItem = dynamic_cast<QGraphicsPolygonItem *>(item)) {
    obj["type"] = "polygon";
    obj["pen"] = serializePen(polygonItem->pen());
    obj["brush"] = serializeBrush(polygonItem->brush());
    obj["fillRule"] = static_cast<int>(polygonItem->fillRule());
    QJsonArray points;
    for (const QPointF &pt : polygonItem->polygon()) {
      points.append(QJsonArray{pt.x(), pt.y()});
    }
    obj["points"] = points;
  } else if (auto *mermaidItem = dynamic_cast<MermaidTextItem *>(item)) {
    obj["type"] = "mermaid";
    obj["code"] = mermaidItem->mermaidCode();
    obj["theme"] = mermaidItem->theme();
  } else if (auto *pathItem = dynamic_cast<QGraphicsPathItem *>(item)) {
    obj["type"] = "path";
    obj["pen"] = serializePen(pathItem->pen());
    obj["brush"] = serializeBrush(pathItem->brush());
    obj["fillRule"] = static_cast<int>(pathItem->path().fillRule());
    obj["pathElements"] = serializePath(pathItem->path());
  } else if (auto *rectItem = dynamic_cast<QGraphicsRectItem *>(item)) {
    obj["type"] = "rect";
    obj["pen"] = serializePen(rectItem->pen());
    obj["brush"] = serializeBrush(rectItem->brush());
    QRectF r = rectItem->rect();
    obj["rx"] = r.x();
    obj["ry"] = r.y();
    obj["rw"] = r.width();
    obj["rh"] = r.height();
  } else if (auto *ellipseItem = dynamic_cast<QGraphicsEllipseItem *>(item)) {
    obj["type"] = "ellipse";
    obj["pen"] = serializePen(ellipseItem->pen());
    obj["brush"] = serializeBrush(ellipseItem->brush());
    QRectF r = ellipseItem->rect();
    obj["rx"] = r.x();
    obj["ry"] = r.y();
    obj["rw"] = r.width();
    obj["rh"] = r.height();
    if (ellipseItem->startAngle() != 0 || ellipseItem->spanAngle() != 5760) {
      obj["startAngle"] = ellipseItem->startAngle();
      obj["spanAngle"] = ellipseItem->spanAngle();
    }
  } else if (auto *lineItem = dynamic_cast<QGraphicsLineItem *>(item)) {
    obj["type"] = "line";
    obj["pen"] = serializePen(lineItem->pen());
    QLineF l = lineItem->line();
    obj["x1"] = l.x1();
    obj["y1"] = l.y1();
    obj["x2"] = l.x2();
    obj["y2"] = l.y2();
  } else if (auto *pixItem = dynamic_cast<QGraphicsPixmapItem *>(item)) {
    obj["type"] = "pixmap";
    // Encode pixmap as PNG in base64
    const QPixmap pm = pixItem->pixmap();
    obj["data"] = encodePng(pm.toImage());
    // Rasterized items (see rasterizeItem) are high-DPI and offset; keep
    // both or they come back at twice the size after the next save.
    obj["dpr"] = pm.devicePixelRatio();
    obj["offsetX"] = pixItem->offset().x();
    obj["offsetY"] = pixItem->offset().y();
    obj["transformationMode"] = static_cast<int>(pixItem->transformationMode());
    // The fill tool tints images with a colorize effect.
    if (auto *tint = qobject_cast<QGraphicsColorizeEffect *>(
            pixItem->graphicsEffect())) {
      QJsonObject tintObj;
      tintObj["color"] = tint->color().name(QColor::HexArgb);
      tintObj["strength"] = tint->strength();
      tintObj["enabled"] = tint->isEnabled();
      obj["tint"] = tintObj;
    }
  } else if (auto *textItem = dynamic_cast<QGraphicsTextItem *>(item)) {
    obj["type"] = "text";
    obj["html"] = textItem->toHtml();
    obj["defaultColor"] = textItem->defaultTextColor().name(QColor::HexArgb);
    obj["textWidth"] = textItem->textWidth();
    serializeFont(obj, textItem->font());
  } else if (auto *latexItem = dynamic_cast<LatexTextItem *>(item)) {
    if (latexItem->text().trimmed().isEmpty())
      return QJsonObject(); // invisible, unselectable leftovers
    obj["type"] = "latexText";
    obj["text"] = latexItem->text();
    obj["textColor"] = latexItem->textColor().name(QColor::HexArgb);
    serializeFont(obj, latexItem->font());
  } else if (auto *pathTextItem = dynamic_cast<TextOnPathItem *>(item)) {
    obj["type"] = "textOnPath";
    obj["text"] = pathTextItem->text();
    obj["textColor"] = pathTextItem->textColor().name(QColor::HexArgb);
    serializeFont(obj, pathTextItem->font());
    obj["pathElements"] = serializePath(pathTextItem->path());
  } else {
    // No editable representation: report it, and only flatten it into an
    // image when the caller (user) agreed to that.
    if (unsupported)
      unsupported->append(describeItem(item));
    if (!allowRasterFallback)
      return QJsonObject();
    QJsonObject raster = rasterizeItem(item);
    if (raster.isEmpty())
      return QJsonObject();
    for (auto it = raster.constBegin(); it != raster.constEnd(); ++it)
      obj[it.key()] = it.value();
  }

  return obj;
}

QGraphicsItem *ProjectSerializer::deserializeItem(const QJsonObject &obj,
                                                  QString *errorMessage) {
  const QString type = obj["type"].toString();
  QGraphicsItem *item = nullptr;
  auto fail = [&](const QString &message) -> QGraphicsItem * {
    if (errorMessage)
      *errorMessage = message;
    delete item;
    return nullptr;
  };

  if (type == "element") {
    item = createDiagramElement(obj["elementId"].toString());
    if (!item)
      return fail(QStringLiteral("unknown diagram element '%1'")
                      .arg(obj["elementId"].toString()));
    item->setData(kLoadedElementKeyData, obj["key"].toString());
  } else if (type == "group") {
    auto *group = new QGraphicsItemGroup();
    item = group;
    // Add children while the group is still at the origin with an identity
    // transform, so their group-local positions are kept as-is.
    for (const QJsonValue &cv : obj["children"].toArray()) {
      QString childError;
      QGraphicsItem *child = deserializeItem(cv.toObject(), &childError);
      if (!child) {
        if (!childError.isEmpty())
          return fail(childError);
        continue; // nested wires are not restorable on their own
      }
      child->setFlag(QGraphicsItem::ItemIsSelectable, false);
      child->setFlag(QGraphicsItem::ItemIsMovable, false);
      group->addToGroup(child);
    }
  } else if (type == "raster") {
    auto *raster = new RasterLayerItem();
    item = raster;
    if (obj["tileSize"].toInt(RasterSurface::kTileSize) !=
        RasterSurface::kTileSize)
      return fail(QStringLiteral("unsupported raster tile size %1")
                      .arg(obj["tileSize"].toInt()));
    QString tileError;
    if (!raster->surface().fromJson(obj["tiles"].toArray(), &tileError))
      return fail(tileError);
    raster->surfaceChanged();
  } else if (type == "brushStroke") {
    const QJsonObject tipObj = obj["tip"].toObject();
    BrushTip tip;
    tip.setShape(static_cast<BrushTipShape>(tipObj["shape"].toInt(0)));
    tip.setAngle(tipObj["angle"].toDouble(tip.angle()));
    tip.setStampSpacing(tipObj["spacing"].toDouble(tip.stampSpacing()));
    if (tipObj.contains("image"))
      tip.setTipImage(decodePng(tipObj["image"]));
    auto *stroke =
        new BrushStrokeItem(tip, obj["size"].toDouble(1.0),
                            QColor(obj["color"].toString("#ff000000")),
                            obj["strokeOpacity"].toDouble(1.0));
    item = stroke;
    QVector<QPointF> points;
    for (const QJsonValue &pv : obj["points"].toArray()) {
      const QJsonArray pt = pv.toArray();
      points.append(QPointF(pt.at(0).toDouble(), pt.at(1).toDouble()));
    }
    QImage image;
    if (obj.contains("image")) {
      image = decodePng(obj["image"]);
      if (image.isNull())
        return fail(QStringLiteral("brush stroke image is corrupt"));
    }
    stroke->restore(
        points, image,
        QRectF(QPointF(obj["imageX"].toDouble(), obj["imageY"].toDouble()),
               QSizeF(image.size())));
  } else if (type == "polygon") {
    QPolygonF polygon;
    for (const QJsonValue &pv : obj["points"].toArray()) {
      const QJsonArray pt = pv.toArray();
      polygon << QPointF(pt.at(0).toDouble(), pt.at(1).toDouble());
    }
    auto *polygonItem = new QGraphicsPolygonItem(polygon);
    polygonItem->setPen(deserializePen(obj["pen"].toObject()));
    polygonItem->setBrush(deserializeBrush(obj["brush"].toObject()));
    polygonItem->setFillRule(
        static_cast<Qt::FillRule>(obj["fillRule"].toInt(Qt::OddEvenFill)));
    item = polygonItem;
  } else if (type == "mermaid") {
    auto *mermaidItem = new MermaidTextItem();
    mermaidItem->setTheme(obj["theme"].toString(mermaidItem->theme()));
    mermaidItem->setMermaidCode(obj["code"].toString());
    item = mermaidItem;
  } else if (type == "path") {
    QPainterPath path = deserializePath(obj["pathElements"].toArray());
    path.setFillRule(
        static_cast<Qt::FillRule>(obj["fillRule"].toInt(Qt::OddEvenFill)));
    auto *pathItem = new QGraphicsPathItem(path);
    pathItem->setPen(deserializePen(obj["pen"].toObject()));
    pathItem->setBrush(deserializeBrush(obj["brush"].toObject()));
    item = pathItem;
  } else if (type == "rect") {
    QRectF r(obj["rx"].toDouble(), obj["ry"].toDouble(), obj["rw"].toDouble(),
             obj["rh"].toDouble());
    auto *rectItem = new QGraphicsRectItem(r);
    rectItem->setPen(deserializePen(obj["pen"].toObject()));
    rectItem->setBrush(deserializeBrush(obj["brush"].toObject()));
    item = rectItem;
  } else if (type == "ellipse") {
    QRectF r(obj["rx"].toDouble(), obj["ry"].toDouble(), obj["rw"].toDouble(),
             obj["rh"].toDouble());
    auto *ellipseItem = new QGraphicsEllipseItem(r);
    ellipseItem->setPen(deserializePen(obj["pen"].toObject()));
    ellipseItem->setBrush(deserializeBrush(obj["brush"].toObject()));
    if (obj.contains("spanAngle")) {
      ellipseItem->setStartAngle(obj["startAngle"].toInt());
      ellipseItem->setSpanAngle(obj["spanAngle"].toInt());
    }
    item = ellipseItem;
  } else if (type == "line") {
    QLineF l(obj["x1"].toDouble(), obj["y1"].toDouble(), obj["x2"].toDouble(),
             obj["y2"].toDouble());
    auto *lineItem = new QGraphicsLineItem(l);
    lineItem->setPen(deserializePen(obj["pen"].toObject()));
    item = lineItem;
  } else if (type == "pixmap") {
    const QImage image = decodePng(obj["data"]);
    if (image.isNull())
      return fail(QStringLiteral("embedded image is corrupt"));
    QPixmap pm = QPixmap::fromImage(image);
    pm.setDevicePixelRatio(obj["dpr"].toDouble(1.0));
    auto *pixItem = new QGraphicsPixmapItem(pm);
    pixItem->setOffset(obj["offsetX"].toDouble(), obj["offsetY"].toDouble());
    pixItem->setTransformationMode(static_cast<Qt::TransformationMode>(
        obj["transformationMode"].toInt(Qt::FastTransformation)));
    if (obj.contains("tint")) {
      const QJsonObject tintObj = obj["tint"].toObject();
      auto *tint = new QGraphicsColorizeEffect();
      tint->setColor(QColor(tintObj["color"].toString()));
      tint->setStrength(tintObj["strength"].toDouble(1.0));
      tint->setEnabled(tintObj["enabled"].toBool(true));
      pixItem->setGraphicsEffect(tint);
    }
    item = pixItem;
  } else if (type == "text") {
    auto *textItem = new QGraphicsTextItem();
    textItem->setHtml(obj["html"].toString());
    textItem->setDefaultTextColor(
        QColor(obj["defaultColor"].toString("#ff000000")));
    textItem->setFont(deserializeFont(obj));
    if (obj.contains("textWidth"))
      textItem->setTextWidth(obj["textWidth"].toDouble(-1));
    item = textItem;
  } else if (type == "latexText") {
    auto *latexItem = new LatexTextItem();
    latexItem->setText(obj["text"].toString());
    latexItem->setTextColor(QColor(obj["textColor"].toString("#ff000000")));
    latexItem->setFont(deserializeFont(obj));
    item = latexItem;
  } else if (type == "textOnPath") {
    auto *pathTextItem = new TextOnPathItem();
    pathTextItem->setText(obj["text"].toString());
    pathTextItem->setTextColor(QColor(obj["textColor"].toString("#ff000000")));
    pathTextItem->setFont(deserializeFont(obj));
    pathTextItem->setPath(deserializePath(obj["pathElements"].toArray()));
    item = pathTextItem;
  } else if (type == "wire") {
    return nullptr; // resolved by loadProject() once elements exist
  } else {
    return fail(QStringLiteral("unknown item type '%1'").arg(type));
  }

  // Apply common properties
  item->setPos(obj["x"].toDouble(), obj["y"].toDouble());
  item->setZValue(obj["z"].toDouble());
  item->setVisible(obj["visible"].toBool(true));
  item->setOpacity(obj["opacity"].toDouble(1.0));
  item->setTransform(deserializeTransform(obj["transform"].toObject()));
  if (obj.contains("originX") || obj.contains("originY"))
    item->setTransformOriginPoint(obj["originX"].toDouble(),
                                  obj["originY"].toDouble());
  if (obj.contains("rotation"))
    item->setRotation(obj["rotation"].toDouble());
  if (obj.contains("scale"))
    item->setScale(obj["scale"].toDouble(1.0));

  // Make items interactive (unless they were saved locked). Raster layer
  // content is edited with the raster tools, never dragged around.
  const bool locked = obj["locked"].toBool(false);
  if (locked)
    item->setData(0, "locked");
  const bool interactive = !locked && type != QLatin1String("raster");
  item->setFlag(QGraphicsItem::ItemIsSelectable, interactive);
  item->setFlag(QGraphicsItem::ItemIsMovable, interactive);

  return item;
}

QStringList
ProjectSerializer::findUnsupportedItems(ItemStore *itemStore,
                                        LayerManager *layerManager) {
  QStringList unsupported;
  if (!itemStore || !layerManager)
    return unsupported;
  for (int i = 0; i < layerManager->layerCount(); ++i) {
    Layer *layer = layerManager->layer(i);
    if (!layer)
      continue;
    for (const ItemId &id : layer->itemIds()) {
      if (QGraphicsItem *item = itemStore->item(id))
        serializeItem(item, /*allowRasterFallback=*/false, &unsupported);
    }
  }
  return unsupported;
}

// ==================== Save / Load ====================

QByteArray ProjectSerializer::serializeProject(ItemStore *itemStore,
                                               LayerManager *layerManager,
                                               const QRectF &sceneRect,
                                               const QColor &backgroundColor,
                                               const SaveOptions &options,
                                               SaveStatus *status) {
  SaveStatus localStatus;
  SaveStatus &st = status ? *status : localStatus;
  st = SaveStatus();
  if (!itemStore || !layerManager) {
    st.error = SaveError::InvalidArguments;
    st.message = QStringLiteral("No document to save.");
    return QByteArray();
  }

  QJsonObject root;
  root["formatVersion"] = FORMAT_VERSION;
  root["application"] = "FullScreenPencilDraw";

  // Canvas properties
  QJsonObject canvasObj;
  canvasObj["x"] = sceneRect.x();
  canvasObj["y"] = sceneRect.y();
  canvasObj["width"] = sceneRect.width();
  canvasObj["height"] = sceneRect.height();
  canvasObj["backgroundColor"] = backgroundColor.name(QColor::HexArgb);
  root["canvas"] = canvasObj;

  // The opened base image sits under every layer; losing it on save would
  // silently drop raster artwork.
  if (const QGraphicsPixmapItem *bg = options.backgroundImage) {
    QJsonObject bgObj;
    bgObj["data"] = encodePng(bg->pixmap().toImage());
    bgObj["dpr"] = bg->pixmap().devicePixelRatio();
    bgObj["x"] = bg->pos().x();
    bgObj["y"] = bg->pos().y();
    bgObj["z"] = bg->zValue();
    bgObj["visible"] = bg->isVisible();
    root["backgroundImage"] = bgObj;
  }

  // Layers
  QStringList unsupported;
  QJsonArray layersArray;
  for (int i = 0; i < layerManager->layerCount(); ++i) {
    Layer *layer = layerManager->layer(i);
    if (!layer)
      continue;

    QJsonObject layerObj;
    layerObj["id"] = layer->id().toString(QUuid::WithoutBraces);
    layerObj["name"] = layer->name();
    layerObj["visible"] = layer->isVisible();
    layerObj["locked"] = layer->isLocked();
    layerObj["opacity"] = layer->opacity();
    layerObj["type"] = static_cast<int>(layer->type());
    layerObj["blendMode"] = static_cast<int>(layer->blendMode());

    // Items in this layer, in stacking order
    QJsonArray itemsArray;
    for (const ItemId &id : layer->itemIds()) {
      QGraphicsItem *gItem = itemStore->item(id);
      if (!gItem)
        continue;

      const int before = unsupported.size();
      QJsonObject itemObj =
          serializeItem(gItem, options.allowRasterFallback, &unsupported);
      if (itemObj.isEmpty()) {
        // Only blank text leftovers may be skipped; everything else that
        // failed to serialize is reported.
        if (unsupported.size() == before &&
            !dynamic_cast<LatexTextItem *>(gItem))
          unsupported.append(describeItem(gItem));
        continue;
      }
      if (unsupported.size() > before)
        st.rasterizedItems.append(unsupported.mid(before));
      itemObj["id"] = id.toString();
      itemsArray.append(itemObj);
    }
    layerObj["items"] = itemsArray;
    layersArray.append(layerObj);
  }
  root["layers"] = layersArray;

  // Active layer
  root["activeLayer"] = layerManager->activeLayerIndex();

  if (!options.allowRasterFallback && !unsupported.isEmpty()) {
    st.error = SaveError::UnsupportedContent;
    st.unsupportedItems = unsupported;
    st.message = QStringLiteral("%1 object(s) have no editable project "
                                "format and would have to be flattened into "
                                "images:\n%2")
                     .arg(unsupported.size())
                     .arg(unsupported.mid(0, 10).join(QLatin1Char('\n')));
    return QByteArray();
  }

  const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
  // Validate before anything touches the disk.
  QJsonParseError parseError;
  QJsonDocument::fromJson(json, &parseError);
  if (json.isEmpty() || parseError.error != QJsonParseError::NoError) {
    st.error = SaveError::SerializationFailed;
    st.message = QStringLiteral("The document could not be serialized: %1")
                     .arg(parseError.errorString());
    return QByteArray();
  }
  return json;
}

bool ProjectSerializer::writeFileAtomically(const QString &filePath,
                                            const QByteArray &data,
                                            SaveStatus *status) {
  SaveStatus localStatus;
  SaveStatus &st = status ? *status : localStatus;
  const WriteFault fault = g_writeFault;

  // QSaveFile writes to a temporary file next to the target and renames it
  // over the target only on commit(), so a failed write (full disk,
  // permissions, crash) never truncates the existing file.
  QSaveFile file(filePath);
  if (fault == WriteFault::Open || !file.open(QIODevice::WriteOnly)) {
    st.error = SaveError::OpenFailed;
    st.message =
        QStringLiteral("Could not open \"%1\" for writing: %2")
            .arg(QDir::toNativeSeparators(filePath),
                 fault == WriteFault::Open ? QStringLiteral("simulated failure")
                                           : file.errorString());
    return false;
  }
  const qint64 toWrite =
      fault == WriteFault::ShortWrite ? data.size() / 2 : data.size();
  const qint64 written = file.write(data.constData(), toWrite);
  if (written != data.size()) {
    file.cancelWriting();
    st.error = SaveError::WriteFailed;
    st.message =
        QStringLiteral("Could not write \"%1\" (%2 of %3 bytes written): %4")
            .arg(QDir::toNativeSeparators(filePath))
            .arg(qMax<qint64>(written, 0))
            .arg(data.size())
            .arg(file.errorString().isEmpty() ? QStringLiteral("disk full?")
                                              : file.errorString());
    return false;
  }
  if (fault == WriteFault::Commit) {
    file.cancelWriting();
    file.commit(); // discards the temporary file
    st.error = SaveError::CommitFailed;
    st.message = QStringLiteral("Could not finish saving \"%1\": simulated "
                                "failure")
                     .arg(QDir::toNativeSeparators(filePath));
    return false;
  }
  if (!file.commit()) {
    st.error = SaveError::CommitFailed;
    st.message =
        QStringLiteral("Could not finish saving \"%1\": %2")
            .arg(QDir::toNativeSeparators(filePath), file.errorString());
    return false;
  }

  // Read back what is now on disk.
  QFile check(filePath);
  if (!check.open(QIODevice::ReadOnly) || check.readAll() != data) {
    st.error = SaveError::VerifyFailed;
    st.message = QStringLiteral("\"%1\" did not read back correctly after "
                                "saving; keep your work open and save to a "
                                "different location.")
                     .arg(QDir::toNativeSeparators(filePath));
    return false;
  }
  st.error = SaveError::None;
  return true;
}

bool ProjectSerializer::saveProject(const QString &filePath,
                                    QGraphicsScene *scene, ItemStore *itemStore,
                                    LayerManager *layerManager,
                                    const QRectF &sceneRect,
                                    const QColor &backgroundColor,
                                    const SaveOptions &options,
                                    SaveStatus *status) {
  SaveStatus localStatus;
  SaveStatus &st = status ? *status : localStatus;
  if (!scene || !itemStore || !layerManager || filePath.isEmpty()) {
    st = SaveStatus();
    st.error = SaveError::InvalidArguments;
    st.message = QStringLiteral("No document or file name to save.");
    return false;
  }
  const QByteArray json = serializeProject(itemStore, layerManager, sceneRect,
                                           backgroundColor, options, &st);
  if (!st.ok())
    return false;
  return writeFileAtomically(filePath, json, &st);
}

bool ProjectSerializer::loadProject(const QString &filePath,
                                    QGraphicsScene *scene, ItemStore *itemStore,
                                    LayerManager *layerManager,
                                    QRectF &sceneRect, QColor &backgroundColor,
                                    LoadExtras *extras, QString *errorMessage) {
  auto fail = [&](const QString &message) {
    if (errorMessage)
      *errorMessage = message;
    return false;
  };
  if (!scene || !itemStore || !layerManager)
    return fail(QStringLiteral("No document to load into."));

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly))
    return fail(
        QStringLiteral("Could not open \"%1\": %2")
            .arg(QDir::toNativeSeparators(filePath), file.errorString()));

  QByteArray data = file.readAll();
  file.close();

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    return fail(
        QStringLiteral("\"%1\" is not a valid project file (%2).")
            .arg(QDir::toNativeSeparators(filePath), parseError.errorString()));

  QJsonObject root = doc.object();

  // Validate format
  const int version = root["formatVersion"].toInt(0);
  if (version < 1 || version > FORMAT_VERSION)
    return fail(QStringLiteral("\"%1\" uses project format %2; this version "
                               "reads formats 1 to %3.")
                    .arg(QDir::toNativeSeparators(filePath))
                    .arg(version)
                    .arg(FORMAT_VERSION));

  // Build every item before touching the open document, so a corrupt file
  // cannot leave it half replaced.
  struct PendingLayer {
    QJsonObject obj;
    QList<QGraphicsItem *> items;
    QList<ItemId> savedIds;
    QList<QJsonObject> wires;
  };
  QList<PendingLayer> pendingLayers;
  auto discardPending = [&pendingLayers]() {
    for (PendingLayer &pl : pendingLayers)
      qDeleteAll(pl.items);
    pendingLayers.clear();
  };
  for (const QJsonValue &lv : root["layers"].toArray()) {
    PendingLayer pl;
    pl.obj = lv.toObject();
    for (const QJsonValue &iv : pl.obj["items"].toArray()) {
      const QJsonObject itemObj = iv.toObject();
      if (itemObj["type"].toString() == QLatin1String("wire")) {
        pl.wires.append(itemObj);
        continue;
      }
      QString itemError;
      QGraphicsItem *gItem = deserializeItem(itemObj, &itemError);
      if (!gItem) {
        pendingLayers.append(pl);
        discardPending();
        return fail(QStringLiteral("\"%1\" is damaged: %2")
                        .arg(QDir::toNativeSeparators(filePath),
                             itemError.isEmpty()
                                 ? QStringLiteral("unreadable item")
                                 : itemError));
      }
      pl.items.append(gItem);
      pl.savedIds.append(ItemId::fromString(itemObj["id"].toString()));
    }
    pendingLayers.append(pl);
  }

  ProjectLoadExtras loadedExtras;
  loadedExtras.formatVersion = version;
  if (root.contains("backgroundImage")) {
    const QJsonObject bgObj = root["backgroundImage"].toObject();
    const QImage image = decodePng(bgObj["data"]);
    if (image.isNull()) {
      discardPending();
      return fail(QStringLiteral("\"%1\" is damaged: the base image is "
                                 "unreadable.")
                      .arg(QDir::toNativeSeparators(filePath)));
    }
    loadedExtras.hasBackgroundImage = true;
    loadedExtras.backgroundImage = QPixmap::fromImage(image);
    loadedExtras.backgroundImage.setDevicePixelRatio(
        bgObj["dpr"].toDouble(1.0));
    loadedExtras.backgroundImagePos =
        QPointF(bgObj["x"].toDouble(), bgObj["y"].toDouble());
    loadedExtras.backgroundImageZ = bgObj["z"].toDouble(-1000);
  }

  // Clear existing state
  layerManager->clear();
  itemStore->clear();
  itemStore->flushDeletions();

  // Canvas properties
  QJsonObject canvasObj = root["canvas"].toObject();
  sceneRect = QRectF(canvasObj["x"].toDouble(), canvasObj["y"].toDouble(),
                     canvasObj["width"].toDouble(1920),
                     canvasObj["height"].toDouble(1080));
  backgroundColor = QColor(canvasObj["backgroundColor"].toString("#ffffffff"));
  scene->setSceneRect(sceneRect);

  // Layers
  struct PendingWire {
    QJsonObject obj;
    Layer *layer;
  };
  QList<PendingWire> pendingWires;
  QList<QGraphicsItem *> loadedItems;
  bool firstLayer = true;
  for (PendingLayer &pl : pendingLayers) {
    const QJsonObject &layerObj = pl.obj;
    const auto layerType = static_cast<Layer::Type>(layerObj["type"].toInt(0));

    Layer *layer;
    if (firstLayer) {
      // Reuse the default layer created by clear()
      layer = layerManager->layer(0);
      if (layer) {
        layer->setName(layerObj["name"].toString("Layer"));
        layer->setType(layerType);
      }
      firstLayer = false;
    } else {
      layer = layerManager->createLayer(layerObj["name"].toString("Layer"),
                                        layerType);
    }

    if (!layer) {
      qDeleteAll(pl.items);
      pl.items.clear();
      continue;
    }

    if (layerObj.contains("id"))
      layer->setId(QUuid::fromString(layerObj["id"].toString()));
    layer->setVisible(layerObj["visible"].toBool(true));
    layer->setLocked(layerObj["locked"].toBool(false));
    layer->setOpacity(layerObj["opacity"].toDouble(1.0));
    layer->setBlendMode(
        static_cast<Layer::BlendMode>(layerObj["blendMode"].toInt(0)));

    for (int i = 0; i < pl.items.size(); ++i) {
      QGraphicsItem *gItem = pl.items.at(i);
      const ItemId id = itemStore->registerItem(gItem, pl.savedIds.at(i));
      layer->addItem(id, itemStore);
      loadedItems.append(gItem);
    }
    pl.items.clear(); // owned by the scene now
    for (const QJsonObject &wireObj : pl.wires)
      pendingWires.append({wireObj, layer});
  }

  // Wires connect elements that may live in any layer (or inside a group),
  // so they are created once every element exists.
  QHash<QString, QGraphicsItem *> elementsByKey;
  QList<QGraphicsItem *> toVisit = loadedItems;
  while (!toVisit.isEmpty()) {
    QGraphicsItem *item = toVisit.takeLast();
    const QString key = item->data(kLoadedElementKeyData).toString();
    if (!key.isEmpty()) {
      elementsByKey.insert(key, item);
      item->setData(kLoadedElementKeyData, QVariant());
    }
    toVisit.append(item->childItems());
  }
  for (const PendingWire &pending : pendingWires) {
    const QJsonObject &obj = pending.obj;
    auto *src = dynamic_cast<ElectronicsElementItem *>(
        elementsByKey.value(obj["srcKey"].toString()));
    auto *dst = dynamic_cast<ElectronicsElementItem *>(
        elementsByKey.value(obj["dstKey"].toString()));
    if (!src || !dst)
      continue;
    auto *wire =
        new WireItem(src, obj["srcPin"].toInt(), dst, obj["dstPin"].toInt());
    wire->setZValue(obj["z"].toDouble(wire->zValue()));
    if (obj.contains("pen"))
      wire->setPen(deserializePen(obj["pen"].toObject()));
    ItemId id =
        itemStore->registerItem(wire, ItemId::fromString(obj["id"].toString()));
    wire->updatePath();
    pending.layer->addItem(id, itemStore);
  }

  // Saved z-values may come from an older layer spacing; derive them from
  // the loaded layer order so higher layers really draw on top.
  layerManager->updateLayerZOrder();

  // Restore active layer
  int activeIdx = root["activeLayer"].toInt(0);
  if (activeIdx >= 0 && activeIdx < layerManager->layerCount()) {
    layerManager->setActiveLayer(activeIdx);
  }

  if (extras)
    *extras = loadedExtras;
  return true;
}
