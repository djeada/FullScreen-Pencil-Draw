// main_window.h
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QMenu>

class Canvas;
class ToolPanel;
class LayerPanel;
class QLabel;

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
  void onNewCanvas();
  void onToggleTheme();
  void onRecentFilesChanged();
  void openRecentFile();

private:
  Canvas *_canvas;
  ToolPanel *_toolPanel;
  LayerPanel *_layerPanel;
  QLabel *_statusLabel;
  QMenu *_recentFilesMenu;

  void setupConnections();
  void setupStatusBar();
  void setupLayerPanel();
  void setupMenuBar();
  void updateRecentFilesMenu();
};

#endif // MAIN_WINDOW_H
