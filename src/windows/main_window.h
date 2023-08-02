#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../widgets/canvas.h"
#include <QMainWindow>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  Canvas *canvas;
};

#endif // MAINWINDOW_H
