#include "paintscene.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLineItem>
#include <QGraphicsView>
#include <QKeyEvent>

PaintScene::PaintScene(QObject* parent) :
    QGraphicsScene(parent),
    sceneMode(PaintMode::NoMode),
    itemToDraw(nullptr)
{

}

void PaintScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (sceneMode != PaintMode::NoMode)
        lastPoint = event->scenePos();

    QGraphicsScene::mousePressEvent(event);
}

void PaintScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {

    if (sceneMode == PaintMode::DrawPoint)
        drawPoint(event->scenePos());

    else if (sceneMode == PaintMode::DrawLine)
        drawLine(event->scenePos());

    else
        QGraphicsScene::mouseMoveEvent(event);
}

void PaintScene::setItemsSelectable(bool areControllable){
    for (auto& item : items()){
        item->setFlag(QGraphicsItem::ItemIsSelectable, areControllable);
        item->setFlag(QGraphicsItem::ItemIsMovable, areControllable);
    }
}

void PaintScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event){
    itemToDraw = 0;
    QGraphicsScene::mouseReleaseEvent(event);
}

void PaintScene::keyPressEvent(QKeyEvent* event){
    if (event->key() == Qt::Key_Delete)
        for (auto& item : selectedItems()) {
            removeItem(item);
            delete item;
        }
    else
        QGraphicsScene::keyPressEvent(event);
}

void PaintScene::drawPoint(QPointF point) {
    auto pen = QPen(QColor(0,0,0), 10, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    addLine(lastPoint.x(), lastPoint.y(), point.x(), point.y(),  pen);
    lastPoint = point;
}

void PaintScene::drawLine(QPointF point)
{
    if (!itemToDraw) {
        itemToDraw = new QGraphicsLineItem();
        addItem(itemToDraw);
        itemToDraw->setPen(QPen(Qt::black, 3, Qt::SolidLine));
        itemToDraw->setPos(lastPoint);
    }

    itemToDraw->setLine(0,0, point.x() - lastPoint.x(), point.y() - lastPoint.y());
}

void PaintScene::setMode(PaintMode mode) {
    sceneMode = mode;
    auto vMode = QGraphicsView::NoDrag;

    if (mode == PaintMode::DrawLine || mode == PaintMode::DrawPoint) {
        setItemsSelectable(false);
        vMode = QGraphicsView::NoDrag;
    }

    else if (mode == PaintMode::SelectObject) {
        setItemsSelectable(true);
        vMode = QGraphicsView::RubberBandDrag;
    }

    auto* mView = views().at(0);

    if (mView)
        mView->setDragMode(vMode);
}
