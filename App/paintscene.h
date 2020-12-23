#ifndef PAINTSCENE_H
#define PAINTSCENE_H

#include <QGraphicsScene>
enum class PaintMode {NoMode, SelectObject, DrawPoint, DrawLine, DrawRect, DrawCircle};

class PaintScene : public  QGraphicsScene {
    Q_OBJECT
    public:
        PaintScene(QObject* parent);
        void setMode(PaintMode mode);
    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent* event);
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
        void keyPressEvent(QKeyEvent* event);
    private:
        PaintMode sceneMode;
        QPointF lastPoint;
        QGraphicsLineItem* itemToDraw;
        void drawPoint(QPointF point);
        void drawLine(QPointF point);
        void drawRect(QPointF point);
        void drawCircle(QPointF point);
        void setItemsSelectable(bool flag);

};

#endif // PAINTSCENE_H
