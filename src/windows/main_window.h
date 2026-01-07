// main_window.h
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenu>

class Canvas;
class ToolPanel;
class LayerPanel;
class AutoSaveManager;
class QLabel;
class QAction;

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
  void onNewCanvas();
  void onToggleTheme();
  void onRecentFilesChanged();
  void openRecentFile();
  void onAutoSavePerformed(const QString &path);

private:
  Canvas *_canvas;
  ToolPanel *_toolPanel;
  LayerPanel *_layerPanel;
  AutoSaveManager *_autoSaveManager;
  QLabel *_statusLabel;
  QMenu *_recentFilesMenu;
  QAction *_snapToGridAction;
  QAction *_autoSaveAction;

  void setupConnections();
  void setupStatusBar();
  void setupLayerPanel();
  void setupMenuBar();
  void setupAutoSave();
  void updateRecentFilesMenu();
};

#endif // MAIN_WINDOW_H
