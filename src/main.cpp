#include "windows/main_window.h"
#include "core/app_constants.h"
#include "core/theme_manager.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  
  // Set application info for QSettings
  app.setOrganizationName(AppConstants::OrganizationName);
  app.setApplicationName(AppConstants::ApplicationName);
  
  // Initialize theme manager (loads saved preference)
  ThemeManager::instance();
  
  MainWindow window;
  window.showFullScreen(); // or window.showMaximized();
  return app.exec();
}
