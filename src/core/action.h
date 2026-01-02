#ifndef ACTION_H
#define ACTION_H

#include <QGraphicsItem>
#include <QGraphicsScene>

class Action {
public:
  Action() = default;
  virtual ~Action();
  virtual void undo() = 0;
  virtual void redo() = 0;
};

class DrawAction : public Action {
public:
  DrawAction(QGraphicsItem *item, QGraphicsScene *scene);
  ~DrawAction();
  void undo() override;
  void redo() override;

private:
  QGraphicsItem *item;
  QGraphicsScene *scene;
};

class DeleteAction : public Action {
public:
  DeleteAction(QGraphicsItem *item, QGraphicsScene *scene);
  ~DeleteAction();
  void undo() override;
  void redo() override;

private:
  QGraphicsItem *item;
  QGraphicsScene *scene;
};

#endif // ACTION_H
