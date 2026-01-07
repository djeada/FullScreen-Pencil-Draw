#include "windows/main_window.h"
#include "core/theme_manager.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  
  // Set application info for QSettings
  app.setOrganizationName("FullScreenPencilDraw");
  app.setApplicationName("FullScreenPencilDraw");
  
  // Initialize theme manager (loads saved preference)
  ThemeManager::instance();
  
  MainWindow window;
  window.showFullScreen(); // or window.showMaximized();
  return app.exec();
}
