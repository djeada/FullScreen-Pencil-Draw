/**
 * @file mermaid_tool.cpp
 * @brief Mermaid diagram tool implementation with inline editing.
 */
#include "mermaid_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"
#include "../widgets/mermaid_text_item.h"
#include <QPointer>

MermaidTool::MermaidTool(SceneRenderer *renderer)
    : Tool(renderer), currentEditingItem_(nullptr), currentEditingItemId_() {}

MermaidTool::~MermaidTool() = default;

void MermaidTool::mousePressEvent(QMouseEvent *event, const QPointF &scenePos) {
  if (event->button() == Qt::LeftButton) {
    // Check if we clicked on an existing MermaidTextItem
    QGraphicsItem *item = renderer_->scene()->itemAt(scenePos, QTransform());
    if (auto *mermaidItem = dynamic_cast<MermaidTextItem *>(item)) {
      // If item is not editing, start editing it
      if (!mermaidItem->isEditing()) {
        mermaidItem->startEditing();
        currentEditingItem_ = mermaidItem;
        // Only register if we have a store and item doesn't have an ID yet
        ItemStore *store = renderer_->itemStore();
        if (store) {
          ItemId existingId = store->idForItem(mermaidItem);
          currentEditingItemId_ = existingId.isValid()
                                      ? existingId
                                      : renderer_->registerItem(mermaidItem);
        }
      }
      return;
    }

    // If there's a currently editing item, finish editing first
    if (currentEditingItem_ && currentEditingItem_->isEditing()) {
      // The editing will be finished by the focus out event
      currentEditingItem_ = nullptr;
      currentEditingItemId_ = ItemId();
    }

    // Create a new MermaidTextItem with inline editing
    createMermaidItem(scenePos);
  }
}

void MermaidTool::mouseMoveEvent(QMouseEvent * /*event*/,
                                 const QPointF & /*scenePos*/) {
  // Nothing to do on move
}

void MermaidTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                    const QPointF & /*scenePos*/) {
  // Nothing to do on release
}

void MermaidTool::deactivate() {
  // Finish editing if in progress
  if (currentEditingItem_ && currentEditingItem_->isEditing()) {
    currentEditingItem_->finishEditing();
  }
  currentEditingItem_ = nullptr;
  currentEditingItemId_ = ItemId();
  Tool::deactivate();
}

void MermaidTool::createMermaidItem(const QPointF &position) {
  SceneController *controller = renderer_->sceneController();

  auto *mermaidItem = new MermaidTextItem();
  mermaidItem->setPos(position);

  // Add to scene via SceneController if available
  ItemId mermaidItemId;
  if (controller) {
    mermaidItemId = controller->addItem(mermaidItem);
  } else {
    renderer_->scene()->addItem(mermaidItem);
    mermaidItemId = renderer_->registerItem(mermaidItem);
  }

  // Connect to handle when editing is finished
  QObject::connect(mermaidItem, &MermaidTextItem::editingFinished,
                   [this, mermaidItem = QPointer<MermaidTextItem>(mermaidItem),
                    mermaidItemId, controller]() {
                     // Check if mermaidItem is still valid (not deleted)
                     if (!mermaidItem) {
                       return;
                     }
                     // If the code is empty after editing, remove the item
                     if (mermaidItem->mermaidCode().trimmed().isEmpty()) {
                       if (controller && mermaidItemId.isValid()) {
                         controller->removeItem(mermaidItemId,
                                                false); // Don't keep for undo
                       } else {
                         renderer_->onItemRemoved(mermaidItem);
                         renderer_->scene()->removeItem(mermaidItem);
                         mermaidItem->deleteLater();
                       }
                     } else {
                       // Add to undo stack only when there's actual content
                       renderer_->addDrawAction(mermaidItem);
                     }
                     if (currentEditingItem_ == mermaidItem) {
                       currentEditingItem_ = nullptr;
                       currentEditingItemId_ = ItemId();
                     }
                   });

  // Start inline editing immediately
  mermaidItem->startEditing();
  currentEditingItem_ = mermaidItem;
  currentEditingItemId_ = mermaidItemId;
}
