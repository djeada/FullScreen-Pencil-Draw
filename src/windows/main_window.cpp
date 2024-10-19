// main_window.cpp
#include "main_window.h"
#include "../widgets/canvas.h"
#include "../widgets/tool_panel.h"
#include <QApplication> // Include if you choose to use qApp->quit()
#include <QKeyEvent>    // Include to handle key events
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), _canvas(new Canvas(this)),
      _toolPanel(new ToolPanel(this)) {

  // Set up the layout for the main window
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);

  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(_canvas);
  setCentralWidget(centralWidget);

  // Add the ToolPanel to the left side as a toolbar
  this->addToolBar(Qt::LeftToolBarArea, _toolPanel);

  // Set main window properties
  this->setWindowTitle("Pencil Draw");
  // Optionally, remove or comment out the resize if you are setting full screen
  // or maximized this->resize(800, 600);

  // Connect ToolPanel signals to Canvas slots
  connect(_toolPanel, &ToolPanel::penSelected, _canvas, &Canvas::setPenTool);
  connect(_toolPanel, &ToolPanel::eraserSelected, _canvas,
          &Canvas::setEraserTool);
  connect(_toolPanel, &ToolPanel::colorSelected, _canvas, &Canvas::setPenColor);

  // Use setShape instead of drawRectangle, drawCircle, and drawLine
  connect(_toolPanel, &ToolPanel::rectangleSelected, _canvas,
          [this]() { _canvas->setShape("Rectangle"); });
  connect(_toolPanel, &ToolPanel::circleSelected, _canvas,
          [this]() { _canvas->setShape("Circle"); });
  connect(_toolPanel, &ToolPanel::lineSelected, _canvas,
          [this]() { _canvas->setShape("Line"); });
  connect(_toolPanel, &ToolPanel::selectionSelected, _canvas,
          [this]() { _canvas->setShape("Selection"); });

  // Connect Copy, Cut, Paste signals
  connect(_toolPanel, &ToolPanel::copyAction, _canvas,
          &Canvas::copySelectedItems);
  connect(_toolPanel, &ToolPanel::cutAction, _canvas,
          &Canvas::cutSelectedItems);
  connect(_toolPanel, &ToolPanel::pasteAction, _canvas, &Canvas::pasteItems);

  // Other connections
  connect(_toolPanel, &ToolPanel::increaseBrushSize, _canvas,
          &Canvas::increaseBrushSize);
  connect(_toolPanel, &ToolPanel::decreaseBrushSize, _canvas,
          &Canvas::decreaseBrushSize);
  connect(_toolPanel, &ToolPanel::clearCanvas, _canvas, &Canvas::clearCanvas);
  connect(_toolPanel, &ToolPanel::undoAction, _canvas, &Canvas::undoLastAction);

  // Optionally set the window to full screen or maximized
  // Uncomment one of the following lines based on your preference:
  // this->showFullScreen();
  // this->showMaximized();
}

MainWindow::~MainWindow() {
  // No need to manually delete _canvas or _toolPanel as Qt handles child widget
  // memory
}

// Override the keyPressEvent to handle Escape key and shortcuts
void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (event->matches(QKeySequence::Copy)) {
    _canvas->copySelectedItems();
  } else if (event->matches(QKeySequence::Cut)) {
    _canvas->cutSelectedItems();
  } else if (event->matches(QKeySequence::Paste)) {
    _canvas->pasteItems();
  } else if (event->key() == Qt::Key_Escape) {
    // Option 1: Close the main window
    this->close();

    // Option 2: Quit the entire application
    // QApplication::quit();
  } else {
    // Pass the event to the base class for default handling
    QMainWindow::keyPressEvent(event);
  }
}
