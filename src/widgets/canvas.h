// canvas.h
#ifndef CANVAS_H
#define CANVAS_H

#include <QApplication>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QList>
#include <QMimeData>
#include <QMouseEvent>
#include <QPen>
#include <QVector>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>

#include "../core/action.h"

class Canvas : public QGraphicsView {
  Q_OBJECT

public:
  explicit Canvas(QWidget *parent = nullptr);
  ~Canvas();

  int getCurrentBrushSize() const;
  QColor getCurrentColor() const;
  double getCurrentZoom() const;
  int getCurrentOpacity() const;
  bool isGridVisible() const;
  bool isFilledShapes() const;

signals:
  void brushSizeChanged(int size);
  void colorChanged(const QColor &color);
  void zoomChanged(double zoomPercent);
  void opacityChanged(int opacity);
  void cursorPositionChanged(const QPointF &pos);
  void filledShapesChanged(bool filled);

public slots:
  void setShape(const QString &shapeType);
  void setPenTool();
  void setEraserTool();
  void setTextTool();
  void setFillTool();
  void setArrowTool();
  void setPanTool();
  void setPenColor(const QColor &color);
  void setOpacity(int opacity);
  void increaseBrushSize();
  void decreaseBrushSize();
  void clearCanvas();
  void undoLastAction();
  void redoLastAction();
  void copySelectedItems();
  void cutSelectedItems();
  void pasteItems();
  void duplicateSelectedItems();
  void deleteSelectedItems();
  void zoomIn();
  void zoomOut();
  void zoomReset();
  void saveToFile();
  void openFile();
  void newCanvas(int width, int height, const QColor &bgColor);
  void toggleGrid();
  void toggleFilledShapes();
  void selectAll();
  void exportSelectionToSVG();
  void exportSelectionToPNG();
  void exportSelectionToJPG();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void drawBackground(QPainter *painter, const QRectF &rect) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  // Enums
  enum ShapeType { Line, Rectangle, Circle, Pen, Eraser, Selection, Text, Fill, Arrow, Pan };

  // Member variables
  QGraphicsScene *scene;
  QPen currentPen;
  QPen eraserPen;
  ShapeType currentShape;
  QPointF startPoint;
  QPointF lastPanPoint;
  QGraphicsItem *tempShapeItem;
  QGraphicsPathItem *currentPath;
  QColor backgroundColor;
  QGraphicsEllipseItem *eraserPreview;
  QGraphicsPixmapItem *backgroundImage;

  static constexpr int MAX_BRUSH_SIZE = 150;
  static constexpr int MIN_BRUSH_SIZE = 1;
  static constexpr int BRUSH_SIZE_STEP = 2;
  static constexpr double ZOOM_FACTOR = 1.15;
  static constexpr double MAX_ZOOM = 10.0;
  static constexpr double MIN_ZOOM = 0.1;
  static constexpr int GRID_SIZE = 20;

  double currentZoom = 1.0;
  int currentOpacity = 255;
  bool showGrid = false;
  bool isPanning = false;
  bool fillShapes = false;

  QVector<QPointF> pointBuffer;
  QPointF previousPoint;

  QList<Action *> undoStack;
  QList<Action *> redoStack;

  // Private methods
  void updateEraserPreview(const QPointF &position);
  void hideEraserPreview();
  void addPoint(const QPointF &point);
  void eraseAt(const QPointF &point);
  void applyZoom(double factor);
  void fillAt(const QPointF &point);
  void drawArrow(const QPointF &start, const QPointF &end);
  void createTextItem(const QPointF &position);
  void loadDroppedImage(const QString &filePath, const QPointF &dropPosition);
  QRectF getSelectionBoundingRect() const;
};

#endif // CANVAS_H
