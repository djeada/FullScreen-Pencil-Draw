// auto_save_manager.h
#ifndef AUTO_SAVE_MANAGER_H
#define AUTO_SAVE_MANAGER_H

#include "recovery_store.h"
#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>
#include <memory>

class Canvas;

/**
 * @brief Periodically writes crash-recovery snapshots of the open document.
 *
 * Snapshots are complete native project files (layers, vector objects,
 * text, raster pixels) managed by a RecoveryStore, one per document
 * session, so they restore an editable document rather than a flattened
 * picture and one document's autosave never overwrites another's. A
 * snapshot is only deleted once the document was saved, closed cleanly or
 * explicitly discarded.
 */
class AutoSaveManager : public QObject {
  Q_OBJECT

public:
  /// @param recoveryDirectory Empty = the default app data location.
  explicit AutoSaveManager(Canvas *canvas, QObject *parent = nullptr,
                           const QString &recoveryDirectory = QString());
  ~AutoSaveManager() override;

  bool isEnabled() const;
  int intervalMinutes() const;
  /// Snapshot file of the current document session.
  QString autoSavePath() const;
  /// Whether the current document session has a snapshot on disk.
  bool hasAutoSave() const;
  QString documentId() const { return documentId_; }
  RecoveryStore &recoveryStore() { return *store_; }

  /// Snapshots left behind by sessions that did not end cleanly (plus a
  /// pre-recovery-store autosave file, if any), newest first.
  QList<RecoverySnapshot> recoverableSnapshots() const;

  /// Auto-save only runs while this returns true (e.g. unsaved changes).
  void setShouldSaveCheck(std::function<bool()> check) {
    shouldSave_ = std::move(check);
  }

public slots:
  void setEnabled(bool enabled);
  void setIntervalMinutes(int minutes);
  /// @return true if a snapshot was written.
  bool performAutoSave();
  /// The document was saved or deliberately discarded: drop its snapshot.
  void clearAutoSave();
  /// A different document replaced the current one (new/open): drop the
  /// old snapshot and start a new session id.
  void startNewDocument();
  /// Ask the user about each recoverable snapshot (startup).
  /// @return true if a document was recovered into the canvas.
  bool restoreAutoSave();
  /// Load @p snapshot into the canvas as an unsaved document and continue
  /// autosaving into it; the snapshot is kept until the user saves.
  bool recoverSnapshot(const RecoverySnapshot &snapshot);
  /// Delete a snapshot the user chose not to recover.
  void discardSnapshot(const RecoverySnapshot &snapshot);

signals:
  void autoSavePerformed(const QString &path);
  void autoSaveFailed(const QString &message);
  void autoSaveStatusChanged(bool enabled);
  /// A snapshot was recovered: the document is unsaved work again.
  void documentRecovered(const RecoverySnapshot &snapshot);

private:
  Canvas *canvas_;
  QTimer *autoSaveTimer_;
  bool enabled_;
  int intervalMinutes_;
  std::unique_ptr<RecoveryStore> store_;
  QString documentId_;
  QString legacyAutoSavePath_; ///< "autosave.fspd/png" of older versions
  std::function<bool()> shouldSave_;

  void loadSettings();
  void saveSettings();
  void clearLegacyAutoSave();

  static constexpr int DEFAULT_INTERVAL_MINUTES = 5;
  static constexpr int MIN_INTERVAL_MINUTES = 1;
  static constexpr int MAX_INTERVAL_MINUTES = 60;
};

#endif // AUTO_SAVE_MANAGER_H
