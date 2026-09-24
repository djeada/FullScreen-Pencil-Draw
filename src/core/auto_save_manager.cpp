// auto_save_manager.cpp
#include "auto_save_manager.h"
#include "../widgets/canvas.h"
#include "app_constants.h"
#include "layer.h"
#include "project_serializer.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

AutoSaveManager::AutoSaveManager(Canvas *canvas, QObject *parent,
                                 const QString &recoveryDirectory)
    : QObject(parent), canvas_(canvas), autoSaveTimer_(new QTimer(this)),
      enabled_(true), intervalMinutes_(DEFAULT_INTERVAL_MINUTES),
      store_(std::make_unique<RecoveryStore>(recoveryDirectory)) {

  loadSettings();
  documentId_ = store_->createSession();

  connect(autoSaveTimer_, &QTimer::timeout, this,
          &AutoSaveManager::performAutoSave);

  if (enabled_) {
    autoSaveTimer_->start(intervalMinutes_ * 60 * 1000);
  }
}

AutoSaveManager::~AutoSaveManager() {
  // Leave the snapshot on disk (clearAutoSave() decides about deleting);
  // just release the session so the next launch can offer it.
  if (store_)
    store_->releaseSession(documentId_);
}

bool AutoSaveManager::isEnabled() const { return enabled_; }

int AutoSaveManager::intervalMinutes() const { return intervalMinutes_; }

QString AutoSaveManager::autoSavePath() const {
  return store_->snapshotPath(documentId_);
}

bool AutoSaveManager::hasAutoSave() const {
  return store_->hasSnapshot(documentId_);
}

void AutoSaveManager::setEnabled(bool enabled) {
  if (enabled_ == enabled)
    return;

  enabled_ = enabled;

  if (enabled_) {
    autoSaveTimer_->start(intervalMinutes_ * 60 * 1000);
  } else {
    autoSaveTimer_->stop();
  }

  saveSettings();
  emit autoSaveStatusChanged(enabled_);
}

void AutoSaveManager::setIntervalMinutes(int minutes) {
  minutes = qBound(MIN_INTERVAL_MINUTES, minutes, MAX_INTERVAL_MINUTES);

  if (intervalMinutes_ == minutes)
    return;

  intervalMinutes_ = minutes;

  if (enabled_) {
    autoSaveTimer_->start(intervalMinutes_ * 60 * 1000);
  }

  saveSettings();
}

bool AutoSaveManager::performAutoSave() {
  if (!canvas_ || !canvas_->scene())
    return false;
  if (shouldSave_ && !shouldSave_())
    return false; // nothing new since the last save

  // A native project snapshot keeps layers, vector objects, text and raster
  // pixels editable. Objects without a native format are flattened here
  // (a recovery copy should lose as little as possible); an explicit save
  // still asks the user about them.
  ProjectSerializer::SaveOptions options;
  options.allowRasterFallback = true;
  options.backgroundImage = canvas_->backgroundImageItem();
  ProjectSerializer::SaveStatus status;
  QGraphicsScene *scene = canvas_->scene();
  const QByteArray data = ProjectSerializer::serializeProject(
      canvas_->itemStore(), canvas_->layerManager(), scene->sceneRect(),
      canvas_->backgroundColor(), options, &status);
  if (!status.ok()) {
    emit autoSaveFailed(status.message);
    return false;
  }

  RecoverySnapshot info;
  info.originalPath = canvas_->currentFilePath();
  info.displayName = info.originalPath.isEmpty()
                         ? tr("Untitled drawing")
                         : QFileInfo(info.originalPath).fileName();
  if (LayerManager *layers = canvas_->layerManager()) {
    info.layerCount = layers->layerCount();
    for (int i = 0; i < layers->layerCount(); ++i)
      if (Layer *layer = layers->layer(i))
        info.itemCount += layer->itemCount();
  }
  QString error;
  if (!store_->writeSnapshot(documentId_, data, info, &error)) {
    // The previous snapshot (if any) is still intact.
    emit autoSaveFailed(error);
    return false;
  }
  emit autoSavePerformed(autoSavePath());
  return true;
}

void AutoSaveManager::clearAutoSave() {
  store_->discard(documentId_);
  clearLegacyAutoSave();
}

void AutoSaveManager::startNewDocument() {
  store_->discard(documentId_);
  store_->releaseSession(documentId_);
  documentId_ = store_->createSession();
}

QList<RecoverySnapshot> AutoSaveManager::recoverableSnapshots() const {
  QList<RecoverySnapshot> snapshots = store_->recoverableSnapshots();
  if (!legacyAutoSavePath_.isEmpty() &&
      QFileInfo::exists(legacyAutoSavePath_)) {
    RecoverySnapshot legacy;
    legacy.legacy = true;
    legacy.snapshotPath = legacyAutoSavePath_;
    legacy.displayName = tr("Auto-saved drawing");
    legacy.savedAt = QFileInfo(legacyAutoSavePath_).lastModified();
    snapshots.append(legacy);
  }
  return snapshots;
}

