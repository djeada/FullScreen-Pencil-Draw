/**
 * @file project_serializer.cpp
 * @brief Implementation of the native project file serializer.
 */
#include "project_serializer.h"
#include "../widgets/electronics_elements.h"
#include "../widgets/element_factory.h"
#include "../widgets/latex_text_item.h"
#include "../widgets/mermaid_text_item.h"
#include "../widgets/text_on_path_item.h"
#include "../widgets/wire_item.h"
#include "item_store.h"
#include "layer.h"
#include <QBuffer>
#include <QConicalGradient>
#include <QFile>
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
  return obj;
}

QPen ProjectSerializer::deserializePen(const QJsonObject &obj) {
  QPen pen;
  pen.setColor(QColor(obj["color"].toString()));
  pen.setWidthF(obj["width"].toDouble(1.0));
  pen.setStyle(static_cast<Qt::PenStyle>(obj["style"].toInt(1)));
  pen.setCapStyle(static_cast<Qt::PenCapStyle>(obj["capStyle"].toInt(0x00)));
  pen.setJoinStyle(static_cast<Qt::PenJoinStyle>(obj["joinStyle"].toInt(0x00)));
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

// Per-save identity for items that others refer to (wire endpoints).
QString itemKey(const QGraphicsItem *item) {
  return QString::number(reinterpret_cast<quintptr>(item), 16);
}

// Last resort for item types without a dedicated format (e.g. custom-brush
// strokes): keep their appearance as a bitmap instead of dropping them.
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
  QByteArray ba;
  QBuffer buf(&ba);
  buf.open(QIODevice::WriteOnly);
  image.save(&buf, "PNG");
  QJsonObject obj;
  obj["type"] = "pixmap";
  obj["data"] = QString::fromLatin1(ba.toBase64());
  obj["offsetX"] = bounds.x();
  obj["offsetY"] = bounds.y();
  obj["dpr"] = kScale;
  return obj;
}
} // namespace

QJsonObject ProjectSerializer::serializeItem(QGraphicsItem *item) {
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
  } else if (auto *group = dynamic_cast<QGraphicsItemGroup *>(item)) {
    // Arrows and user groups; children keep group-local coordinates.
    obj["type"] = "group";
    QJsonArray children;
    for (QGraphicsItem *child : group->childItems()) {
      QJsonObject childObj = serializeItem(child);
      if (!childObj.isEmpty())
        children.append(childObj);
    }
    obj["children"] = children;
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

    // Serialize path elements
    const QPainterPath &path = pathItem->path();
    QJsonArray elements;
    for (int i = 0; i < path.elementCount(); ++i) {
      QPainterPath::Element e = path.elementAt(i);
      QJsonObject el;
      el["type"] = static_cast<int>(e.type);
      el["x"] = e.x;
      el["y"] = e.y;
      elements.append(el);
    }
    obj["pathElements"] = elements;
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
    QPixmap pm = pixItem->pixmap();
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    pm.save(&buf, "PNG");
    obj["data"] = QString::fromLatin1(ba.toBase64());
    // Rasterized items (see rasterizeItem) are high-DPI and offset; keep
    // both or they come back at twice the size after the next save.
    obj["dpr"] = pm.devicePixelRatio();
    obj["offsetX"] = pixItem->offset().x();
    obj["offsetY"] = pixItem->offset().y();
  } else if (auto *textItem = dynamic_cast<QGraphicsTextItem *>(item)) {
    obj["type"] = "text";
    obj["html"] = textItem->toHtml();
    obj["defaultColor"] = textItem->defaultTextColor().name(QColor::HexArgb);
    QFont f = textItem->font();
    obj["fontFamily"] = f.family();
    obj["fontSize"] = f.pointSize();
    obj["fontBold"] = f.bold();
    obj["fontItalic"] = f.italic();
  } else if (auto *latexItem = dynamic_cast<LatexTextItem *>(item)) {
    if (latexItem->text().trimmed().isEmpty())
      return QJsonObject(); // invisible, unselectable leftovers
    obj["type"] = "latexText";
    obj["text"] = latexItem->text();
    obj["textColor"] = latexItem->textColor().name(QColor::HexArgb);
    QFont f = latexItem->font();
    obj["fontFamily"] = f.family();
    obj["fontSize"] = f.pointSize();
    obj["fontBold"] = f.bold();
    obj["fontItalic"] = f.italic();
  } else if (auto *pathTextItem = dynamic_cast<TextOnPathItem *>(item)) {
    obj["type"] = "textOnPath";
    obj["text"] = pathTextItem->text();
    obj["textColor"] = pathTextItem->textColor().name(QColor::HexArgb);
    QFont f = pathTextItem->font();
    obj["fontFamily"] = f.family();
    obj["fontSize"] = f.pointSize();
    obj["fontBold"] = f.bold();
    obj["fontItalic"] = f.italic();
    // Serialize the path
    const QPainterPath &path = pathTextItem->path();
    QJsonArray elements;
    for (int i = 0; i < path.elementCount(); ++i) {
      QPainterPath::Element e = path.elementAt(i);
      QJsonObject el;
      el["type"] = static_cast<int>(e.type);
      el["x"] = e.x;
      el["y"] = e.y;
      elements.append(el);
    }
    obj["pathElements"] = elements;
  } else {
    QJsonObject raster = rasterizeItem(item);
    if (raster.isEmpty())
      return QJsonObject();
    for (auto it = raster.constBegin(); it != raster.constEnd(); ++it)
      obj[it.key()] = it.value();
  }

  return obj;
}

