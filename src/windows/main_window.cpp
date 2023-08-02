#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), canvas(new Canvas(this)) {
  this->setCentralWidget(canvas);
  this->setWindowTitle("Paint Application");
  this->resize(800, 600); // Set to desired initial window size
}

MainWindow::~MainWindow() { delete canvas; }
