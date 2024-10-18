// main_window.h
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class Canvas;
class ToolPanel;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  // Override the keyPressEvent to handle key presses
  void keyPressEvent(QKeyEvent *event) override;

private:
  Canvas *_canvas;
  ToolPanel *_toolPanel;
};

#endif // MAIN_WINDOW_H
