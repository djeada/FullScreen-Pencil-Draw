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
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHash>
#include <QImage>
#include <QList>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPen>
#include <QTabletEvent>
#include <QVector>
#include <QWheelEvent>
#include <functional>
#include <memory>
#include <vector>

#include "../core/action.h"
#include "../core/brush_tip.h"
#include "../core/item_id.h"
#include "../core/layer.h"
#include "../core/scene_renderer.h"
#include "../core/snap_engine.h"

class ToolManager;
class Tool;
class BezierTool;
class TextOnPathTool;
class TransformHandleItem;
class SceneController;
class ItemStore;
class BusySpinnerOverlay;
class UndoRedoManager;
class ElectronicsElementItem;
class WireItem;
struct ExportResult;

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
  bool isSnapToObjectEnabled() const;
  bool isRulerVisible() const;
  bool isMeasurementToolEnabled() const;
  bool isPressureSensitive() const override { return pressureSensitive_; }
  int pixelEraserStrength() const { return qRound(pixelEraserStrength_ * 100); }
  int pixelEraserHardness() const { return qRound(pixelEraserHardness_ * 100); }
  bool isPixelEraserActive() const { return currentShape_ == PixelEraser; }
  bool isObjectEraserActive() const { return currentShape_ == Eraser; }
  int colorSelectTolerance() const { return colorSelectTolerance_; }
  bool isColorSelectContiguous() const { return colorSelectContiguous_; }
  bool hasActiveColorSelection() const;

  // Tool system accessors - implements SceneRenderer interface
  QGraphicsScene *scene() const override { return scene_; }
  const QPen &currentPen() const override { return currentPen_; }
  const QPen &eraserPen() const override { return eraserPen_; }
  QBrush currentBrush() const override { return fillBrush_; }
  const BrushTip &currentBrushTip() const override { return brushTip_; }
  QGraphicsPixmapItem *backgroundImageItem() const override {
    return backgroundImage_;
  }
  QColor backgroundColor() const { return backgroundColor_; }
  SceneController *sceneController() const override { return sceneController_; }
  ItemStore *itemStore() const override;
  ItemId registerItem(QGraphicsItem *item) override;

  // Layer management
  LayerManager *layerManager() const { return layerManager_; }
  /**
   * @brief Called before an operation replaces the whole document (opening a
   *        project); returning false cancels the operation.
   */
  void setDiscardChangesHandler(std::function<bool()> handler) {
    discardChangesHandler_ = std::move(handler);
  }
  /// Commit every open inline text/mermaid editor except @p except.
  void finishInlineEditing(const QGraphicsItem *except = nullptr);
  /**
   * @brief Load a .fspd file, replacing the document (after confirming).
   * @param recovered The file is a crash-recovery snapshot: the document
   *        opens as unsaved work with no file name (so saving asks where
   *        instead of overwriting anything) and documentRecovered() is
   *        emitted instead of documentLoaded().
   */
  bool loadProjectFile(const QString &fileName, bool addToRecentFiles = true,
                       bool recovered = false);
  /// Native project file the document was last opened from / saved to.
  QString currentFilePath() const { return currentFilePath_; }
  /**
   * @brief Save the document as a native project to @p fileName.
   *
   * Objects without an editable project format are listed to the user,
   * who can cancel or approve flattening them; failures are reported and
   * leave the document marked modified.
   * @param interactive Show confirmation/error dialogs.
   */
  bool saveProjectTo(const QString &fileName, bool interactive = true);

  // Action management - implements SceneRenderer interface
  void addDrawAction(QGraphicsItem *item) override;
  void addDeleteAction(QGraphicsItem *item) override;
  void addAction(std::unique_ptr<Action> action) override;
  void onItemRemoved(QGraphicsItem *item) override;
  void setUndoRedoManager(UndoRedoManager *manager);

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
  void fillBrushChanged(const QBrush &brush);
  void snapToGridChanged(bool enabled);
  void snapToObjectChanged(bool enabled);
  void canvasModified();
  /// The document was completely written to its native project file.
  void documentSaved();
  /// A native project replaced the document (it matches its file again).
  void documentLoaded();
  /// A recovery snapshot replaced the document (it is unsaved work).
  void documentRecovered();
  /// A save or export failed; the document keeps its modified state.
  void saveFailed(const QString &message);
  /// Short user-facing hint (e.g. why a tool did nothing).
  void statusMessage(const QString &message);
  void rulerVisibilityChanged(bool visible);
  void measurementToolChanged(bool enabled);
  void measurementUpdated(const QString &measurement);
  void pdfFileDropped(const QString &filePath);
  void pressureSensitivityChanged(bool enabled);
  void brushTipChanged(const BrushTip &tip);