QGraphicsItem *ProjectSerializer::deserializeItem(const QJsonObject &obj) {
  QString type = obj["type"].toString();
  QGraphicsItem *item = nullptr;

  if (type == "element") {
    item = createDiagramElement(obj["elementId"].toString());
    if (item)
      item->setData(kLoadedElementKeyData, obj["key"].toString());
  } else if (type == "group") {
    auto *group = new QGraphicsItemGroup();
    // Add children while the group is still at the origin with an identity
    // transform, so their group-local positions are kept as-is.
    for (const QJsonValue &cv : obj["children"].toArray()) {
      QGraphicsItem *child = deserializeItem(cv.toObject());
      if (!child)
        continue;
      child->setFlag(QGraphicsItem::ItemIsSelectable, false);
      child->setFlag(QGraphicsItem::ItemIsMovable, false);
      group->addToGroup(child);
    }
    item = group;
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
    QPainterPath path;
    QJsonArray elements = obj["pathElements"].toArray();
    for (int i = 0; i < elements.size(); ++i) {
      QJsonObject el = elements[i].toObject();
      int elType = el["type"].toInt();
      qreal ex = el["x"].toDouble();
      qreal ey = el["y"].toDouble();
      switch (elType) {
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
          QJsonObject d1 = elements[i + 1].toObject();
          c2x = d1["x"].toDouble();
          c2y = d1["y"].toDouble();
        }
        if (i + 2 < elements.size()) {
          QJsonObject d2 = elements[i + 2].toObject();
          epx = d2["x"].toDouble();
          epy = d2["y"].toDouble();
        }
        path.cubicTo(ex, ey, c2x, c2y, epx, epy);
        i += 2; // Skip the two CurveToDataElements
        break;
      }
      case QPainterPath::CurveToDataElement:
        // Consumed by CurveToElement handling above
        break;
      default:
        break;
      }
    }
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
    item = ellipseItem;
  } else if (type == "line") {
    QLineF l(obj["x1"].toDouble(), obj["y1"].toDouble(), obj["x2"].toDouble(),
             obj["y2"].toDouble());
    auto *lineItem = new QGraphicsLineItem(l);
    lineItem->setPen(deserializePen(obj["pen"].toObject()));
    item = lineItem;
  } else if (type == "pixmap") {
    QByteArray ba = QByteArray::fromBase64(obj["data"].toString().toLatin1());
    QPixmap pm;
    pm.loadFromData(ba, "PNG");
    pm.setDevicePixelRatio(obj["dpr"].toDouble(1.0));
    auto *pixItem = new QGraphicsPixmapItem(pm);
    pixItem->setOffset(obj["offsetX"].toDouble(), obj["offsetY"].toDouble());
    item = pixItem;
  } else if (type == "text") {
    auto *textItem = new QGraphicsTextItem();
    textItem->setHtml(obj["html"].toString());
    textItem->setDefaultTextColor(
        QColor(obj["defaultColor"].toString("#ff000000")));
    QFont f;
    f.setFamily(obj["fontFamily"].toString());
    f.setPointSize(obj["fontSize"].toInt(12));
    f.setBold(obj["fontBold"].toBool());
    f.setItalic(obj["fontItalic"].toBool());
    textItem->setFont(f);
    item = textItem;
  } else if (type == "latexText") {
    auto *latexItem = new LatexTextItem();
    latexItem->setText(obj["text"].toString());
    latexItem->setTextColor(QColor(obj["textColor"].toString("#ff000000")));
    QFont f;
    f.setFamily(obj["fontFamily"].toString());
    f.setPointSize(obj["fontSize"].toInt(12));
    f.setBold(obj["fontBold"].toBool());
    f.setItalic(obj["fontItalic"].toBool());
    latexItem->setFont(f);
    item = latexItem;
  } else if (type == "textOnPath") {
    auto *pathTextItem = new TextOnPathItem();
    pathTextItem->setText(obj["text"].toString());
    pathTextItem->setTextColor(QColor(obj["textColor"].toString("#ff000000")));
    QFont f;
    f.setFamily(obj["fontFamily"].toString());
    f.setPointSize(obj["fontSize"].toInt(12));
    f.setBold(obj["fontBold"].toBool());
    f.setItalic(obj["fontItalic"].toBool());
    pathTextItem->setFont(f);
    // Deserialize the path
    QPainterPath path;
    QJsonArray elements = obj["pathElements"].toArray();
    for (int i = 0; i < elements.size(); ++i) {
      QJsonObject el = elements[i].toObject();
      int elType = el["type"].toInt();
      qreal ex = el["x"].toDouble();
      qreal ey = el["y"].toDouble();
      switch (elType) {
      case QPainterPath::MoveToElement:
        path.moveTo(ex, ey);
        break;
      case QPainterPath::LineToElement:
        path.lineTo(ex, ey);
        break;
      case QPainterPath::CurveToElement: {
        qreal c2x = ex, c2y = ey, epx = ex, epy = ey;
        if (i + 1 < elements.size()) {
          QJsonObject d1 = elements[i + 1].toObject();
          c2x = d1["x"].toDouble();
          c2y = d1["y"].toDouble();
        }
        if (i + 2 < elements.size()) {
          QJsonObject d2 = elements[i + 2].toObject();
          epx = d2["x"].toDouble();
          epy = d2["y"].toDouble();
        }
        path.cubicTo(ex, ey, c2x, c2y, epx, epy);
        i += 2;
        break;
      }
      case QPainterPath::CurveToDataElement:
        break;
      default:
        break;
      }
    }
    pathTextItem->setPath(path);
    item = pathTextItem;
  }

  if (!item)
    return nullptr;

  // Apply common properties
  item->setPos(obj["x"].toDouble(), obj["y"].toDouble());
  item->setZValue(obj["z"].toDouble());
  item->setVisible(obj["visible"].toBool(true));
  item->setOpacity(obj["opacity"].toDouble(1.0));
  item->setTransform(deserializeTransform(obj["transform"].toObject()));

  // Make items interactive (unless they were saved locked)
  const bool locked = obj["locked"].toBool(false);
  if (locked)
    item->setData(0, "locked");
  item->setFlag(QGraphicsItem::ItemIsSelectable, !locked);
  item->setFlag(QGraphicsItem::ItemIsMovable, !locked);

  return item;
}

