/**
 * @file recovery_store.h
 * @brief Crash-recovery snapshots of open documents.
 *
 * Every open document session gets its own id. Its autosaves are complete
 * native project files (.fspd, fully editable) written atomically to
 * `<dir>/<id>.fspd`, with a small `<id>.json` describing what they hold.
 * While a session is alive it holds `<id>.lock`, so a second running
 * instance never offers (or deletes) another instance's live snapshot; a
 * crashed process leaves a stale lock that is taken over on the next launch.
 */
#ifndef RECOVERY_STORE_H
#define RECOVERY_STORE_H

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <memory>

class QLockFile;

struct RecoverySnapshot {
  QString documentId;
  QString snapshotPath; ///< Native project file to load
  QString originalPath; ///< File the document came from (empty: unsaved)
  QString displayName;  ///< Document name to show the user
  QDateTime savedAt;
  int layerCount = 0;
  int itemCount = 0;
  bool legacy = false; ///< Pre-recovery-store "autosave.*" file
};

class RecoveryStore {
public:
  /// @param directory Where snapshots live; empty = app data "/recovery".
  explicit RecoveryStore(const QString &directory = QString());
  ~RecoveryStore();

  RecoveryStore(const RecoveryStore &) = delete;
  RecoveryStore &operator=(const RecoveryStore &) = delete;

  QString directory() const { return directory_; }

  /// New document session id; also takes its lock.
  QString createSession();
  /// Adopt an existing (recovered) snapshot's id for this process.
  bool adoptSession(const QString &documentId);
  /// Drop the lock of @p documentId without deleting its files.
  void releaseSession(const QString &documentId);

  /**
   * @brief Atomically write a snapshot for @p documentId.
   *
   * The previous snapshot stays valid until the new one was completely
   * written and committed; a failure never replaces it.
   */
  bool writeSnapshot(const QString &documentId, const QByteArray &projectData,
                     const RecoverySnapshot &info,
                     QString *errorMessage = nullptr);

  /// Snapshots not owned by a live session (this or another process),
  /// newest first. Incomplete or unreadable entries are skipped.
  QList<RecoverySnapshot> recoverableSnapshots() const;

  bool hasSnapshot(const QString &documentId) const;
  /// Delete a snapshot (after a successful save or an explicit discard).
  void discard(const QString &documentId);

  QString snapshotPath(const QString &documentId) const;
  QString metadataPath(const QString &documentId) const;
  QString lockPath(const QString &documentId) const;

private:
  bool isLiveElsewhere(const QString &documentId) const;

  QString directory_;
  QHash<QString, std::shared_ptr<QLockFile>> locks_;
};

#endif // RECOVERY_STORE_H
