/**
 * @file project_serializer.cpp
 * @brief Implementation of the native project file serializer.
 */
#include "project_serializer.h"
#include "item_store.h"
#include "layer.h"
#include <QBuffer>
#include <QConicalGradient>
#include <QFile>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include "../widgets/latex_text_item.h"
#include "../widgets/text_on_path_item.h"

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

  // Determine type and serialize type-specific data
  if (auto *pathItem = dynamic_cast<QGraphicsPathItem *>(item)) {
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
    // Unsupported item type - skip
    return QJsonObject();
  }

  return obj;
}

QGraphicsItem *ProjectSerializer::deserializeItem(const QJsonObject &obj) {
  QString type = obj["type"].toString();
  QGraphicsItem *item = nullptr;

  if (type == "path") {
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
    auto *pixItem = new QGraphicsPixmapItem(pm);
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
    latexItem->setTextColor(
        QColor(obj["textColor"].toString("#ff000000")));
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
    pathTextItem->setTextColor(
        QColor(obj["textColor"].toString("#ff000000")));
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

  // Make items interactive
  item->setFlag(QGraphicsItem::ItemIsSelectable, true);
  item->setFlag(QGraphicsItem::ItemIsMovable, true);

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
  QJsonDocument doc(root);
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();
  return true;
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

    // Load items
    QJsonArray itemsArray = layerObj["items"].toArray();
    for (const QJsonValue &iv : itemsArray) {
      QJsonObject itemObj = iv.toObject();
      QGraphicsItem *gItem = deserializeItem(itemObj);
      if (gItem) {
        ItemId id = itemStore->registerItem(gItem);
        layer->addItem(id, itemStore);
      }
    }
  }

  // Restore active layer
  int activeIdx = root["activeLayer"].toInt(0);
  if (activeIdx >= 0 && activeIdx < layerManager->layerCount()) {
    layerManager->setActiveLayer(activeIdx);
  }

  return true;
}