// ==================== Save / Load ====================

bool ProjectSerializer::saveProject(const QString &filePath,
                                    QGraphicsScene *scene, ItemStore *itemStore,
                                    LayerManager *layerManager,
                                    const QRectF &sceneRect,
                                    const QColor &backgroundColor) {
  if (!scene || !itemStore || !layerManager)
    return false;

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

  // Layers
  QJsonArray layersArray;
  for (int i = 0; i < layerManager->layerCount(); ++i) {
    Layer *layer = layerManager->layer(i);
    if (!layer)
      continue;

    QJsonObject layerObj;
    layerObj["name"] = layer->name();
    layerObj["visible"] = layer->isVisible();
    layerObj["locked"] = layer->isLocked();
    layerObj["opacity"] = layer->opacity();
    layerObj["type"] = static_cast<int>(layer->type());
    layerObj["blendMode"] = static_cast<int>(layer->blendMode());

    // Items in this layer
    QJsonArray itemsArray;
    const QList<ItemId> &ids = layer->itemIds();
    for (const ItemId &id : ids) {
      QGraphicsItem *gItem = itemStore->item(id);
      if (!gItem)
        continue;

      QJsonObject itemObj = serializeItem(gItem);
      if (!itemObj.isEmpty()) {
        itemsArray.append(itemObj);
      }
    }
    layerObj["items"] = itemsArray;
    layersArray.append(layerObj);
  }
  root["layers"] = layersArray;

  // Active layer
  root["activeLayer"] = layerManager->activeLayerIndex();

  // Write to file
  // QSaveFile only replaces the target once everything was written, so a
  // failed write (full disk, permissions) never truncates an existing file.
  QJsonDocument doc(root);
  QSaveFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  const QByteArray json = doc.toJson(QJsonDocument::Indented);
  if (file.write(json) != json.size()) {
    file.cancelWriting();
    return false;
  }
  return file.commit();
}

