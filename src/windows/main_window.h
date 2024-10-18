#pragma once

#include <QMainWindow>

// Forward declarations to reduce unnecessary dependencies in the header
class Canvas;
class ToolPanel;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  Canvas *_canvas;        // Canvas where drawing happens
  ToolPanel *_toolPanel;  // Toolbar for selecting tools
};
