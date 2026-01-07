// auto_save_manager.h
#ifndef AUTO_SAVE_MANAGER_H
#define AUTO_SAVE_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QString>

class Canvas;

/**
 * @brief Manages automatic saving of the canvas at regular intervals.
 *
 * The AutoSaveManager periodically saves the canvas to a temporary file,
 * allowing recovery of work in case of unexpected shutdown.
 */
class AutoSaveManager : public QObject {
  Q_OBJECT

public:
  explicit AutoSaveManager(Canvas *canvas, QObject *parent = nullptr);
  ~AutoSaveManager() = default;

  bool isEnabled() const;
  int intervalMinutes() const;
  QString autoSavePath() const;
  bool hasAutoSave() const;

public slots:
  void setEnabled(bool enabled);
  void setIntervalMinutes(int minutes);
  void performAutoSave();
  void clearAutoSave();
  bool restoreAutoSave();

signals:
  void autoSavePerformed(const QString &path);
  void autoSaveStatusChanged(bool enabled);

private:
  Canvas *canvas_;
  QTimer *autoSaveTimer_;
  bool enabled_;
  int intervalMinutes_;
  QString autoSavePath_;

  void loadSettings();
  void saveSettings();
  QString generateAutoSavePath() const;

  static constexpr int DEFAULT_INTERVAL_MINUTES = 5;
  static constexpr int MIN_INTERVAL_MINUTES = 1;
  static constexpr int MAX_INTERVAL_MINUTES = 60;
};

#endif // AUTO_SAVE_MANAGER_H
