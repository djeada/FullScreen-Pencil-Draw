/**
 * @file canvas.h
 * @brief Main drawing canvas widget.
 *
 * The Canvas class provides the primary drawing surface for the application.
 * It uses a QGraphicsView/QGraphicsScene architecture with a modular tool
 * system for different drawing operations.
 */
#ifndef CANVAS_H
#define CANVAS_H

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QImage>
#include <QGraphicsView>
#include <QHash>
#include <QList>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPen>
#include <QTabletEvent>
#include <QVector>
#include <QWheelEvent>
#include <memory>
#include <vector>

#include "../core/action.h"
#include "../core/layer.h"
#include "../core/scene_renderer.h"

class ToolManager;
class Tool;
class TransformHandleItem;
class SceneController;
class ItemStore;

/**
 * @brief The main drawing canvas widget.
 *
 * Canvas is a QGraphicsView-based widget that provides the drawing surface.
 * It integrates with the ToolManager for tool-based drawing operations
 * and maintains undo/redo stacks for all actions.
 *
 * Canvas implements the SceneRenderer interface, allowing drawing tools
 * to work with it through a common abstraction.
 */
class Canvas : public QGraphicsView, public SceneRenderer {
  Q_OBJECT

public:
  explicit Canvas(QWidget *parent = nullptr);
  ~Canvas();

  // State accessors
  int getCurrentBrushSize() const;
  QColor getCurrentColor() const;
  double getCurrentZoom() const;
  int getCurrentOpacity() const;
  bool isGridVisible() const;
  bool isFilledShapes() const override;
  bool isSnapToGridEnabled() const;
  bool isRulerVisible() const;
  bool isMeasurementToolEnabled() const;
  bool isPressureSensitive() const override { return pressureSensitive_; }
  int colorSelectTolerance() const { return colorSelectTolerance_; }
  bool isColorSelectContiguous() const { return colorSelectContiguous_; }
  bool hasActiveColorSelection() const;

  // Tool system accessors - implements SceneRenderer interface
  QGraphicsScene *scene() const override { return scene_; }
  const QPen &currentPen() const override { return currentPen_; }
  const QPen &eraserPen() const override { return eraserPen_; }
  QGraphicsPixmapItem *backgroundImageItem() const override {
    return backgroundImage_;
  }
  QColor backgroundColor() const { return backgroundColor_; }
  SceneController *sceneController() const override { return sceneController_; }
  ItemStore *itemStore() const override;
  ItemId registerItem(QGraphicsItem *item) override;

  // Layer management
  LayerManager *layerManager() const { return layerManager_; }

  // Action management - implements SceneRenderer interface
  void addDrawAction(QGraphicsItem *item) override;
  void addDeleteAction(QGraphicsItem *item) override;
  void addAction(std::unique_ptr<Action> action) override;
  void onItemRemoved(QGraphicsItem *item) override;

