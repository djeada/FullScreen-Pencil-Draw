/**
 * @file text_on_path_tool.cpp
 * @brief Text-on-path tool implementation.
 */
#include "text_on_path_tool.h"
#include "../core/scene_controller.h"
#include "../core/scene_renderer.h"
#include "../widgets/text_on_path_item.h"
#include <QInputDialog>

TextOnPathTool::TextOnPathTool(SceneRenderer *renderer)
    : Tool(renderer), previewPath_(nullptr), previewSegment_(nullptr),
      isDragging_(false) {}

TextOnPathTool::~TextOnPathTool() { clearPreviewItems(); }

void TextOnPathTool::mousePressEvent(QMouseEvent *event,
                                     const QPointF &scenePos) {
  if (!(event->buttons() & Qt::LeftButton))
    return;

  if (event->type() == QEvent::MouseButtonDblClick) {
    finalizePath();
    return;
  }

  isDragging_ = true;
  dragStart_ = scenePos;

  AnchorPoint anchor;
  anchor.position = scenePos;
  anchor.handleOut = scenePos;
  anchor.hasHandle = false;
  anchors_.append(anchor);

  // Create a preview path on the first anchor
  if (anchors_.size() == 1) {
    previewPath_ = new QGraphicsPathItem();
    QPen pen = renderer_->currentPen();
    pen.setStyle(Qt::DashLine);
    previewPath_->setPen(pen);
    previewPath_->setZValue(998);
    renderer_->scene()->addItem(previewPath_);
  }

  auto *marker =
      new QGraphicsEllipseItem(scenePos.x() - ANCHOR_MARKER_SIZE / 2,
                               scenePos.y() - ANCHOR_MARKER_SIZE / 2,
                               ANCHOR_MARKER_SIZE, ANCHOR_MARKER_SIZE);
  QPen markerPen(renderer_->currentPen().color());
  markerPen.setWidth(1);
  marker->setPen(markerPen);
  marker->setBrush(Qt::white);
  marker->setZValue(1000);
  renderer_->scene()->addItem(marker);
  anchorMarkers_.append(marker);

  rebuildPath();
}

void TextOnPathTool::mouseMoveEvent(QMouseEvent * /*event*/,
                                    const QPointF &scenePos) {
  if (isDragging_ && !anchors_.isEmpty()) {
    AnchorPoint &current = anchors_.last();
    current.handleOut = scenePos;
    current.hasHandle = true;
    rebuildPath();
  } else if (!anchors_.isEmpty() && previewPath_) {
    updatePreview(scenePos);
  }
}

void TextOnPathTool::mouseReleaseEvent(QMouseEvent * /*event*/,
                                       const QPointF &scenePos) {
  if (isDragging_ && !anchors_.isEmpty()) {
    AnchorPoint &current = anchors_.last();
    if (current.position != scenePos) {
      current.handleOut = scenePos;
      current.hasHandle = true;
    }
    isDragging_ = false;
    rebuildPath();
  }
}

void TextOnPathTool::deactivate() {
  finalizePath();
  Tool::deactivate();
}

void TextOnPathTool::finalizePath() {
  clearPreviewItems();

  if (anchors_.size() < 2) {
    // Not enough points — discard
    if (previewPath_) {
      if (previewPath_->scene())
        renderer_->scene()->removeItem(previewPath_);
      delete previewPath_;
    }
    previewPath_ = nullptr;
    anchors_.clear();
    isDragging_ = false;
    return;
  }

  // Build the final path
  QPainterPath path;
  path.moveTo(anchors_.first().position);
  for (int i = 1; i < anchors_.size(); ++i) {
    const AnchorPoint &prev = anchors_[i - 1];
    const AnchorPoint &curr = anchors_[i];

    QPointF cp1 = prev.hasHandle ? prev.handleOut : prev.position;
    QPointF cp2 =
        curr.hasHandle ? 2.0 * curr.position - curr.handleOut : curr.position;

    path.cubicTo(cp1, cp2, curr.position);
  }

  // Remove preview path from scene
  if (previewPath_) {
    if (previewPath_->scene())
      renderer_->scene()->removeItem(previewPath_);
    delete previewPath_;
    previewPath_ = nullptr;
  }

  // Ask user for text
  bool ok = false;
  QString text =
      QInputDialog::getText(nullptr, "Text on Path",
                            "Enter text:", QLineEdit::Normal, QString(), &ok);
  if (!ok || text.trimmed().isEmpty()) {
    anchors_.clear();
    isDragging_ = false;
    return;
  }

  // Create the final TextOnPathItem
  auto *item = new TextOnPathItem();
  item->setFont(QFont("Arial", qMax(12, renderer_->currentPen().width() * 3)));
  item->setTextColor(renderer_->currentPen().color());
  item->setPath(path);
  item->setText(text);

  SceneController *controller = renderer_->sceneController();
  if (controller) {
    controller->addItem(item);
  } else {
    renderer_->scene()->addItem(item);
    renderer_->registerItem(item);
  }
  renderer_->addDrawAction(item);

  anchors_.clear();
  isDragging_ = false;
}

void TextOnPathTool::updatePreview(const QPointF &mousePos) {
  if (anchors_.isEmpty())
    return;

  const AnchorPoint &last = anchors_.last();
  QPainterPath preview;
  preview.moveTo(last.position);

  if (last.hasHandle) {
    QPointF mirroredHandle = 2.0 * last.position - last.handleOut;
    preview.cubicTo(mirroredHandle, mousePos, mousePos);
  } else {
    preview.lineTo(mousePos);
  }

  if (!previewSegment_) {
    previewSegment_ = new QGraphicsPathItem();
    QPen pen = renderer_->currentPen();
    pen.setStyle(Qt::DashLine);
    previewSegment_->setPen(pen);
    previewSegment_->setZValue(999);
    renderer_->scene()->addItem(previewSegment_);
  }
  previewSegment_->setPath(preview);
}

void TextOnPathTool::rebuildPath() {
  if (!previewPath_ || anchors_.isEmpty())
    return;

  QPainterPath path;
  path.moveTo(anchors_.first().position);

  for (int i = 1; i < anchors_.size(); ++i) {
    const AnchorPoint &prev = anchors_[i - 1];
    const AnchorPoint &curr = anchors_[i];

    QPointF cp1 = prev.hasHandle ? prev.handleOut : prev.position;
    QPointF cp2 =
        curr.hasHandle ? 2.0 * curr.position - curr.handleOut : curr.position;

    path.cubicTo(cp1, cp2, curr.position);
  }

  previewPath_->setPath(path);
}

void TextOnPathTool::clearPreviewItems() {
  if (previewSegment_) {
    if (previewSegment_->scene())
      renderer_->scene()->removeItem(previewSegment_);
    delete previewSegment_;
    previewSegment_ = nullptr;
  }

  for (auto *marker : anchorMarkers_) {
    if (marker->scene())
      renderer_->scene()->removeItem(marker);
    delete marker;
  }
  anchorMarkers_.clear();
}
