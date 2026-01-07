// recent_files_manager.cpp
#include "recent_files_manager.h"
#include <QFileInfo>
#include <QSettings>

RecentFilesManager &RecentFilesManager::instance() {
  static RecentFilesManager instance;
  return instance;
}

RecentFilesManager::RecentFilesManager(QObject *parent) : QObject(parent) {
  loadRecentFiles();
}

QStringList RecentFilesManager::recentFiles() const { return recentFiles_; }

void RecentFilesManager::addRecentFile(const QString &filePath) {
  // Normalize the path
  QString normalizedPath = QFileInfo(filePath).absoluteFilePath();

  // Remove if already exists (to move it to the front)
  recentFiles_.removeAll(normalizedPath);

  // Add to the front
  recentFiles_.prepend(normalizedPath);

  // Limit the list size
  while (recentFiles_.size() > MAX_RECENT_FILES) {
    recentFiles_.removeLast();
  }

  saveRecentFiles();
  emit recentFilesChanged();
}

void RecentFilesManager::clearRecentFiles() {
  recentFiles_.clear();
  saveRecentFiles();
  emit recentFilesChanged();
}

int RecentFilesManager::maxRecentFiles() const { return MAX_RECENT_FILES; }

void RecentFilesManager::loadRecentFiles() {
  QSettings settings("FullScreenPencilDraw", "FullScreenPencilDraw");
  recentFiles_ = settings.value("recentFiles").toStringList();

  // Remove any files that no longer exist
  QStringList validFiles;
  for (const QString &file : recentFiles_) {
    if (QFileInfo::exists(file)) {
      validFiles.append(file);
    }
  }

  if (validFiles.size() != recentFiles_.size()) {
    recentFiles_ = validFiles;
    saveRecentFiles();
  }
}

void RecentFilesManager::saveRecentFiles() {
  QSettings settings("FullScreenPencilDraw", "FullScreenPencilDraw");
  settings.setValue("recentFiles", recentFiles_);
}