bool AutoSaveManager::recoverSnapshot(const RecoverySnapshot &snapshot) {
  if (!canvas_)
    return false;
  if (snapshot.legacy) {
    bool restored = false;
    if (snapshot.snapshotPath.endsWith(QStringLiteral(".fspd"),
                                       Qt::CaseInsensitive)) {
      restored = canvas_->loadProjectFile(snapshot.snapshotPath,
                                          /*addToRecentFiles=*/false,
                                          /*recovered=*/true);
    } else {
      // Auto-saves from much older versions were flattened images.
      canvas_->openRecentFile(snapshot.snapshotPath);
      restored = true;
    }
    if (restored) {
      // Its content now lives in this session's snapshots.
      clearLegacyAutoSave();
      emit documentRecovered(snapshot);
    }
    return restored;
  }
  if (!canvas_->loadProjectFile(snapshot.snapshotPath,
                                /*addToRecentFiles=*/false,
                                /*recovered=*/true))
    return false; // keep the snapshot; the user can try again next time
  // Continue in the recovered session: further autosaves update this
  // snapshot, and it survives until the user saves or discards the work.
  if (store_->adoptSession(snapshot.documentId)) {
    const QString previous = documentId_;
    documentId_ = snapshot.documentId;
    store_->discard(previous);
    store_->releaseSession(previous);
  }
  emit documentRecovered(snapshot);
  return true;
}

void AutoSaveManager::discardSnapshot(const RecoverySnapshot &snapshot) {
  if (snapshot.legacy) {
    clearLegacyAutoSave();
    return;
  }
  store_->discard(snapshot.documentId);
}

bool AutoSaveManager::restoreAutoSave() {
  const QList<RecoverySnapshot> snapshots = recoverableSnapshots();
  for (int i = 0; i < snapshots.size(); ++i) {
    const RecoverySnapshot &s = snapshots.at(i);
    QString details;
    if (s.legacy) {
      details = tr("An auto-saved drawing from an earlier version was "
                   "found.");
    } else {
      details = tr("Document: %1\nLast auto-saved: %2\nContents: %3 layer(s), "
                   "%4 object(s)\n\nRecovering restores the layers, vector "
                   "objects, text and raster pixels in an editable form. It "
                   "opens as an unsaved document")
                    .arg(s.displayName.isEmpty() ? tr("Untitled drawing")
                                                 : s.displayName)
                    .arg(QLocale().toString(s.savedAt.toLocalTime(),
                                            QLocale::ShortFormat))
                    .arg(s.layerCount)
                    .arg(s.itemCount);
      details += s.originalPath.isEmpty()
                     ? tr(".")
                     : tr("; \"%1\" itself is not changed until you save.")
                           .arg(QDir::toNativeSeparators(s.originalPath));
    }
    QMessageBox box(QMessageBox::Question, tr("Recover Unsaved Work"),
                    tr("FullScreen Pencil Draw did not close normally last "
                       "time and kept a recovery copy of your work."),
                    QMessageBox::NoButton);
    box.setInformativeText(details);
    QPushButton *recover =
        box.addButton(tr("Recover"), QMessageBox::AcceptRole);
    QPushButton *discard =
        box.addButton(tr("Discard"), QMessageBox::DestructiveRole);
    box.addButton(tr("Decide Later"), QMessageBox::RejectRole);
    box.setDefaultButton(recover);
    box.exec();
    if (box.clickedButton() == recover) {
      // Anything else stays on disk and is offered on the next launch.
      return recoverSnapshot(s);
    }
    if (box.clickedButton() == discard) {
      discardSnapshot(s);
      continue;
    }
    return false; // "Decide Later": keep everything, ask again next time
  }
  return false;
}

void AutoSaveManager::clearLegacyAutoSave() {
  if (legacyAutoSavePath_.isEmpty())
    return;
  QFile::remove(legacyAutoSavePath_);
  legacyAutoSavePath_.clear();
  saveSettings();
}

void AutoSaveManager::loadSettings() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  enabled_ = settings.value("autosave/enabled", true).toBool();
  intervalMinutes_ =
      settings.value("autosave/interval", DEFAULT_INTERVAL_MINUTES).toInt();
  legacyAutoSavePath_ = settings.value("autosave/lastPath", "").toString();
}

void AutoSaveManager::saveSettings() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  settings.setValue("autosave/enabled", enabled_);
  settings.setValue("autosave/interval", intervalMinutes_);
  if (legacyAutoSavePath_.isEmpty())
    settings.remove("autosave/lastPath");
  else
    settings.setValue("autosave/lastPath", legacyAutoSavePath_);
}
