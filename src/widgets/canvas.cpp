// canvas.cpp
#include "canvas.h"
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QFileDialog>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QInputDialog>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <cmath>

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent), scene(new QGraphicsScene(this)),
      tempShapeItem(nullptr), currentShape(Pen), currentPen(Qt::white, 3),
      eraserPen(Qt::black, 10), currentPath(nullptr),
      backgroundColor(Qt::black), eraserPreview(nullptr),
      backgroundImage(nullptr), isPanning(false) {

  this->setScene(scene);
  this->setRenderHint(QPainter::Antialiasing);
  this->setRenderHint(QPainter::SmoothPixmapTransform);
  this->setRenderHint(QPainter::TextAntialiasing);
  this->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
  this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  this->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
  this->setCacheMode(QGraphicsView::CacheBackground);
  scene->setSceneRect(0, 0, 3000, 2000);

  scene->setBackgroundBrush(backgroundColor);
  eraserPen.setColor(backgroundColor);

  eraserPreview = scene->addEllipse(0, 0, eraserPen.width(), eraserPen.width(),
                                    QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview->setZValue(1000);
  eraserPreview->hide();

  this->setMouseTracking(true);
  scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);

  currentPen.setCapStyle(Qt::RoundCap);
  currentPen.setJoinStyle(Qt::RoundJoin);
}

Canvas::~Canvas() {}

int Canvas::getCurrentBrushSize() const { return currentPen.width(); }
QColor Canvas::getCurrentColor() const { return currentPen.color(); }
double Canvas::getCurrentZoom() const { return currentZoom * 100.0; }
int Canvas::getCurrentOpacity() const { return currentOpacity; }
bool Canvas::isGridVisible() const { return showGrid; }

void Canvas::drawBackground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawBackground(painter, rect);
  if (showGrid) {
    painter->setPen(QPen(QColor(60, 60, 60), 0.5));
    qreal left = int(rect.left()) - (int(rect.left()) % GRID_SIZE);
    qreal top = int(rect.top()) - (int(rect.top()) % GRID_SIZE);
    QVector<QLineF> lines;
    for (qreal x = left; x < rect.right(); x += GRID_SIZE)
      lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    for (qreal y = top; y < rect.bottom(); y += GRID_SIZE)
      lines.append(QLineF(rect.left(), y, rect.right(), y));
    painter->drawLines(lines);
  }
}

void Canvas::setShape(const QString &shapeType) {
  if (shapeType == "Line") { currentShape = Line; setCursor(Qt::CrossCursor); }
  else if (shapeType == "Rectangle") { currentShape = Rectangle; setCursor(Qt::CrossCursor); }
  else if (shapeType == "Circle") { currentShape = Circle; setCursor(Qt::CrossCursor); }
  else if (shapeType == "Selection") { currentShape = Selection; setCursor(Qt::ArrowCursor); this->setDragMode(QGraphicsView::RubberBandDrag); }
  tempShapeItem = nullptr;
  if (currentShape != Eraser) {
    hideEraserPreview();
    for (auto item : scene->items()) {
      if (item != eraserPreview && item != backgroundImage) {
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
      }
    }
  }
  if (currentShape != Selection) { scene->clearSelection(); this->setDragMode(QGraphicsView::NoDrag); }
  isPanning = false;
}

void Canvas::setPenTool() {
  currentShape = Pen; tempShapeItem = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene->clearSelection();
  setCursor(Qt::CrossCursor); isPanning = false;
}

void Canvas::setEraserTool() {
  currentShape = Eraser; tempShapeItem = nullptr;
  eraserPen.setColor(backgroundColor);
  this->setDragMode(QGraphicsView::NoDrag);
  scene->clearSelection();
  for (auto item : scene->items()) {
    if (item != eraserPreview && item != backgroundImage) {
      item->setFlag(QGraphicsItem::ItemIsSelectable, false);
      item->setFlag(QGraphicsItem::ItemIsMovable, false);
    }
  }
  setCursor(Qt::BlankCursor); isPanning = false;
}

void Canvas::setTextTool() {
  currentShape = Text; tempShapeItem = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene->clearSelection();
  setCursor(Qt::IBeamCursor); isPanning = false;
}