bool ProjectSerializer::loadProject(const QString &filePath,
                                    QGraphicsScene *scene, ItemStore *itemStore,
                                    LayerManager *layerManager,
                                    QRectF &sceneRect,
                                    QColor &backgroundColor) {
  if (!scene || !itemStore || !layerManager)
    return false;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
    return false;
  }

  QJsonObject root = doc.object();

  // Validate format
  int version = root["formatVersion"].toInt(0);
  if (version < 1 || version > FORMAT_VERSION) {
    return false;
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
  QJsonArray layersArray = root["layers"].toArray();
  bool firstLayer = true;
  for (const QJsonValue &lv : layersArray) {
    QJsonObject layerObj = lv.toObject();

    Layer *layer;
    if (firstLayer) {
      // Reuse the default layer created by clear()
      layer = layerManager->layer(0);
      if (layer) {
        layer->setName(layerObj["name"].toString("Layer"));
      }
      firstLayer = false;
    } else {
      layer = layerManager->createLayer(
          layerObj["name"].toString("Layer"),
          static_cast<Layer::Type>(layerObj["type"].toInt(0)));
    }

    if (!layer)
      continue;

    layer->setVisible(layerObj["visible"].toBool(true));
    layer->setLocked(layerObj["locked"].toBool(false));
    layer->setOpacity(layerObj["opacity"].toDouble(1.0));
    layer->setBlendMode(
        static_cast<Layer::BlendMode>(layerObj["blendMode"].toInt(0)));

    // Load items
    QJsonArray itemsArray = layerObj["items"].toArray();
    for (const QJsonValue &iv : itemsArray) {
      QJsonObject itemObj = iv.toObject();
      if (itemObj["type"].toString() == QLatin1String("wire")) {
        pendingWires.append({itemObj, layer});
        continue;
      }
      QGraphicsItem *gItem = deserializeItem(itemObj);
      if (gItem) {
        ItemId id = itemStore->registerItem(gItem);
        layer->addItem(id, itemStore);
        loadedItems.append(gItem);
      }
    }
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
    ItemId id = itemStore->registerItem(wire);
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

  return true;
}
