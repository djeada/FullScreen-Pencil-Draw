#include "core/app_constants.h"
#include "core/theme_manager.h"
#include "windows/main_window.h"
#include <QApplication>
#include <QStringList>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Set application info for QSettings
  app.setOrganizationName(AppConstants::OrganizationName);
  app.setApplicationName(AppConstants::ApplicationName);

  // Initialize theme manager (loads saved preference)
  ThemeManager::instance();

  MainWindow window;
  window.showFullScreen(); // or window.showMaximized();

  // `FullScreen-Pencil-Draw file.fspd` (or a file manager's "Open with").
  const QStringList args = app.arguments();
  if (args.size() > 1)
    window.openPath(args.at(1));
  return app.exec();
}
