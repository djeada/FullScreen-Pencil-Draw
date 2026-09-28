/**
 * @file file_dialogs.h
 * @brief Open/save dialogs that start in a sensible folder.
 *
 * With an empty directory argument QFileDialog starts in the process's
 * working directory, which for an app started from a terminal or IDE is
 * wherever it happened to be launched (e.g. a source checkout). These
 * wrappers start in the folder the user last opened or saved something in,
 * or in Documents, and remember the folder of every chosen file.
 */
#ifndef FILE_DIALOGS_H
#define FILE_DIALOGS_H

#include "app_constants.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace FileDialogs {

inline QString startDirectory() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  const QString last = settings.value("dialogs/lastDirectory").toString();
  if (!last.isEmpty() && QFileInfo(last).isDir())
    return last;
  const QString documents =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  return !documents.isEmpty() && QFileInfo(documents).isDir()
             ? documents
             : QDir::homePath();
}

inline void rememberDirectory(const QString &filePath) {
  if (filePath.isEmpty())
    return;
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  settings.setValue("dialogs/lastDirectory",
                    QFileInfo(filePath).absolutePath());
}

/// QFileDialog::getSaveFileName; an empty @p dir means startDirectory().
inline QString getSave(QWidget *parent, const QString &caption,
                       const QString &dir, const QString &filter,
                       QString *selectedFilter = nullptr) {
  const QString file = QFileDialog::getSaveFileName(
      parent, caption, dir.isEmpty() ? startDirectory() : dir, filter,
      selectedFilter);
  rememberDirectory(file);
  return file;
}

/// QFileDialog::getOpenFileName; an empty @p dir means startDirectory().
inline QString getOpen(QWidget *parent, const QString &caption,
                       const QString &dir, const QString &filter,
                       QString *selectedFilter = nullptr) {
  const QString file = QFileDialog::getOpenFileName(
      parent, caption, dir.isEmpty() ? startDirectory() : dir, filter,
      selectedFilter);
  rememberDirectory(file);
  return file;
}

} // namespace FileDialogs

#endif // FILE_DIALOGS_H
