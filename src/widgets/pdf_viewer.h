/**
 * @file pdf_viewer.h
 * @brief PDF viewing widget with annotation support.
 *
 * This file defines the PdfViewer class, a QGraphicsView-based widget
 * that displays PDF pages with editable overlay annotations.
 */
#ifndef PDF_VIEWER_H
#define PDF_VIEWER_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>
#include <memory>

#include "../core/action.h"
#include "../core/pdf_document.h"
#include "../core/pdf_overlay.h"

/**
 * @brief Graphics item for displaying a PDF page as background.
 *
 * This item renders a PDF page and is set as non-selectable and
 * non-movable to act as a read-only background layer.
 */
class PdfPageItem : public QGraphicsPixmapItem {
public:
  explicit PdfPageItem(QGraphicsItem *parent = nullptr);
  ~PdfPageItem() override = default;

  /**
   * @brief Set the page image
   * @param image The rendered PDF page
   */
  void setPageImage(const QImage &image);

  /**
   * @brief Check if dark mode (inverted) is enabled
   * @return true if inverted
   */
  bool isInverted() const { return inverted_; }

  /**
   * @brief Set color inversion (dark mode)
   * @param inverted Whether to invert colors
   */
  void setInverted(bool inverted);

private:
  QImage originalImage_;
  bool inverted_;

  void updatePixmap();
};

/**
 * @brief PDF viewing widget with annotation capabilities.
 *
 * PdfViewer extends QGraphicsView to provide PDF viewing with
 * overlay drawing support. It renders PDF pages as a read-only
 * background and allows editable annotations on top.
 */
class PdfViewer : public QGraphicsView {
  Q_OBJECT

public:
  /**
   * @brief Drawing tool types
   */
  enum class Tool {
    Selection,
    Pen,
    Eraser,
    Line,
    Rectangle,
    Circle,
    Arrow,
    Text,
    Fill,
    Pan
  };
  Q_ENUM(Tool)

  explicit PdfViewer(QWidget *parent = nullptr);
  ~PdfViewer() override;

  // Document operations
  /**
   * @brief Open a PDF file
   * @param filePath Path to the PDF file
   * @return true if opened successfully
   */
  bool openPdf(const QString &filePath);

  /**
   * @brief Close the current PDF
   */
  void closePdf();

  /**
   * @brief Check if a PDF is loaded
   * @return true if a PDF is loaded
   */
  bool hasPdf() const;

  /**
   * @brief Get the PDF document
   * @return Pointer to the document
   */
  PdfDocument *document() const { return document_.get(); }

  // Page navigation
  /**
   * @brief Get the current page index
   * @return Current page (0-based)
   */
  int currentPage() const { return currentPage_; }

  /**
   * @brief Get the total number of pages
   * @return Page count
   */
  int pageCount() const;

  /**
   * @brief Go to a specific page
   * @param pageIndex The page index (0-based)
   */
  void goToPage(int pageIndex);

  /**
   * @brief Go to the next page
   */
  void nextPage();

  /**
   * @brief Go to the previous page
   */
  void previousPage();

  /**
   * @brief Go to the first page
   */
  void firstPage();

  /**
   * @brief Go to the last page
   */
  void lastPage();

  // Drawing settings
  /**
   * @brief Set the current drawing tool
   * @param tool The tool to use
   */
  void setTool(Tool tool);

  /**
   * @brief Get the current tool
   * @return The current tool
   */
  Tool currentTool() const { return currentTool_; }

  /**
   * @brief Set the pen color
   * @param color The color to use
   */
  void setPenColor(const QColor &color);

  /**
   * @brief Get the current pen color
   * @return The pen color
   */
  QColor penColor() const { return currentPen_.color(); }

  /**
   * @brief Set the pen width
   * @param width Pen width in pixels
   */
  void setPenWidth(int width);

  /**
   * @brief Get the current pen width
   * @return Pen width
   */
  int penWidth() const { return currentPen_.width(); }

  /**
   * @brief Set filled shapes mode
   * @param filled Whether shapes should be filled
   */
  void setFilledShapes(bool filled);

  /**
   * @brief Check if filled shapes mode is enabled
   * @return true if shapes are filled
   */
  bool filledShapes() const { return fillShapes_; }

  // View settings
  /**
   * @brief Set dark mode (color inversion)
   * @param enabled Whether to enable dark mode
   */
  void setDarkMode(bool enabled);

