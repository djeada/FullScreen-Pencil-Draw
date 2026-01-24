/**
 * @file pen_tool.h
 * @brief Freehand drawing tool with smooth spline interpolation.
 */
#ifndef PEN_TOOL_H
#define PEN_TOOL_H

#include "tool.h"
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QVector>

/**
 * @brief Freehand drawing tool with smooth Catmull-Rom spline interpolation.
 *
 * The pen tool allows users to draw smooth freehand curves. It uses
 * Catmull-Rom spline interpolation to create smooth curves from the
 * user's mouse movements.
 */
class PenTool : public Tool {
public:
  explicit PenTool(SceneRenderer *renderer);
  ~PenTool() override;

  QString name() const override { return "Pen"; }
  QCursor cursor() const override { return Qt::CrossCursor; }

  void mousePressEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseMoveEvent(QMouseEvent *event, const QPointF &scenePos) override;
  void mouseReleaseEvent(QMouseEvent *event, const QPointF &scenePos) override;

private:
  void addPoint(const QPointF &point);

  QGraphicsPathItem *currentPath_;
  QVector<QPointF> pointBuffer_;
  static constexpr int MIN_POINTS_FOR_SPLINE = 4;
};

#endif // PEN_TOOL_H
