// main_window.h
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QSplitter>

class Canvas;
class ToolPanel;
class LayerPanel;
class AutoSaveManager;
class QLabel;
class QAction;
class QToolBar;

#ifdef HAVE_QT_PDF
class PdfViewer;
class QFrame;
#endif

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void onBrushSizeChanged(int size);
  void onColorChanged(const QColor &color);
  void onZoomChanged(double zoom);
  void onOpacityChanged(int opacity);
  void onCursorPositionChanged(const QPointF &pos);
  void onFilledShapesChanged(bool filled);
  void onSnapToGridChanged(bool enabled);
  void onRulerVisibilityChanged(bool visible);
  void onMeasurementToolChanged(bool enabled);
  void onMeasurementUpdated(const QString &measurement);
  void onNewCanvas();
  void onToggleTheme();
  void onRecentFilesChanged();
  void openRecentFile();
  void onAutoSavePerformed(const QString &path);

#ifdef HAVE_QT_PDF
  // PDF viewer slots
  void onOpenPdf();
  void onClosePdf();
  void onPdfPageChanged(int pageIndex, int pageCount);
  void onPdfZoomChanged(double zoom);
  void onPdfDarkModeChanged(bool enabled);
  void onExportAnnotatedPdf();
  void onPdfFileDropped(const QString &filePath);
#endif

private:
  Canvas *_canvas;
  ToolPanel *_toolPanel;
  LayerPanel *_layerPanel;
  AutoSaveManager *_autoSaveManager;
  QLabel *_statusLabel;
  QLabel *_measurementLabel;
  QMenu *_recentFilesMenu;
  QAction *_snapToGridAction;
  QAction *_autoSaveAction;
  QAction *_rulerAction;
  QAction *_measurementAction;

#ifdef HAVE_QT_PDF
  PdfViewer *_pdfViewer;
  QSplitter *_centralSplitter;
  QFrame *_pdfPanel;
  QToolBar *_pdfToolBar;
  QLabel *_pdfPageLabel;
  QAction *_pdfDarkModeAction;
  bool _pdfPanelOnLeft;  // Track if PDF panel is on the left side

  void setupPdfViewer();
  void setupPdfToolBar();
  void showPdfPanel();
  void hidePdfPanel();
  void swapPanelOrder();  // Swap the order of canvas and PDF panel
#endif

  void setupConnections();
  void setupStatusBar();
  void setupLayerPanel();
  void setupMenuBar();
  void setupAutoSave();
  void updateRecentFilesMenu();
};

#endif // MAIN_WINDOW_H
