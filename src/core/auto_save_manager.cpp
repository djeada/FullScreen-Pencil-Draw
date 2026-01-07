// auto_save_manager.cpp
#include "auto_save_manager.h"
#include "app_constants.h"
#include "../widgets/canvas.h"
#include <QApplication>
#include <QDir>
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
  
  connect(autoSaveTimer_, &QTimer::timeout, this, &AutoSaveManager::performAutoSave);
  
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
  if (enabled_ == enabled) return;
  
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
  
  if (intervalMinutes_ == minutes) return;
  
  intervalMinutes_ = minutes;
  
  if (enabled_) {
    autoSaveTimer_->start(intervalMinutes_ * 60 * 1000);
  }
  
  saveSettings();
}

void AutoSaveManager::performAutoSave() {
  if (!canvas_ || !canvas_->scene()) return;
  
  QString savePath = generateAutoSavePath();
  
  // Get scene bounds
  QGraphicsScene *scene = canvas_->scene();
  QRectF sr = scene->itemsBoundingRect();
  if (sr.isEmpty()) sr = scene->sceneRect();
  sr.adjust(-10, -10, 10, 10);
  
  // Render to image
  QImage img(sr.size().toSize(), QImage::Format_ARGB32);
  img.fill(canvas_->backgroundColor());
  
  QPainter painter(&img);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  scene->render(&painter, QRectF(), sr);
  painter.end();
  
  // Save image
  if (img.save(savePath)) {
    autoSavePath_ = savePath;
    emit autoSavePerformed(savePath);
  }
}

void AutoSaveManager::clearAutoSave() {
  if (hasAutoSave()) {
    QFile::remove(autoSavePath_);
    autoSavePath_.clear();
  }
}

bool AutoSaveManager::restoreAutoSave() {
  if (!hasAutoSave()) return false;
  
  QMessageBox::StandardButton reply = QMessageBox::question(
      nullptr, "Restore Auto-Save",
      "An auto-saved file was found. Would you like to restore it?",
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
  
  if (reply == QMessageBox::Yes) {
    canvas_->openRecentFile(autoSavePath_);
    return true;
  }
  
  // User declined - clear the auto-save
  clearAutoSave();
  return false;
}

void AutoSaveManager::loadSettings() {
  QSettings settings(AppConstants::OrganizationName, AppConstants::ApplicationName);
  enabled_ = settings.value("autosave/enabled", true).toBool();
  intervalMinutes_ = settings.value("autosave/interval", DEFAULT_INTERVAL_MINUTES).toInt();
  autoSavePath_ = settings.value("autosave/lastPath", "").toString();
}

void AutoSaveManager::saveSettings() {
  QSettings settings(AppConstants::OrganizationName, AppConstants::ApplicationName);
  settings.setValue("autosave/enabled", enabled_);
  settings.setValue("autosave/interval", intervalMinutes_);
  settings.setValue("autosave/lastPath", autoSavePath_);
}

QString AutoSaveManager::generateAutoSavePath() const {
  QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(dataDir);
  return dataDir + "/autosave.png";
}