  /**
   * @brief Check if dark mode is enabled
   * @return true if dark mode is on
   */
  bool darkMode() const { return darkMode_; }

  /**
   * @brief Set the rendering DPI
   * @param dpi DPI value
   */
  void setRenderDpi(int dpi);

  /**
   * @brief Get the current rendering DPI
   * @return DPI value
   */
  int renderDpi() const { return renderDpi_; }

  // Zoom
  /**
   * @brief Zoom in
   */
  void zoomIn();

  /**
   * @brief Zoom out
   */
  void zoomOut();

  /**
   * @brief Reset zoom to 100%
   */
  void zoomReset();

  /**
   * @brief Get the current zoom level
   * @return Zoom as percentage (100.0 = 100%)
   */
  double zoomLevel() const { return currentZoom_ * 100.0; }

  // Grid
  /**
   * @brief Toggle grid visibility
   */
  void toggleGrid();

  /**
   * @brief Check if grid is visible
   * @return true if grid is visible
   */
  bool isGridVisible() const { return showGrid_; }

  // Undo/Redo
  /**
   * @brief Undo the last action
   */
  void undo();

  /**
   * @brief Redo the last undone action
   */
  void redo();

  /**
   * @brief Check if undo is available
   * @return true if can undo
   */
  bool canUndo() const;

  /**
   * @brief Check if redo is available
   * @return true if can redo
   */
  bool canRedo() const;

  // Export
  /**
   * @brief Export the annotated PDF to a file
   * @param filePath Output file path
   * @return true if export succeeded
   */
  bool exportAnnotatedPdf(const QString &filePath);

  /**
   * @brief Get the graphics scene
   * @return Pointer to the scene
   */
  QGraphicsScene *scene() const { return scene_; }

signals:
  /**
   * @brief Emitted when a PDF is loaded
   */
  void pdfLoaded();

  /**
   * @brief Emitted when the PDF is closed
   */
  void pdfClosed();

  /**
   * @brief Emitted when the current page changes
   * @param pageIndex New page index
   * @param pageCount Total page count
   */
  void pageChanged(int pageIndex, int pageCount);

  /**
   * @brief Emitted when zoom changes
   * @param zoomPercent Zoom as percentage
   */
  void zoomChanged(double zoomPercent);

  /**
   * @brief Emitted when dark mode changes
   * @param enabled Whether dark mode is enabled
   */
  void darkModeChanged(bool enabled);

  /**
   * @brief Emitted when an error occurs
   * @param message Error message
   */
  void errorOccurred(const QString &message);

  /**
   * @brief Emitted when cursor position changes
   * @param pos Position in scene coordinates
   */
  void cursorPositionChanged(const QPointF &pos);

  /**
   * @brief Emitted when the document is modified
   */
  void documentModified();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
  // Document and overlay management
  std::unique_ptr<PdfDocument> document_;
  std::unique_ptr<PdfOverlayManager> overlayManager_;
  PdfPageItem *pageItem_;

  // Scene
  QGraphicsScene *scene_;

  // Current state
  int currentPage_;
  int renderDpi_;
  bool darkMode_;
  bool showGrid_;
  bool fillShapes_;
  double currentZoom_;
  Tool currentTool_;
  QPen currentPen_;
  QPen eraserPen_;

  // Drawing state
  QPointF startPoint_;
  QPointF lastPanPoint_;
  QGraphicsItem *tempShapeItem_;
  QGraphicsPathItem *currentPath_;
  QVector<QPointF> pointBuffer_;
  bool isPanning_;

  // Constants
  static constexpr int GRID_SIZE = 20;
  static constexpr double ZOOM_FACTOR = 1.15;
  static constexpr double MAX_ZOOM = 10.0;
  static constexpr double MIN_ZOOM = 0.1;
  static constexpr int DEFAULT_DPI = 150;

  // Private methods
  void renderCurrentPage();
  void setupScene();
  void addDrawAction(QGraphicsItem *item);
  void addDeleteAction(QGraphicsItem *item);
  void clearRedoStack();
  void eraseAt(const QPointF &point);
  void applyZoom(double factor);
  void addPoint(const QPointF &point);
  void createTextItem(const QPointF &pos);
  void fillAt(const QPointF &point);
  void drawArrow(const QPointF &start, const QPointF &end);
};

#endif // PDF_VIEWER_H
