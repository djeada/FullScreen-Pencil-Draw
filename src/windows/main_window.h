// main_window.h
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class Canvas;
class ToolPanel;
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

private:
  Canvas *_canvas;
  ToolPanel *_toolPanel;
  QLabel *_statusLabel;

  void setupConnections();
  void setupStatusBar();
};

#endif // MAIN_WINDOW_H
