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
  DrawAction(QGraphicsItem *item);
  ~DrawAction();
  void undo() override;
  void redo() override;

private:
  QGraphicsItem *item;
};

class DeleteAction : public Action {
public:
  DeleteAction(QGraphicsItem *item);
  ~DeleteAction();
  void undo() override;
  void redo() override;

private:
  QGraphicsItem *item;
};

#endif // ACTION_H
