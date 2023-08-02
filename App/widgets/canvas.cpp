#include "canvas.h"
#include <QGraphicsPathItem>
#include <QPen>

Canvas::Canvas(QWidget *parent) : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    this->setScene(scene);

    this->setRenderHint(QPainter::Antialiasing);
    scene->setSceneRect(0, 0, 800, 600);  // Change this to desired canvas size
}

Canvas::~Canvas()
{
    delete scene;
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        currentPath = new QGraphicsPathItem();
        currentPath->setPen(QPen(Qt::black, 3));  // Change color and size as desired

        QPainterPath p;
        p.moveTo(mapToScene(event->pos()));
        currentPath->setPath(p);

        scene->addItem(currentPath);
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPainterPath p = currentPath->path();
        p.lineTo(mapToScene(event->pos()));
        currentPath->setPath(p);
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // If you want to do something on mouse release, add it here
    }
}
