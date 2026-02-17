/**
 * @file fill_tool.cpp
 * @brief Fill tool implementation.
 */
#include "fill_tool.h"
#include "../core/action.h"
#include "../core/fill_utils.h"
#include "../core/item_store.h"
#include "../core/scene_renderer.h"

FillTool::FillTool(SceneRenderer *renderer) : Tool(renderer) {}

FillTool::~FillTool() = default;

void FillTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->button() == Qt::LeftButton) {
    fillAt(scenePos);
  }
}

void FillTool::mouseMoveEvent(QMouseEvent * /*event*/,
                              const QPointF & /*scenePos*/) {
  // Nothing to do on move
}

void FillTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                 const QPointF & /*scenePos*/) {
  // Nothing to do on release
}

void FillTool::fillAt(const QPointF &point) {
  QGraphicsScene *scene = renderer_->scene();
  ItemStore *store = renderer_->itemStore();
  fillTopItemAtPoint(
      scene, point, renderer_->currentPen().color(), store,
      renderer_->backgroundImageItem(), nullptr,
      [this](std::unique_ptr<Action> action) {
        if (action) {
          renderer_->addAction(std::move(action));
        }
      });
}