  // SceneRenderer interface methods using QGraphicsView
  void setCursor(const QCursor &cursor) override {
    QGraphicsView::setCursor(cursor);
  }
  QScrollBar *horizontalScrollBar() const override {
    return QGraphicsView::horizontalScrollBar();
  }
  QScrollBar *verticalScrollBar() const override {
    return QGraphicsView::verticalScrollBar();
  }
  void clearRedoStack();

signals:
  void brushSizeChanged(int size);
  void colorChanged(const QColor &color);
  void zoomChanged(double zoomPercent);
  void opacityChanged(int opacity);
  void cursorPositionChanged(const QPointF &pos);
  void filledShapesChanged(bool filled);
  void snapToGridChanged(bool enabled);
  void canvasModified();
  void rulerVisibilityChanged(bool visible);
  void measurementToolChanged(bool enabled);
  void measurementUpdated(const QString &measurement);
  void pdfFileDropped(const QString &filePath);
  void pressureSensitivityChanged(bool enabled);

public slots:
  void setShape(const QString &shapeType);
  void setPenTool();
  void setEraserTool();
  void setTextTool();
  void setMermaidTool();
  void setFillTool();
  void setColorSelectTool();
  void setArrowTool();
  void setCurvedArrowTool();
  void setBezierTool();
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
  void saveProject();
  void openProject();
  void newCanvas(int width, int height, const QColor &bgColor);
  void toggleGrid();
  void toggleFilledShapes();
  void toggleSnapToGrid();
  void toggleRuler();
  void toggleMeasurementTool();
  void togglePressureSensitivity();
  void lockSelectedItems();
  void unlockSelectedItems();
  void selectAll();
  void groupSelectedItems();
  void ungroupSelectedItems();
  void bringToFront();
  void bringForward();
  void sendBackward();
  void sendToBack();
  void exportSelectionToSVG();
  void exportSelectionToPNG();
  void exportSelectionToJPG();
  void exportSelectionToWebP();
  void exportSelectionToTIFF();
  void exportToPDF();
  void extractColorSelectionToNewLayer();
  void clearColorSelection();
  void setColorSelectTolerance();
  void toggleColorSelectContiguous();
  void openRecentFile(const QString &filePath);
  void addImageFromScreenshot(const QImage &image);
  void scaleSelectedItems();
  void scaleActiveLayer();
  void resizeCanvas();
  void applyBlurToSelection();
  void applySharpenToSelection();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void tabletEvent(QTabletEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void drawBackground(QPainter *painter, const QRectF &rect) override;
  void drawForeground(QPainter *painter, const QRectF &rect) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  // Enums
  enum ShapeType {
    Line,
    Rectangle,
    Circle,
    Pen,
    Eraser,
    Selection,
    Text,
    Mermaid,
    Fill,
    ColorSelect,
    Arrow,
    Pan,
    CurvedArrow,
    Bezier
  };

  // Member variables
  QGraphicsScene *scene_;
  SceneController *sceneController_;
  LayerManager *layerManager_;
  QPen currentPen_;
  QPen eraserPen_;
  ShapeType currentShape_;
  QPointF startPoint_;
  QPointF lastPanPoint_;
  QGraphicsItem *tempShapeItem_;
  QGraphicsPathItem *currentPath_;
  QColor backgroundColor_;
  QGraphicsEllipseItem *eraserPreview_;
  QGraphicsPixmapItem *backgroundImage_;

  // Transform handles for selected items
  QList<TransformHandleItem *> transformHandles_;

  // Constants
  static constexpr int MAX_BRUSH_SIZE = 150;
  static constexpr int MIN_BRUSH_SIZE = 1;
  static constexpr int BRUSH_SIZE_STEP = 2;
  static constexpr double ZOOM_FACTOR = 1.15;
  static constexpr double MAX_ZOOM = 10.0;
  static constexpr double MIN_ZOOM = 0.1;
  static constexpr int GRID_SIZE = 20;
  static constexpr int RULER_SIZE = 25;

  // State variables
  double currentZoom_ = 1.0;
  int currentOpacity_ = 255;
  bool showGrid_ = false;
  bool isPanning_ = false;
  bool fillShapes_ = false;
  bool snapToGrid_ = false;
  bool showRuler_ = false;
  bool measurementToolEnabled_ = false;
  int duplicateOffset_ = 20;
  bool dragAccepted_ = false;
  bool curvedArrowAutoBendEnabled_ = true;
  bool curvedArrowManualFlip_ = false;
  bool curvedArrowShiftWasDown_ = false;
  bool trackingSelectionMove_ = false;
  int colorSelectTolerance_ = 32;
  bool colorSelectContiguous_ = true;
  ItemId colorSelectionItemId_;
  QImage colorSelectionMask_;
  bool colorSelectionHasPixels_ = false;
  QGraphicsPixmapItem *colorSelectionOverlay_ = nullptr;
  QHash<ItemId, QPointF> selectionMoveStartPositions_;

  // Drawing state
  QVector<QPointF> pointBuffer_;
  QPointF previousPoint_;
  QPointF measurementStart_;

  // Pressure sensitivity state
  bool pressureSensitive_ = false;
  qreal tabletPressure_ = 1.0;
  bool tabletActive_ = false;
  QVector<qreal> pressureBuffer_;

  // Undo/Redo stacks (using vector for move-only types)
  std::vector<std::unique_ptr<Action>> undoStack_;
  std::vector<std::unique_ptr<Action>> redoStack_;

  // Private methods
  void updateEraserPreview(const QPointF &position);
  void hideEraserPreview();
  void addPoint(const QPointF &point);
  void addPressurePoint(const QPointF &point, qreal pressure);
  void eraseAt(const QPointF &point);
  void applyZoom(double factor);
  void fillAt(const QPointF &point);
  void drawArrow(const QPointF &start, const QPointF &end);
  void drawCurvedArrow(const QPointF &start, const QPointF &end,
                       Qt::KeyboardModifiers modifiers = Qt::NoModifier);
  void createTextItem(const QPointF &position);
  void createMermaidItem(const QPointF &position);
  void loadDroppedImage(const QString &filePath, const QPointF &dropPosition);
  void exportToPDFWithFilename(const QString &fileName);
  bool selectByColorAt(const QPointF &scenePoint,
                       Qt::KeyboardModifiers modifiers);
  QGraphicsPixmapItem *findPixmapItemAt(const QPointF &scenePoint) const;
  QImage createColorSelectionMask(const QImage &image, const QPoint &seed,
                                  int tolerance, bool contiguous) const;
  void refreshColorSelectionOverlay();
  void resetColorSelection();
  QRectF getSelectionBoundingRect() const;
  QPointF snapToGridPoint(const QPointF &point) const;
  QPointF calculateSmartDuplicateOffset() const;
  void drawRuler(QPainter *painter, const QRectF &rect);
  QString calculateDistance(const QPointF &p1, const QPointF &p2) const;
  void updateTransformHandles();
  void clearTransformHandles();
  void applyResizeToOtherItems(QGraphicsItem *sourceItem, qreal scaleX,
                               qreal scaleY, const QPointF &anchor);
  void applyRotationToOtherItems(QGraphicsItem *sourceItem, qreal angleDelta,
                                 const QPointF &center);
  bool hasNonNormalBlendModes() const;
  void importSvg(const QString &filePath,
                 const QPointF &position = QPointF());
};

#endif // CANVAS_H
