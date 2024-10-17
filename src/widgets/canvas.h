#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QPen>

class Canvas : public QGraphicsView {
  Q_OBJECT

public:
  explicit Canvas(QWidget *parent = nullptr);
  ~Canvas();

public slots:
  void setPenTool();
  void setEraserTool();
  void setPenColor(const QColor &color);
  void drawRectangle();
  void drawCircle();
  void drawLine();
  void increaseBrushSize();
  void decreaseBrushSize();
  void clearCanvas();
  void undoLastAction();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  QGraphicsScene *scene;
  QGraphicsPathItem *currentPath;
  QList<QGraphicsItem*> itemsStack; // For undo functionality
  QPen currentPen;
};

#endif // CANVAS_H
