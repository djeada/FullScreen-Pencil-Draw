#include "main_window.h"
#include "../widgets/canvas.h"
#include "../widgets/tool_panel.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)), _toolPanel(new ToolPanel(this)) {

  // Set up the layout for the main window
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);

  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_canvas);
  setCentralWidget(centralWidget);

  // Add the ToolPanel to the left side as a toolbar
  this->addToolBar(Qt::LeftToolBarArea, _toolPanel);

  // Set main window properties
  this->setWindowTitle("Paint Application");
  this->resize(800, 600);

  // Connect ToolPanel signals to Canvas slots
  connect(_toolPanel, &ToolPanel::penSelected, _canvas, &Canvas::setPenTool);
  connect(_toolPanel, &ToolPanel::eraserSelected, _canvas, &Canvas::setEraserTool);
  connect(_toolPanel, &ToolPanel::colorSelected, _canvas, &Canvas::setPenColor);

  // Use setShape instead of drawRectangle, drawCircle, and drawLine
  connect(_toolPanel, &ToolPanel::rectangleSelected, _canvas, [this]() {
    _canvas->setShape("Rectangle");
  });
  connect(_toolPanel, &ToolPanel::circleSelected, _canvas, [this]() {
    _canvas->setShape("Circle");
  });
  connect(_toolPanel, &ToolPanel::lineSelected, _canvas, [this]() {
    _canvas->setShape("Line");
  });

  // Other connections
  connect(_toolPanel, &ToolPanel::increaseBrushSize, _canvas, &Canvas::increaseBrushSize);
  connect(_toolPanel, &ToolPanel::decreaseBrushSize, _canvas, &Canvas::decreaseBrushSize);
  connect(_toolPanel, &ToolPanel::clearCanvas, _canvas, &Canvas::clearCanvas);
  connect(_toolPanel, &ToolPanel::undoAction, _canvas, &Canvas::undoLastAction);
}

MainWindow::~MainWindow() {
  // No need to manually delete _canvas or _toolPanel as Qt handles child widget memory
}
