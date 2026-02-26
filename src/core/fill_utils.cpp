/**
 * @file fill_utils.cpp
 * @brief Shared fill operations for canvas and tool-based renderers.
 */
#include "fill_utils.h"
#include "../widgets/latex_text_item.h"
#include "../widgets/mermaid_text_item.h"
#include "action.h"
#include "item_store.h"
#include <QAbstractGraphicsShapeItem>
#include <QGraphicsColorizeEffect>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>
#include <QSet>
#include <QtGlobal>
#include <vector>

namespace {
QString mermaidThemeForColor(const QColor &color) {
  if (color.saturation() < 40) {
    return "neutral";
  }
  if (color.lightness() < 110) {
    return "dark";
  }
  const int hue = color.hue();
  if (hue >= 70 && hue <= 170) {
    return "forest";
  }
  return "default";
}

QGraphicsItem *resolveFillTarget(QGraphicsItem *item) {
  if (!item) {
    return nullptr;
  }

  QGraphicsItem *target = item;
  for (QGraphicsItem *parent = target->parentItem(); parent;
       parent = target->parentItem()) {
    if (dynamic_cast<QGraphicsItemGroup *>(parent)) {
      target = parent;
    } else {
      break;
    }
  }
  return target;
}

std::unique_ptr<Action>
collapseActions(std::vector<std::unique_ptr<Action>> actions) {
  if (actions.empty()) {
    return nullptr;
  }
  if (actions.size() == 1) {
    return std::move(actions.front());
  }

  auto composite = std::make_unique<CompositeAction>();
  for (auto &action : actions) {
    composite->addAction(std::move(action));
  }
  return composite;
}

FillAction::PixmapTintState currentTintState(QGraphicsPixmapItem *pixmap) {
  FillAction::PixmapTintState state;
  if (!pixmap) {
    return state;
  }
  if (auto *effect =
          dynamic_cast<QGraphicsColorizeEffect *>(pixmap->graphicsEffect())) {
    state.enabled = true;
    state.color = effect->color();
    state.strength = effect->strength();
  }
  return state;
}

bool sameTintState(const FillAction::PixmapTintState &a,
                   const FillAction::PixmapTintState &b) {
  return a.enabled == b.enabled && a.color == b.color &&
         qFuzzyCompare(a.strength + 1.0, b.strength + 1.0);
}

void applyTintState(QGraphicsPixmapItem *pixmap,
                    const FillAction::PixmapTintState &state) {
  if (!pixmap) {
    return;
  }

  if (!state.enabled) {
    pixmap->setGraphicsEffect(nullptr);
    return;
  }

  auto *effect =
      dynamic_cast<QGraphicsColorizeEffect *>(pixmap->graphicsEffect());
  if (!effect) {
    effect = new QGraphicsColorizeEffect();
    pixmap->setGraphicsEffect(effect);
  }
  effect->setColor(state.color);
  effect->setStrength(state.strength);
}

bool applyFillToItem(QGraphicsItem *item, ItemStore *store, const QBrush &brush,
                     std::unique_ptr<Action> &outAction) {
  outAction.reset();
  if (!item) {
    return false;
  }

  const QColor color = brush.color();

  if (auto *group = dynamic_cast<QGraphicsItemGroup *>(item)) {
    std::vector<std::unique_ptr<Action>> groupActions;
    bool changed = false;
    const QList<QGraphicsItem *> children = group->childItems();
    for (QGraphicsItem *child : children) {
      std::unique_ptr<Action> childAction;
      if (applyFillToItem(child, store, brush, childAction)) {
        changed = true;
        if (childAction) {
          groupActions.push_back(std::move(childAction));
        }
      }
    }
    outAction = collapseActions(std::move(groupActions));
    return changed;
  }

  ItemId itemId;
  bool itemIdResolved = false;
  auto resolveItemId = [&]() -> ItemId {
    if (!itemIdResolved) {
      itemIdResolved = true;
      if (store) {
        itemId = store->idForItem(item);
        if (!itemId.isValid()) {
          itemId = store->registerItem(item);
        }
      }
    }
    return itemId;
  };

  std::vector<std::unique_ptr<Action>> actions;

  if (auto *polygon = dynamic_cast<QGraphicsPolygonItem *>(item)) {
    bool changed = false;

    const QBrush oldBrush = polygon->brush();
    if (oldBrush != brush) {
      polygon->setBrush(brush);
      changed = true;
      ItemId id = resolveItemId();
      if (store && id.isValid()) {
        actions.push_back(
            std::make_unique<FillAction>(id, store, oldBrush, brush));
      }
    }

    const QPen oldPen = polygon->pen();
    QPen newPen = oldPen;
    newPen.setColor(color);
    if (oldPen != newPen) {
      polygon->setPen(newPen);
      changed = true;
      ItemId id = resolveItemId();
      if (store && id.isValid()) {
        actions.push_back(
            std::make_unique<FillAction>(id, store, oldPen, newPen));
      }
    }

    outAction = collapseActions(std::move(actions));
    return changed;
  }

  if (auto *line = dynamic_cast<QGraphicsLineItem *>(item)) {
    const QPen oldPen = line->pen();
    QPen newPen = oldPen;
    newPen.setColor(color);
    if (oldPen == newPen) {
      return false;
    }
    line->setPen(newPen);
    ItemId id = resolveItemId();
    if (store && id.isValid()) {
      outAction = std::make_unique<FillAction>(id, store, oldPen, newPen);
    }
    return true;
  }

  if (auto *path = dynamic_cast<QGraphicsPathItem *>(item)) {
    const QPen oldPen = path->pen();
    QPen newPen = oldPen;
    newPen.setColor(color);
    if (oldPen == newPen) {
      return false;
    }
    path->setPen(newPen);
    ItemId id = resolveItemId();
    if (store && id.isValid()) {
      outAction = std::make_unique<FillAction>(id, store, oldPen, newPen);
    }
    return true;
  }

  if (auto *shape = dynamic_cast<QAbstractGraphicsShapeItem *>(item)) {
    const QBrush oldBrush = shape->brush();
    if (oldBrush == brush) {
      return false;
    }
    shape->setBrush(brush);
    ItemId id = resolveItemId();
    if (store && id.isValid()) {
      outAction = std::make_unique<FillAction>(id, store, oldBrush, brush);
    }
    return true;
  }

  if (auto *text = dynamic_cast<QGraphicsTextItem *>(item)) {
    const QColor oldColor = text->defaultTextColor();
    if (oldColor == color) {
      return false;
    }
    text->setDefaultTextColor(color);
    ItemId id = resolveItemId();
    if (store && id.isValid()) {
      outAction = std::make_unique<FillAction>(id, store, oldColor, color);
    }
    return true;
  }

  if (auto *latex = dynamic_cast<LatexTextItem *>(item)) {
    const QColor oldColor = latex->textColor();
    if (oldColor == color) {
      return false;
    }
    latex->setTextColor(color);
    ItemId id = resolveItemId();
    if (store && id.isValid()) {
      outAction = std::make_unique<FillAction>(id, store, oldColor, color);
    }
    return true;
  }

  if (auto *mermaid = dynamic_cast<MermaidTextItem *>(item)) {
    const QString oldTheme = mermaid->theme();
    const QString newTheme = mermaidThemeForColor(color);
    if (oldTheme == newTheme) {
      return false;
    }
    mermaid->setTheme(newTheme);
    ItemId id = resolveItemId();
    if (store && id.isValid()) {
      outAction = std::make_unique<FillAction>(id, store, oldTheme, newTheme);
    }
    return true;
  }

  if (auto *pixmap = dynamic_cast<QGraphicsPixmapItem *>(item)) {
    // Preserve non-colorize effects; fill should not destroy unrelated effects.
    if (pixmap->graphicsEffect() &&
        !dynamic_cast<QGraphicsColorizeEffect *>(pixmap->graphicsEffect())) {
      return false;
    }

    const FillAction::PixmapTintState oldState = currentTintState(pixmap);
    FillAction::PixmapTintState newState;
    newState.enabled = true;
    newState.color = color;
    newState.strength = 0.85;

    if (sameTintState(oldState, newState)) {
      return false;
    }

    applyTintState(pixmap, newState);
    ItemId id = resolveItemId();
    if (store && id.isValid()) {
      outAction = std::make_unique<FillAction>(id, store, oldState, newState);
    }
    return true;
  }

  return false;
}
} // namespace

bool fillTopItemAtPoint(
    QGraphicsScene *scene, const QPointF &point, const QBrush &brush,
    ItemStore *store, QGraphicsItem *backgroundItem,
    QGraphicsItem *extraSkipItem,
    const std::function<void(std::unique_ptr<Action>)> &pushAction) {
  if (!scene) {
    return false;
  }

  const QList<QGraphicsItem *> itemsAtPoint = scene->items(point);
  QSet<QGraphicsItem *> visitedTargets;

  for (QGraphicsItem *item : itemsAtPoint) {
    if (!item || item == backgroundItem || item == extraSkipItem) {
      continue;
    }

    QGraphicsItem *target = resolveFillTarget(item);
    if (!target || target == backgroundItem || target == extraSkipItem) {
      continue;
    }
    if (visitedTargets.contains(target)) {
      continue;
    }
    visitedTargets.insert(target);

    std::unique_ptr<Action> action;
    if (applyFillToItem(target, store, brush, action)) {
      if (action && pushAction) {
        pushAction(std::move(action));
      }
      return true;
    }
  }

  return false;
}