void Canvas::setFillTool() {
  currentShape = Fill; tempShapeItem = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene->clearSelection();
  setCursor(Qt::PointingHandCursor); isPanning = false;
}

void Canvas::setArrowTool() {
  currentShape = Arrow; tempShapeItem = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene->clearSelection();
  setCursor(Qt::CrossCursor); isPanning = false;
}

void Canvas::setPanTool() {
  currentShape = Pan; tempShapeItem = nullptr;
  this->setDragMode(QGraphicsView::NoDrag);
  hideEraserPreview(); scene->clearSelection();
  setCursor(Qt::OpenHandCursor); isPanning = false;
}

void Canvas::setPenColor(const QColor &color) {
  QColor newColor = color;
  newColor.setAlpha(currentOpacity);
  currentPen.setColor(newColor);
  emit colorChanged(color);
}

void Canvas::setOpacity(int opacity) {
  currentOpacity = qBound(0, opacity, 255);
  QColor color = currentPen.color();
  color.setAlpha(currentOpacity);
  currentPen.setColor(color);
  emit opacityChanged(currentOpacity);
}

void Canvas::increaseBrushSize() {
  if (currentPen.width() < MAX_BRUSH_SIZE) {
    int newSize = currentPen.width() + BRUSH_SIZE_STEP;
    currentPen.setWidth(newSize);
    eraserPen.setWidth(eraserPen.width() + BRUSH_SIZE_STEP);
    if (currentShape == Eraser && eraserPreview)
      eraserPreview->setRect(eraserPreview->rect().x(), eraserPreview->rect().y(), eraserPen.width(), eraserPen.width());
    emit brushSizeChanged(newSize);
  }
}

void Canvas::decreaseBrushSize() {
  if (currentPen.width() > MIN_BRUSH_SIZE) {
    int newSize = qMax(currentPen.width() - BRUSH_SIZE_STEP, MIN_BRUSH_SIZE);
    currentPen.setWidth(newSize);
    eraserPen.setWidth(qMax(eraserPen.width() - BRUSH_SIZE_STEP, MIN_BRUSH_SIZE));
    if (currentShape == Eraser && eraserPreview)
      eraserPreview->setRect(eraserPreview->rect().x(), eraserPreview->rect().y(), eraserPen.width(), eraserPen.width());
    emit brushSizeChanged(newSize);
  }
}

void Canvas::clearCanvas() {
  scene->clear();
  undoStack.clear();
  redoStack.clear();
  backgroundImage = nullptr;
  scene->setBackgroundBrush(backgroundColor);
  eraserPreview = scene->addEllipse(0, 0, eraserPen.width(), eraserPen.width(), QPen(Qt::gray), QBrush(Qt::NoBrush));
  eraserPreview->setZValue(1000);
  eraserPreview->hide();
}

void Canvas::newCanvas(int width, int height, const QColor &bgColor) {
  clearCanvas();
  backgroundColor = bgColor;
  eraserPen.setColor(backgroundColor);
  scene->setSceneRect(0, 0, width, height);
  scene->setBackgroundBrush(backgroundColor);
  resetTransform();
  currentZoom = 1.0;
  emit zoomChanged(100.0);
}

void Canvas::undoLastAction() {
  if (!undoStack.isEmpty()) {
    Action *lastAction = undoStack.takeLast();
    lastAction->undo();
    redoStack.append(lastAction);
  }
}

void Canvas::redoLastAction() {
  if (!redoStack.isEmpty()) {
    Action *nextAction = redoStack.takeLast();
    nextAction->redo();
    undoStack.append(nextAction);
  }
}

void Canvas::zoomIn() { applyZoom(ZOOM_FACTOR); }
void Canvas::zoomOut() { applyZoom(1.0 / ZOOM_FACTOR); }
void Canvas::zoomReset() { resetTransform(); currentZoom = 1.0; emit zoomChanged(100.0); }

void Canvas::applyZoom(double factor) {
  double newZoom = currentZoom * factor;
  if (newZoom > MAX_ZOOM || newZoom < MIN_ZOOM) return;
  currentZoom = newZoom;
  scale(factor, factor);
  emit zoomChanged(currentZoom * 100.0);
}

