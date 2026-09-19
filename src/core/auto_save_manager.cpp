// auto_save_manager.cpp
#include "auto_save_manager.h"
#include "../widgets/canvas.h"
#include "app_constants.h"
#include "project_serializer.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QStandardPaths>

AutoSaveManager::AutoSaveManager(Canvas *canvas, QObject *parent)
    : QObject(parent), canvas_(canvas), autoSaveTimer_(new QTimer(this)),
      enabled_(true), intervalMinutes_(DEFAULT_INTERVAL_MINUTES) {

  loadSettings();

  connect(autoSaveTimer_, &QTimer::timeout, this,
          &AutoSaveManager::performAutoSave);

  if (enabled_) {
    autoSaveTimer_->start(intervalMinutes_ * 60 * 1000);
  }
}

bool AutoSaveManager::isEnabled() const { return enabled_; }

int AutoSaveManager::intervalMinutes() const { return intervalMinutes_; }

QString AutoSaveManager::autoSavePath() const { return autoSavePath_; }

bool AutoSaveManager::hasAutoSave() const {
  return !autoSavePath_.isEmpty() && QFileInfo::exists(autoSavePath_);
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

void AutoSaveManager::performAutoSave() {
  if (!canvas_ || !canvas_->scene())
    return;
  if (shouldSave_ && !shouldSave_())
    return; // nothing new since the last save

  // Save as a project so a recovered document keeps its layers and stays
  // editable (a flattened image would come back as a locked background).
  const QString savePath = generateAutoSavePath();
  QGraphicsScene *scene = canvas_->scene();
  if (ProjectSerializer::saveProject(
          savePath, scene, canvas_->itemStore(), canvas_->layerManager(),
          scene->sceneRect(), canvas_->backgroundColor())) {
    autoSavePath_ = savePath;
    // Persist the location right away: this is what lets the next launch
    // offer recovery after a crash.
    saveSettings();
    emit autoSavePerformed(savePath);
  }
}

void AutoSaveManager::clearAutoSave() {
  if (autoSavePath_.isEmpty())
    return;
  QFile::remove(autoSavePath_);
  autoSavePath_.clear();
  saveSettings();
}

bool AutoSaveManager::restoreAutoSave() {
  if (!hasAutoSave())
    return false;

  QMessageBox::StandardButton reply = QMessageBox::question(
      nullptr, "Restore Auto-Save",
      "An auto-saved file was found. Would you like to restore it?",
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

  if (reply == QMessageBox::Yes) {
    bool restored = false;
    if (autoSavePath_.endsWith(QStringLiteral(".fspd"), Qt::CaseInsensitive)) {
      restored = canvas_->loadProjectFile(autoSavePath_,
                                          /*addToRecentFiles=*/false);
    } else {
      // Auto-saves from older versions were flattened images.
      canvas_->openRecentFile(autoSavePath_);
      restored = true;
    }
    return restored;
  }

  // User declined - clear the auto-save
  clearAutoSave();
  return false;
}

void AutoSaveManager::loadSettings() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  enabled_ = settings.value("autosave/enabled", true).toBool();
  intervalMinutes_ =
      settings.value("autosave/interval", DEFAULT_INTERVAL_MINUTES).toInt();
  autoSavePath_ = settings.value("autosave/lastPath", "").toString();
}

void AutoSaveManager::saveSettings() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  settings.setValue("autosave/enabled", enabled_);
  settings.setValue("autosave/interval", intervalMinutes_);
  settings.setValue("autosave/lastPath", autoSavePath_);
}

QString AutoSaveManager::generateAutoSavePath() const {
  QString dataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(dataDir);
  return dataDir + "/autosave.fspd";
}
