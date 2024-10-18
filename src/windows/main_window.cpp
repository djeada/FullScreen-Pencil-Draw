#include "main_window.h"
#include "../widgets/canvas.h"
#include "../widgets/tool_panel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)), _toolPanel(new ToolPanel(this)) {

  this->addToolBar(Qt::LeftToolBarArea, _toolPanel);
  this->setCentralWidget(_canvas);
  this->setWindowTitle("Paint Application");
  this->resize(800, 600);

  // Connect ToolPanel signals to Canvas slots
  connect(_toolPanel, &ToolPanel::penSelected, _canvas, &Canvas::setPenTool);
  connect(_toolPanel, &ToolPanel::eraserSelected, _canvas, &Canvas::setEraserTool);
  connect(_toolPanel, &ToolPanel::colorSelected, _canvas, &Canvas::setPenColor);
  connect(_toolPanel, &ToolPanel::rectangleSelected, _canvas, &Canvas::drawRectangle);
  connect(_toolPanel, &ToolPanel::circleSelected, _canvas, &Canvas::drawCircle);
  connect(_toolPanel, &ToolPanel::lineSelected, _canvas, &Canvas::drawLine);
  connect(_toolPanel, &ToolPanel::increaseBrushSize, _canvas, &Canvas::increaseBrushSize);
  connect(_toolPanel, &ToolPanel::decreaseBrushSize, _canvas, &Canvas::decreaseBrushSize);
  connect(_toolPanel, &ToolPanel::clearCanvas, _canvas, &Canvas::clearCanvas);
  connect(_toolPanel, &ToolPanel::undoAction, _canvas, &Canvas::undoLastAction);
}

MainWindow::~MainWindow() {
  delete _canvas;
}