void Canvas::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    event->angleDelta().y() > 0 ? zoomIn() : zoomOut();
    event->accept();
  } else QGraphicsView::wheelEvent(event);
}

void Canvas::toggleGrid() {
  showGrid = !showGrid;
  viewport()->update();
  scene->invalidate(scene->sceneRect(), QGraphicsScene::BackgroundLayer);
}

void Canvas::selectAll() {
  for (auto item : scene->items())
    if (item != eraserPreview && item != backgroundImage) item->setSelected(true);
}

void Canvas::deleteSelectedItems() {
  for (QGraphicsItem *item : scene->selectedItems()) {
    if (item != eraserPreview && item != backgroundImage) {
      undoStack.append(new DeleteAction(item));
      redoStack.clear();
      scene->removeItem(item);
    }
  }
}

void Canvas::duplicateSelectedItems() {
  QList<QGraphicsItem *> newItems;
  for (QGraphicsItem *item : scene->selectedItems()) {
    if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) {
      auto n = new QGraphicsRectItem(r->rect()); n->setPen(r->pen()); n->setBrush(r->brush());
      n->setPos(r->pos() + QPointF(20, 20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene->addItem(n); newItems.append(n); undoStack.append(new DrawAction(n));
    } else if (auto e = dynamic_cast<QGraphicsEllipseItem *>(item)) {
      if (item == eraserPreview) continue;
      auto n = new QGraphicsEllipseItem(e->rect()); n->setPen(e->pen()); n->setBrush(e->brush());
      n->setPos(e->pos() + QPointF(20, 20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene->addItem(n); newItems.append(n); undoStack.append(new DrawAction(n));
    } else if (auto l = dynamic_cast<QGraphicsLineItem *>(item)) {
      auto n = new QGraphicsLineItem(l->line()); n->setPen(l->pen());
      n->setPos(l->pos() + QPointF(20, 20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene->addItem(n); newItems.append(n); undoStack.append(new DrawAction(n));
    } else if (auto p = dynamic_cast<QGraphicsPathItem *>(item)) {
      auto n = new QGraphicsPathItem(p->path()); n->setPen(p->pen());
      n->setPos(p->pos() + QPointF(20, 20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene->addItem(n); newItems.append(n); undoStack.append(new DrawAction(n));
    } else if (auto t = dynamic_cast<QGraphicsTextItem *>(item)) {
      auto n = new QGraphicsTextItem(t->toPlainText()); n->setFont(t->font()); n->setDefaultTextColor(t->defaultTextColor());
      n->setPos(t->pos() + QPointF(20, 20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
      scene->addItem(n); newItems.append(n); undoStack.append(new DrawAction(n));
    }
  }
  redoStack.clear();
  scene->clearSelection();
  for (auto i : newItems) i->setSelected(true);
}

void Canvas::saveToFile() {
  QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");
  if (fileName.isEmpty()) return;
  bool ev = eraserPreview && eraserPreview->isVisible();
  if (eraserPreview) eraserPreview->hide();
  scene->clearSelection();
  QRectF sr = scene->itemsBoundingRect();
  if (sr.isEmpty()) sr = scene->sceneRect();
  sr.adjust(-10, -10, 10, 10);
  QImage img(sr.size().toSize(), QImage::Format_ARGB32);
  img.fill(backgroundColor);
  QPainter p(&img); p.setRenderHint(QPainter::Antialiasing); p.setRenderHint(QPainter::TextAntialiasing);
  scene->render(&p, QRectF(), sr); p.end();
  img.save(fileName);
  if (ev && eraserPreview) eraserPreview->show();
}

void Canvas::openFile() {
  QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif);;All (*)");
  if (fileName.isEmpty()) return;
  QPixmap pm(fileName);
  if (pm.isNull()) return;
  if (backgroundImage) { scene->removeItem(backgroundImage); delete backgroundImage; }
  backgroundImage = scene->addPixmap(pm);
  backgroundImage->setZValue(-1000);
  backgroundImage->setFlag(QGraphicsItem::ItemIsSelectable, false);
  backgroundImage->setFlag(QGraphicsItem::ItemIsMovable, false);
  scene->setSceneRect(0, 0, qMax(scene->sceneRect().width(), (qreal)pm.width()), qMax(scene->sceneRect().height(), (qreal)pm.height()));
}

void Canvas::createTextItem(const QPointF &pos) {
  bool ok;
  QString text = QInputDialog::getText(this, "Add Text", "Enter text:", QLineEdit::Normal, "", &ok);
  if (ok && !text.isEmpty()) {
    auto ti = new QGraphicsTextItem(text);
    ti->setFont(QFont("Arial", currentPen.width() * 4));
    ti->setDefaultTextColor(currentPen.color());
    ti->setPos(pos);
    ti->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    scene->addItem(ti);
    undoStack.append(new DrawAction(ti));
    redoStack.clear();
  }
}

void Canvas::fillAt(const QPointF &point) {
  for (QGraphicsItem *item : scene->items(point)) {
    if (item == eraserPreview || item == backgroundImage) continue;
    if (auto r = dynamic_cast<QGraphicsRectItem *>(item)) { r->setBrush(currentPen.color()); return; }
    if (auto e = dynamic_cast<QGraphicsEllipseItem *>(item)) { e->setBrush(currentPen.color()); return; }
    if (auto p = dynamic_cast<QGraphicsPolygonItem *>(item)) { p->setBrush(currentPen.color()); return; }
  }
}

void Canvas::drawArrow(const QPointF &start, const QPointF &end) {
  auto li = new QGraphicsLineItem(QLineF(start, end));
  li->setPen(currentPen);
  li->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
  scene->addItem(li);
  double angle = std::atan2(-(end.y() - start.y()), end.x() - start.x());
  double sz = currentPen.width() * 4;
  QPolygonF ah; ah << end << end + QPointF(std::sin(angle + M_PI/3)*sz, std::cos(angle + M_PI/3)*sz)
                    << end + QPointF(std::sin(angle + M_PI - M_PI/3)*sz, std::cos(angle + M_PI - M_PI/3)*sz);
  auto ahi = new QGraphicsPolygonItem(ah);
  ahi->setPen(currentPen); ahi->setBrush(currentPen.color());
  ahi->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
  scene->addItem(ahi);
  undoStack.append(new DrawAction(li)); undoStack.append(new DrawAction(ahi));
  redoStack.clear();
}

void Canvas::mousePressEvent(QMouseEvent *event) {
  QPointF sp = mapToScene(event->pos());
  emit cursorPositionChanged(sp);
  if (currentShape == Selection) { QGraphicsView::mousePressEvent(event); return; }
  if (currentShape == Pan) { isPanning = true; lastPanPoint = event->pos(); setCursor(Qt::ClosedHandCursor); return; }
  startPoint = sp;
  switch (currentShape) {
  case Text: createTextItem(sp); break;
  case Fill: fillAt(sp); break;
  case Eraser: eraseAt(sp); break;
  case Pen: {
    currentPath = new QGraphicsPathItem();
    currentPath->setPen(currentPen);
    currentPath->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    QPainterPath p; p.moveTo(sp); currentPath->setPath(p);
    scene->addItem(currentPath);
    pointBuffer.clear(); pointBuffer.append(sp);
    undoStack.append(new DrawAction(currentPath)); redoStack.clear();
  } break;
  case Arrow: case Rectangle: {
    auto ri = new QGraphicsRectItem(QRectF(startPoint, startPoint));
    ri->setPen(currentPen); ri->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    scene->addItem(ri); tempShapeItem = ri;
    if (currentShape == Rectangle) { undoStack.append(new DrawAction(ri)); redoStack.clear(); }
  } break;
  case Circle: {
    auto ei = new QGraphicsEllipseItem(QRectF(startPoint, startPoint));
    ei->setPen(currentPen); ei->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    scene->addItem(ei); tempShapeItem = ei;
    undoStack.append(new DrawAction(ei)); redoStack.clear();
  } break;
  case Line: {
    auto li = new QGraphicsLineItem(QLineF(startPoint, startPoint));
    li->setPen(currentPen); li->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    scene->addItem(li); tempShapeItem = li;
    undoStack.append(new DrawAction(li)); redoStack.clear();
  } break;
  default: QGraphicsView::mousePressEvent(event); break;
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
  QPointF cp = mapToScene(event->pos());
  emit cursorPositionChanged(cp);
  if (currentShape == Selection) { QGraphicsView::mouseMoveEvent(event); return; }
  if (currentShape == Pan && isPanning) {
    QPointF d = event->pos() - lastPanPoint; lastPanPoint = event->pos();
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
    return;
  }
  switch (currentShape) {
  case Pen: if (event->buttons() & Qt::LeftButton) addPoint(cp); break;
  case Eraser: if (event->buttons() & Qt::LeftButton) eraseAt(cp); updateEraserPreview(cp); break;
  case Arrow: case Rectangle: if (tempShapeItem) static_cast<QGraphicsRectItem*>(tempShapeItem)->setRect(QRectF(startPoint, cp).normalized()); break;
  case Circle: if (tempShapeItem) static_cast<QGraphicsEllipseItem*>(tempShapeItem)->setRect(QRectF(startPoint, cp).normalized()); break;
  case Line: if (tempShapeItem) static_cast<QGraphicsLineItem*>(tempShapeItem)->setLine(QLineF(startPoint, cp)); break;
  default: QGraphicsView::mouseMoveEvent(event); break;
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
  QPointF ep = mapToScene(event->pos());
  if (currentShape == Selection) { QGraphicsView::mouseReleaseEvent(event); return; }
  if (currentShape == Pan) { isPanning = false; setCursor(Qt::OpenHandCursor); return; }
  if (currentShape == Arrow && tempShapeItem) {
    scene->removeItem(tempShapeItem); delete tempShapeItem; tempShapeItem = nullptr;
    drawArrow(startPoint, ep); return;
  }
  if (currentShape != Pen && currentShape != Eraser && tempShapeItem) tempShapeItem = nullptr;
  else if (currentShape == Pen) { currentPath = nullptr; pointBuffer.clear(); }
}

void Canvas::updateEraserPreview(const QPointF &pos) {
  if (!eraserPreview) return;
  qreal r = eraserPen.width() / 2.0;
  eraserPreview->setRect(pos.x() - r, pos.y() - r, eraserPen.width(), eraserPen.width());
  if (!eraserPreview->isVisible()) eraserPreview->show();
}

void Canvas::hideEraserPreview() { if (eraserPreview) eraserPreview->hide(); }

void Canvas::copySelectedItems() {
  auto sel = scene->selectedItems();
  if (sel.isEmpty()) return;
  auto md = new QMimeData();
  QByteArray ba; QDataStream ds(&ba, QIODevice::WriteOnly);
  for (auto item : sel) {
    if (auto r = dynamic_cast<QGraphicsRectItem*>(item)) { ds << QString("Rectangle") << r->rect() << r->pos() << r->pen() << r->brush(); }
    else if (auto e = dynamic_cast<QGraphicsEllipseItem*>(item)) { if (item == eraserPreview) continue; ds << QString("Ellipse") << e->rect() << e->pos() << e->pen() << e->brush(); }
    else if (auto l = dynamic_cast<QGraphicsLineItem*>(item)) { ds << QString("Line") << l->line() << l->pos() << l->pen(); }
    else if (auto p = dynamic_cast<QGraphicsPathItem*>(item)) { ds << QString("Path") << p->path() << p->pos() << p->pen(); }
    else if (auto t = dynamic_cast<QGraphicsTextItem*>(item)) { ds << QString("Text") << t->toPlainText() << t->pos() << t->font() << t->defaultTextColor(); }
    else if (auto pg = dynamic_cast<QGraphicsPolygonItem*>(item)) { ds << QString("Polygon") << pg->polygon() << pg->pos() << pg->pen() << pg->brush(); }
  }
  md->setData("application/x-canvas-items", ba);
  QApplication::clipboard()->setMimeData(md);
}

void Canvas::cutSelectedItems() {
  auto sel = scene->selectedItems();
  if (sel.isEmpty()) return;
  copySelectedItems();
  for (auto item : sel) {
    if (item != eraserPreview && item != backgroundImage) {
      undoStack.append(new DeleteAction(item)); redoStack.clear();
      scene->removeItem(item);
    }
  }
}

void Canvas::pasteItems() {
  auto md = QApplication::clipboard()->mimeData();
  if (!md->hasFormat("application/x-canvas-items")) return;
  QByteArray ba = md->data("application/x-canvas-items");
  QDataStream ds(&ba, QIODevice::ReadOnly);
  QList<QGraphicsItem*> pi;
  while (!ds.atEnd()) {
    QString t; ds >> t;
    if (t == "Rectangle") { QRectF r; QPointF p; QPen pn; QBrush b; ds >> r >> p >> pn >> b; auto n = new QGraphicsRectItem(r); n->setPen(pn); n->setBrush(b); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene->addItem(n); pi.append(n); undoStack.append(new DrawAction(n)); }
    else if (t == "Ellipse") { QRectF r; QPointF p; QPen pn; QBrush b; ds >> r >> p >> pn >> b; auto n = new QGraphicsEllipseItem(r); n->setPen(pn); n->setBrush(b); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene->addItem(n); pi.append(n); undoStack.append(new DrawAction(n)); }
    else if (t == "Line") { QLineF l; QPointF p; QPen pn; ds >> l >> p >> pn; auto n = new QGraphicsLineItem(l); n->setPen(pn); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene->addItem(n); pi.append(n); undoStack.append(new DrawAction(n)); }
    else if (t == "Path") { QPainterPath pp; QPointF p; QPen pn; ds >> pp >> p >> pn; auto n = new QGraphicsPathItem(pp); n->setPen(pn); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene->addItem(n); pi.append(n); undoStack.append(new DrawAction(n)); }
    else if (t == "Text") { QString tx; QPointF p; QFont f; QColor c; ds >> tx >> p >> f >> c; auto n = new QGraphicsTextItem(tx); n->setFont(f); n->setDefaultTextColor(c); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene->addItem(n); pi.append(n); undoStack.append(new DrawAction(n)); }
    else if (t == "Polygon") { QPolygonF pg; QPointF p; QPen pn; QBrush b; ds >> pg >> p >> pn >> b; auto n = new QGraphicsPolygonItem(pg); n->setPen(pn); n->setBrush(b); n->setPos(p + QPointF(20,20)); n->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable); scene->addItem(n); pi.append(n); undoStack.append(new DrawAction(n)); }
  }
  redoStack.clear(); scene->clearSelection();
  for (auto i : pi) i->setSelected(true);
}

void Canvas::addPoint(const QPointF &point) {
  if (!currentPath) return;
  pointBuffer.append(point);
  const int mpr = 4;
  if (pointBuffer.size() >= mpr) {
    QPointF p0 = pointBuffer.at(pointBuffer.size() - mpr);
    QPointF p1 = pointBuffer.at(pointBuffer.size() - mpr + 1);
    QPointF p2 = pointBuffer.at(pointBuffer.size() - mpr + 2);
    QPointF p3 = pointBuffer.at(pointBuffer.size() - mpr + 3);
    QPainterPath path = currentPath->path();
    path.cubicTo(p1 + (p2 - p0) / 6.0, p2 - (p3 - p1) / 6.0, p2);
    currentPath->setPath(path);
  }
  if (pointBuffer.size() > mpr) pointBuffer.removeFirst();
}

void Canvas::eraseAt(const QPointF &point) {
  qreal sz = eraserPen.width();
  QRectF er(point.x() - sz/2, point.y() - sz/2, sz, sz);
  QPainterPath ep; ep.addEllipse(er);
  for (QGraphicsItem *item : scene->items(er)) {
    if (item == eraserPreview || item == backgroundImage) continue;
    QPainterPathStroker s; s.setWidth(1);
    if (ep.intersects(s.createStroke(item->shape()))) {
      undoStack.append(new DeleteAction(item)); redoStack.clear();
      scene->removeItem(item);
    }
  }
}