public:
  /**
   * @brief Commit an in-progress multi-click path gesture (Bezier / text on
   * path), if one is running.
   * @return true when a gesture was active and has been committed
   */
  bool finishActiveGesture();

  /**
   * @brief Discard an in-progress multi-click path gesture, if one is running.
   * @return true when a gesture was active and has been discarded
   */
  bool cancelActiveGesture();

public slots:
  /// Rebuild/refresh selection handles, e.g. after undo/redo moved items.
  void updateTransformHandles();
  void setShape(const QString &shapeType);
  void deselectAll();
  void setPenTool();
  void setHighlighterTool();
  /// Object Eraser: deletes whole objects it touches (E).
  void setEraserTool();
  void setObjectEraserTool() { setEraserTool(); }
  /// Pixel Eraser: removes pixels from raster content on the active layer
  /// (images, brush strokes, raster layers) and never deletes objects.
  void setPixelEraserTool();
  /// Pixel Eraser strength (alpha removed per dab) and edge hardness, %.
  void setPixelEraserStrength(int percent);
  void setPixelEraserHardness(int percent);
  void setTextTool();
  void setMermaidTool();
  void setFillTool();
  void setColorSelectTool();
  void setArrowTool();
  void setCurvedArrowTool();
  void setBezierTool();
  void setTextOnPathTool();
  void setPanTool();
  void setWireTool();
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
  /// Export dialog (images, PDF, SVG); a .fspd choice saves the project.
  void saveToFile();
  void openFile();
  /// Save As for the native project format.
  void saveProject();
  /// Save to the current project file, or ask for one (Ctrl+S).
  void saveDocument();
  void openProject();
  void newCanvas(int width, int height, const QColor &bgColor);
  void toggleGrid();
  void toggleFilledShapes();
  void setFillBrush(const QBrush &brush);
  void toggleSnapToGrid();
  void toggleSnapToObject();
  void toggleRuler();
  void toggleMeasurementTool();
  void togglePressureSensitivity();
  void setBrushTip(const BrushTip &tip);
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
  void rotateSelectedItems();
  void alignSelectedItems();
  void changeColorOfSelectedItems();
  void perspectiveTransformSelectedItems();
  void scaleActiveLayer();
  void resizeCanvas();
  void applyBlurToSelection();
  void applySharpenToSelection();
  void applyScanDocumentToSelection();
  void applyColorCurvesToSelection();
  void exportSingleElementToPNG();
  void openSingleImage();
  void placeElement(const QString &elementId);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
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
  bool isSelectableCanvasItem(const QGraphicsItem *item) const;
  QGraphicsItem *selectableCanvasItemAtViewportPos(const QPoint &viewPos) const;
  QGraphicsItem *selectableCanvasItemNearViewportPos(const QPoint &viewPos,
                                                     int radiusPx) const;
  QGraphicsItem *movableCanvasItemNearViewportPos(const QPoint &viewPos,
                                                  int radiusPx) const;

  // Enums
  enum ShapeType {
    Line,
    Rectangle,
    Circle,
    Pen,
    Highlighter,
    Eraser,
    Selection,
    LassoSelection,
    Text,
    Mermaid,
    Fill,
    ColorSelect,
    Arrow,
    Pan,
    CurvedArrow,
    Bezier,
    TextOnPath,
    Wire,
    PixelEraser
  };

  // Member variables
  QGraphicsScene *scene_;
  SceneController *sceneController_;
  LayerManager *layerManager_;
  int wheelZoomAccum_ = 0; ///< Partial Ctrl+wheel delta (1/8 degrees)
  std::function<bool()> discardChangesHandler_;
  QPen currentPen_;
  QPen eraserPen_;
  QBrush fillBrush_;
  ShapeType currentShape_;
  QPointF startPoint_;
  QPointF lastPanPoint_;
  QGraphicsItem *tempShapeItem_;
  QGraphicsPathItem *currentPath_;
  QColor backgroundColor_;
  QGraphicsEllipseItem *eraserPreview_;
  QGraphicsPixmapItem *backgroundImage_;
  QString currentFilePath_;

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
  bool snapToObject_ = false;
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

  // Lasso selection state
  QGraphicsPathItem *lassoPathItem_ = nullptr;
  QVector<QPointF> lassoPoints_;
  bool lassoDrawing_ = false;

  // Drawing state
  QVector<QPointF> pointBuffer_;
  QPointF previousPoint_;
  QPointF measurementStart_;

  // Pressure sensitivity state
  bool pressureSensitive_ = false;
  qreal tabletPressure_ = 1.0;
  bool tabletActive_ = false;
  QVector<qreal> pressureBuffer_;
  QPainterPath pressureCommitted_; ///< Frozen start of a long pressure stroke
  QPainterPath pressureOutline(int begin, int end) const;

  // Eraser gesture: one composite undo entry per stroke sweep
  std::unique_ptr<CompositeAction> eraserStrokeAction_;

  // Pixel Eraser gesture: every raster target touched so far, with its
  // state before the gesture (tiles are recorded by the surface itself).
  struct PixelEraseTarget {
    bool tiled = false;
    QImage before;
    QImage working;
  };
  QHash<ItemId, PixelEraseTarget> pixelEraseTargets_;
  bool pixelEraseActive_ = false;
  bool pixelEraseHasLast_ = false;
  QPointF pixelEraseLast_;
  bool pixelEraseHinted_ = false;
  qreal pixelEraserStrength_ = 1.0;
  qreal pixelEraserHardness_ = 1.0;
  void beginPixelErase();
  void pixelEraseAt(const QPointF &scenePos);
  void endPixelErase();

  // Painting on a raster layer (Pen tool with a Raster layer active).
  ItemId rasterStrokeItemId_;
  bool rasterStrokeCreated_ = false;
  bool rasterStrokeActive_ = false;
  QPointF rasterStrokeLast_;
  bool beginRasterStroke(const QPointF &scenePos);
  void continueRasterStroke(const QPointF &scenePos);
  void endRasterStroke();

  // Brush tip
  BrushTip brushTip_;

  // Undo/Redo stacks (using vector for move-only types)
  std::vector<std::unique_ptr<Action>> undoStack_;
  std::vector<std::unique_ptr<Action>> redoStack_;
  UndoRedoManager *undoRedoManager_ = nullptr;

  // Private methods
  void updateEraserPreview(const QPointF &position);
  void hideEraserPreview();
  void beginEraseStroke();
  void endEraseStroke();
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
  void reportExportResult(const QString &fileName, const ExportResult &result);
  bool selectByColorAt(const QPointF &scenePoint,
                       Qt::KeyboardModifiers modifiers);
  QGraphicsPixmapItem *findPixmapItemAt(const QPointF &scenePoint) const;
  QImage createColorSelectionMask(const QImage &image, const QPoint &seed,
                                  int tolerance, bool contiguous) const;
  void refreshColorSelectionOverlay();
  void resetColorSelection();
  // Export helpers
  QList<QGraphicsItem *> documentItems() const;
  QList<QGraphicsItem *> exportableSelection() const;
  static QRectF visibleBounds(const QList<QGraphicsItem *> &items);
  QRectF documentExportRect(const QList<QGraphicsItem *> &items) const;
  void renderItems(QPainter *painter, const QRectF &target,
                   const QRectF &source, const QList<QGraphicsItem *> &items);
  QImage renderItemsToImage(const QList<QGraphicsItem *> &items,
                            const QRectF &source, QImage::Format format,
                            const QColor &fill);
  void exportSelectionImage(const QString &title, const QString &filter,
                            const char *format, QImage::Format imageFormat,
                            const QColor &fill);
  QPointF snapToGridPoint(const QPointF &point) const;
  QPointF snapPoint(const QPointF &point,
                    const QSet<QGraphicsItem *> &excludeItems = {});
  QPointF calculateSmartDuplicateOffset() const;
  void drawRuler(QPainter *painter, const QRectF &rect);
  QString calculateDistance(const QPointF &p1, const QPointF &p2) const;
  void clearTransformHandles();
  void applyResizeToOtherItems(QGraphicsItem *sourceItem, qreal scaleX,
                               qreal scaleY, const QPointF &anchor);
  void applyRotationToOtherItems(QGraphicsItem *sourceItem, qreal angleDelta,
                                 const QPointF &center);
  void savePreTransformStates();
  void createTransformUndoActions();
  std::unique_ptr<DrawAction> prepareDrawAction(QGraphicsItem *item);
  std::unique_ptr<DeleteAction> prepareDeleteAction(QGraphicsItem *item);
  bool hasNonNormalBlendModes() const;
  void importSvg(const QString &filePath, const QPointF &position = QPointF());

  // Pre-transform state for undo/redo of all selected items
  struct ItemPreTransformState {
    ItemId id;
    QTransform transform;
    QPointF pos;
    QFont font;
    bool isTextItem = false;
  };
  QList<ItemPreTransformState> preTransformStates_;

  // Snap engine and visual guides
  SnapEngine snapEngine_;
  SnapResult lastSnapResult_;
  bool hasActiveSnap_ = false;

  // Progress overlay for long-running operations
  BusySpinnerOverlay *busySpinner_ = nullptr;
  void showBusySpinner(const QString &text);
  void hideBusySpinner();

  // Wire-drawing state
  ElectronicsElementItem *wireSrcElem_ = nullptr;
  int wireSrcPin_ = -1;
  QGraphicsPathItem *wireTempPath_ = nullptr;    // Manhattan-routed preview
  QGraphicsEllipseItem *pinHighlight_ = nullptr; // hover highlight ring
  void cleanupTransientToolState();
  /// Remove every item and the undo history without asking.
  void resetDocument();
  /// fitInView() that keeps currentZoom_ and the zoom display in sync.
  void fitRectInView(const QRectF &rect);
  /// True if @p item (or its top-level group) or its layer is locked.
  bool isItemLocked(QGraphicsItem *item) const;

  /**
   * @brief Drop every in-progress drawing gesture and its preview items.
   *
   * Called before the scene is torn down (clear / new / load) so no tool is
   * left holding a pointer to an item the teardown is about to delete.
   */
  void abortInProgressDrawing();

  // Multi-click path tools (Bezier, text-on-path). These are the shared
  // Tool implementations also used by PdfViewer; the canvas drives them
  // directly instead of duplicating their logic.
  std::unique_ptr<BezierTool> bezierTool_;
  std::unique_ptr<TextOnPathTool> textOnPathTool_;
  Tool *activePathTool_ = nullptr;
  Tool *pathToolFor(ShapeType shape);
  ElectronicsElementItem *findElectronicsElementNear(const QPointF &scenePos,
                                                     int &pinIndex) const;
  bool wireAlreadyExists(ElectronicsElementItem *a, int ap,
                         ElectronicsElementItem *b, int bp) const;
};

#endif // CANVAS_H
