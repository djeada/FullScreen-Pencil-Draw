// recent_files_manager.h
#ifndef RECENT_FILES_MANAGER_H
#define RECENT_FILES_MANAGER_H

#include <QObject>
#include <QStringList>

/**
 * @brief Manages the list of recently opened files.
 *
 * The RecentFilesManager tracks recently opened files, stores them
 * persistently using QSettings, and provides access to the list.
 */
class RecentFilesManager : public QObject {
  Q_OBJECT

public:
  static RecentFilesManager &instance();

  QStringList recentFiles() const;
  void addRecentFile(const QString &filePath);
  void clearRecentFiles();
  int maxRecentFiles() const;

signals:
  void recentFilesChanged();

private:
  explicit RecentFilesManager(QObject *parent = nullptr);
  ~RecentFilesManager() = default;

  // Prevent copying
  RecentFilesManager(const RecentFilesManager &) = delete;
  RecentFilesManager &operator=(const RecentFilesManager &) = delete;

  void loadRecentFiles();
  void saveRecentFiles();

  QStringList recentFiles_;
  static constexpr int MAX_RECENT_FILES = 10;
};

#endif // RECENT_FILES_MANAGER_H
