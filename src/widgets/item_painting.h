/**
 * @file item_painting.h
 * @brief Paint graphics items (with their children) outside of
 *        QGraphicsScene::render, e.g. for per-layer compositing or export.
 */
#ifndef ITEM_PAINTING_H
#define ITEM_PAINTING_H

#include <QGraphicsColorizeEffect>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

/**
 * @brief Pixmap tinted like QGraphicsColorizeEffect (grayscale, screened with
 *        the effect colour, blended with the original by 1 - strength).
 *
 * item->paint() bypasses graphics effects, and the fill tool tints images
 * with a colorize effect, so painting outside the scene has to redo it.
 */
inline QPixmap colorizedPixmap(const QPixmap &source,
                               const QGraphicsColorizeEffect &effect) {
  QImage gray =
      source.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
  const QImage original = gray;
  for (int y = 0; y < gray.height(); ++y) {
    auto *line = reinterpret_cast<QRgb *>(gray.scanLine(y));
    for (int x = 0; x < gray.width(); ++x) {
      const int alpha = qAlpha(line[x]);
      const int g = qGray(line[x]);
      line[x] = qRgba(g, g, g, alpha);
    }
  }
  {
    QPainter p(&gray);
    p.setCompositionMode(QPainter::CompositionMode_Screen);
    p.fillRect(gray.rect(), effect.color());
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(0, 0, original); // restore the source's alpha
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.setOpacity(1.0 - effect.strength());
    p.drawImage(0, 0, original);
  }
  QPixmap result = QPixmap::fromImage(gray);
  result.setDevicePixelRatio(source.devicePixelRatio());
  return result;
}

/**
 * @brief Paint @p item and its visible children in stacking order.
 * @param paintSelection Draw selection outlines (off for exports).
 * @param baseTransform Maps scene coordinates to the painter's device
 *        (e.g. the view's viewportTransform(), or identity for scene space).
 *
 * Unlike calling item->paint() directly this also draws the children of
 * groups (arrows, grouped shapes), which have no content of their own.
 */
inline void paintItemTree(QPainter *painter, QGraphicsItem *item,
                          const QTransform &baseTransform,
                          bool paintSelection = true) {
  if (!item || !item->isVisible())
    return;
  const QList<QGraphicsItem *> children = item->childItems(); // stack order
  for (QGraphicsItem *child : children) {
    if (child->flags() & QGraphicsItem::ItemStacksBehindParent)
      paintItemTree(painter, child, baseTransform, paintSelection);
  }
  if (!(item->flags() & QGraphicsItem::ItemHasNoContents)) {
    QStyleOptionGraphicsItem option;
    option.state = QStyle::State_None;
    if (paintSelection && item->isSelected())
      option.state |= QStyle::State_Selected;
    if (item->isEnabled())
      option.state |= QStyle::State_Enabled;
    option.exposedRect = item->boundingRect();
    painter->save();
    painter->setWorldTransform(item->sceneTransform() * baseTransform);
    painter->setOpacity(item->effectiveOpacity());
    auto *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
    auto *colorize =
        qobject_cast<QGraphicsColorizeEffect *>(item->graphicsEffect());
    if (pixmapItem && colorize && colorize->isEnabled()) {
      painter->setRenderHint(QPainter::SmoothPixmapTransform,
                             pixmapItem->transformationMode() ==
                                 Qt::SmoothTransformation);
      painter->drawPixmap(pixmapItem->offset(),
                          colorizedPixmap(pixmapItem->pixmap(), *colorize));
    } else {
      item->paint(painter, &option, nullptr);
    }
    painter->restore();
  }
  for (QGraphicsItem *child : children) {
    if (!(child->flags() & QGraphicsItem::ItemStacksBehindParent))
      paintItemTree(painter, child, baseTransform, paintSelection);
  }
}

#endif // ITEM_PAINTING_H
